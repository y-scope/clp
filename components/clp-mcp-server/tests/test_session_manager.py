"""Unit tests for the SessionManager."""

import asyncio
import time
from unittest.mock import patch

import pytest

from clp_mcp_server.server import constants
from clp_mcp_server.server.session_manager import (
    PaginatedQueryResult,
    SessionManager,
    SessionState,
)


class TestConstants:
    """Constants for the test suite."""

    # Default values from constants
    ITEMS_PER_PAGE = constants.NUM_ITEMS_PER_PAGE
    SESSION_TTL_SECONDS = constants.SESSION_TTL_SECONDS
    EXPIRED_SESSION_TTL_SECONDS = SESSION_TTL_SECONDS + 1

    # Number of logs tested in unit test and its expected page counts
    EXPECTED_NUM_LOG_ENTRIES = 25
    EXPECTED_NUM_PAGES = 3
    EMPTY_LOG_ENTRIES = 0

    # 0.5 second for fast expiration tests
    FAST_SESSION_TTL_SECONDS = 0.5

    # Session names
    TEST_SESSION_ID = "test_session"
    NON_EXISTENT_SESSION_ID = "non_existent_session"
    EXPIRED_SESSION_PREFIX = "expired_"
    ACTIVE_SESSION_PREFIX = "active_"

    # Error messages
    EXCEEDS_MAX_CACHED_RESULTS_ERR = "exceeds maximum allowed cached results"
    INVALID_NUM_ITEMS_PER_PAGE_ERR = "must be a positive integer"
    GET_INSTRUCTIONS_NOT_CALLED_ERR = "Please call `get_instructions()`"
    NO_RESULTS_FOUND_IN_KQL_QUERY_ERR = "No log events found matching the KQL query."
    NO_PREVIOUS_PAGINATED_RESPONSE_ERR = "No previous paginated response in this session."
    PAGE_INDEX_OUT_OF_BOUNDS_ERR = "Page index is out of bounds."

    # Log message template
    LOG_MESSAGE_PREFIX = "log_"

    @staticmethod
    def create_log_message(index: int) -> str:
        """Create a simulated log message."""
        return f"{TestConstants.LOG_MESSAGE_PREFIX}{index}"

    @staticmethod
    def create_log_messages(count: int) -> list[str]:
        """Create a list of simulated log messages."""
        return [TestConstants.create_log_message(i) for i in range(count)]

    @staticmethod
    def create_log_messages_from_range(start: int, end: int) -> list[str]:
        """Create a list of simulated log messages for a specific range."""
        return [TestConstants.create_log_message(i) for i in range(start, end)]


class TestPaginatedQueryResult:
    """Unit tests for `PaginatedQueryResult` class."""

    def test_get_page(self) -> None:
        """Validates pagination functionality."""
        results = TestConstants.create_log_messages(TestConstants.EXPECTED_NUM_LOG_ENTRIES)
        query_result = PaginatedQueryResult(
            log_entries=results, num_items_per_page=TestConstants.ITEMS_PER_PAGE
        )

        # Test first page
        page1 = query_result.get_page(0)
        assert page1 is not None
        assert list(page1) == TestConstants.create_log_messages(TestConstants.ITEMS_PER_PAGE)
        assert page1.page == 1
        assert page1.page_count == TestConstants.EXPECTED_NUM_PAGES

        # Test second page
        page2 = query_result.get_page(1)
        assert page2 is not None
        assert list(page2) == TestConstants.create_log_messages_from_range(
            TestConstants.ITEMS_PER_PAGE, TestConstants.ITEMS_PER_PAGE * 2
        )

        # Test last page
        page3 = query_result.get_page(2)
        assert page3 is not None
        assert list(page3) == TestConstants.create_log_messages_from_range(
            TestConstants.ITEMS_PER_PAGE * 2, TestConstants.EXPECTED_NUM_LOG_ENTRIES
        )

        # Test invalid page
        page4 = query_result.get_page(3)
        assert page4 is None

    def test_query_result_initialization(self) -> None:
        """
        Validates `PaginatedQueryResult` raises `ValueError` when the number of results exceed
        `constants.MAX_CACHED_RESULTS`.
        """
        large_results = TestConstants.create_log_messages(constants.MAX_CACHED_RESULTS + 1)
        with pytest.raises(ValueError, match=TestConstants.EXCEEDS_MAX_CACHED_RESULTS_ERR):
            PaginatedQueryResult(
                log_entries=large_results, num_items_per_page=TestConstants.ITEMS_PER_PAGE
            )


class TestSessionState:
    """Unit tests for SessionState class."""

    def test_error_handling(self) -> None:
        """Validates error handling of `SessionState`."""
        session = SessionState(
            _num_items_per_page=TestConstants.ITEMS_PER_PAGE,
            _session_id=TestConstants.TEST_SESSION_ID,
            _session_ttl_seconds=TestConstants.SESSION_TTL_SECONDS,
        )

        first_page = session.cache_query_result_and_get_first_page(
            TestConstants.create_log_messages(TestConstants.EXPECTED_NUM_LOG_ENTRIES)
        )

        assert TestConstants.GET_INSTRUCTIONS_NOT_CALLED_ERR in first_page["Error"]
        page_data = session.get_page_data(1)
        assert TestConstants.GET_INSTRUCTIONS_NOT_CALLED_ERR in page_data["Error"]

        _ = session.get_instructions()
        first_page = session.cache_query_result_and_get_first_page(
            TestConstants.create_log_messages(TestConstants.EMPTY_LOG_ENTRIES)
        )
        assert TestConstants.NO_RESULTS_FOUND_IN_KQL_QUERY_ERR in first_page["Error"]

    def test_get_page_data(self) -> None:
        """Validates pagination functionality is respecting the defined dictionary format."""
        session = SessionState(
            _num_items_per_page=TestConstants.ITEMS_PER_PAGE,
            _session_id=TestConstants.TEST_SESSION_ID,
            _session_ttl_seconds=TestConstants.SESSION_TTL_SECONDS,
        )
        _ = session.get_instructions()
        results = TestConstants.create_log_messages(TestConstants.EXPECTED_NUM_LOG_ENTRIES)

        # Test no cached result
        page_data = session.get_page_data(0)
        assert page_data["Error"] == TestConstants.NO_PREVIOUS_PAGINATED_RESPONSE_ERR

        session.cache_query_result_and_get_first_page(results=results)

        # Test first page
        page_data = session.get_page_data(0)
        assert page_data is not None
        assert page_data["items"] == TestConstants.create_log_messages(TestConstants.ITEMS_PER_PAGE)
        assert page_data["num_total_pages"] == TestConstants.EXPECTED_NUM_PAGES
        assert page_data["num_total_items"] == TestConstants.EXPECTED_NUM_LOG_ENTRIES
        assert page_data["num_items_per_page"] == TestConstants.ITEMS_PER_PAGE
        assert page_data["has_next"] is True
        assert page_data["has_previous"] is False

        # Test second page
        page_data = session.get_page_data(1)
        assert page_data is not None
        assert page_data["items"] == TestConstants.create_log_messages_from_range(
            TestConstants.ITEMS_PER_PAGE, TestConstants.ITEMS_PER_PAGE * 2
        )
        assert page_data["has_next"] is True
        assert page_data["has_previous"] is True

        # Test last page
        page_data = session.get_page_data(2)
        assert page_data["items"] == TestConstants.create_log_messages_from_range(
            TestConstants.ITEMS_PER_PAGE * 2, TestConstants.EXPECTED_NUM_LOG_ENTRIES
        )
        assert page_data["has_next"] is False
        assert page_data["has_previous"] is True

        # Test invalid page
        page_data = session.get_page_data(3)
        assert page_data["Error"] == TestConstants.PAGE_INDEX_OUT_OF_BOUNDS_ERR

    def test_session_expiration(self) -> None:
        """Validates session expiration check."""
        session = SessionState(
            _num_items_per_page=TestConstants.ITEMS_PER_PAGE,
            _session_id=TestConstants.TEST_SESSION_ID,
            _session_ttl_seconds=TestConstants.FAST_SESSION_TTL_SECONDS,
        )

        assert session.is_expired() is False

        time.sleep(TestConstants.FAST_SESSION_TTL_SECONDS)

        assert session.is_expired() is True


class TestSessionManager:
    """Unit tests for SessionManager class."""

    @pytest.fixture
    def active_session_manager(self) -> SessionManager:
        """
        Create a `SessionManager` with a test session where `get_instructions()` has been called.
        """
        manager = SessionManager(session_ttl_seconds=TestConstants.SESSION_TTL_SECONDS)
        session = manager.get_or_create_session(TestConstants.TEST_SESSION_ID)
        # Simulate calling `get_instructions()`
        _ = session.get_instructions()
        return manager

    def test_get_or_create_session(self, active_session_manager: SessionManager) -> None:
        """Validates session creation and retrieval."""
        # Get existing session
        _ = active_session_manager.get_or_create_session(TestConstants.TEST_SESSION_ID)
        assert TestConstants.TEST_SESSION_ID in active_session_manager.sessions

    def test_cached_query_result(self, active_session_manager: SessionManager) -> None:
        """Validates caching query results returns correct first page data."""
        results = TestConstants.create_log_messages(TestConstants.EXPECTED_NUM_LOG_ENTRIES)

        first_page = active_session_manager.cache_query_result_and_get_first_page(
            session_id=TestConstants.TEST_SESSION_ID, query_results=results
        )

        assert first_page["items"] == TestConstants.create_log_messages(
            TestConstants.ITEMS_PER_PAGE
        )
        assert first_page["num_total_items"] == TestConstants.EXPECTED_NUM_LOG_ENTRIES

    def test_get_nth_page(self, active_session_manager: SessionManager) -> None:
        """Validates retrieving specific pages."""
        results = TestConstants.create_log_messages(TestConstants.EXPECTED_NUM_LOG_ENTRIES)

        active_session_manager.cache_query_result_and_get_first_page(
            session_id=TestConstants.TEST_SESSION_ID, query_results=results
        )

        # Get second page (index 1)
        page_data = active_session_manager.get_nth_page(TestConstants.TEST_SESSION_ID, 1)
        assert "Error" not in page_data
        assert page_data["items"] == TestConstants.create_log_messages_from_range(
            TestConstants.ITEMS_PER_PAGE, TestConstants.ITEMS_PER_PAGE * 2
        )

        # Test invalid page
        page_data = active_session_manager.get_nth_page(TestConstants.TEST_SESSION_ID, 10)
        assert TestConstants.PAGE_INDEX_OUT_OF_BOUNDS_ERR in page_data["Error"]

        # Test non-existent session - this will create a new session, but no cached query
        active_session_manager.get_or_create_session(TestConstants.NON_EXISTENT_SESSION_ID)
        # Simulate get_instructions was run
        _ = active_session_manager.sessions[
            TestConstants.NON_EXISTENT_SESSION_ID
        ].get_instructions()
        page_data = active_session_manager.get_nth_page(TestConstants.NON_EXISTENT_SESSION_ID, 0)
        assert TestConstants.NO_PREVIOUS_PAGINATED_RESPONSE_ERR in page_data["Error"]

    @pytest.mark.asyncio
    async def test_async_expiration_for_cleanup_loop(self) -> None:
        """Verifies that _cleanup_loop runs as an asynchronous task and deletes expired sessions."""
        with patch.object(constants, "EXPIRED_SESSION_SWEEP_INTERVAL_SECONDS", 0.05):
            manager = SessionManager(session_ttl_seconds=TestConstants.FAST_SESSION_TTL_SECONDS)
            await manager.start()

            # Create sessions that we will expire and NOT access again
            expired_session_ids = [f"{TestConstants.EXPIRED_SESSION_PREFIX}{i}" for i in range(20)]
            for sid in expired_session_ids:
                _ = manager.get_or_create_session(sid)

            # Ensure created sessions expire
            time.sleep(1)

            # Create active sessions that we WILL keep accessing
            active_session_ids = [f"{TestConstants.ACTIVE_SESSION_PREFIX}{i}" for i in range(20)]
            for _ in range(100):
                for sid in active_session_ids:
                    manager.get_or_create_session(sid)

            # Wait for at least one cleanup cycle
            await asyncio.sleep(0.1)

            # Verify: expired sessions deleted by cleanup asynchronously
            for sid in expired_session_ids:
                assert sid not in manager.sessions

            # Verify: active sessions still exist
            for sid in active_session_ids:
                assert sid in manager.sessions

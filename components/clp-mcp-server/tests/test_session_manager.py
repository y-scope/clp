"""Unit tests for the SessionManager."""

from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timedelta, timezone
from unittest.mock import patch

import asyncio
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
    SESSION_TTL_MINUTES = constants.SESSION_TTL_MINUTES
    EXPIRED_SESSION_TTL_MINUTES = constants.SESSION_TTL_MINUTES + 1

    # Number of logs tested in unit test and its expected page counts
    EXPECTED_PAGES_25_ITEMS = 3
    SAMPLE_RESULTS_COUNT_25 = 25


class TestPaginatedQueryResult:
    """Unit tests for PaginatedQueryResult class."""

    def test_query_result_initialization(self) -> None:
        """
        Validates PaginatedQueryResult raises ValueError when results exceed MAX_CACHED_RESULTS.
        """
        # Create a result with more than max_cached_results
        large_results = [f"log_{i}" for i in range(constants.MAX_CACHED_RESULTS + 1)]
        with pytest.raises(ValueError, match="exceeds maximum allowed cached results"):
            PaginatedQueryResult(
                result_log_entries=large_results,
                num_items_per_page=TestConstants.ITEMS_PER_PAGE
            )

    def test_get_page(self) -> None:
        """Validates pagination functionality."""
        results = [f"log_{i}" for i in range(TestConstants.SAMPLE_RESULTS_COUNT_25)]
        query_result = PaginatedQueryResult(
            result_log_entries=results,
            num_items_per_page=TestConstants.ITEMS_PER_PAGE
        )

        # Test first page
        page1 = query_result.get_page(0)
        assert page1 is not None
        assert list(page1) == [f"log_{i}" for i in range(TestConstants.ITEMS_PER_PAGE)]
        assert page1.page == 1
        assert page1.page_count == TestConstants.EXPECTED_PAGES_25_ITEMS

        # Test second page
        page2 = query_result.get_page(1)
        assert page2 is not None
        assert list(page2) == [f"log_{i}" for i in range(
            TestConstants.ITEMS_PER_PAGE, TestConstants.ITEMS_PER_PAGE * 2
        )]

        # Test last page
        page3 = query_result.get_page(2)
        assert page3 is not None
        assert (
            list(page3) ==
            [f"log_{i}" for i in range(
                TestConstants.ITEMS_PER_PAGE * 2, TestConstants.SAMPLE_RESULTS_COUNT_25
            )]
        )

        # Test invalid page
        page4 = query_result.get_page(3)
        assert page4 is None


class TestSessionState:
    """Unit tests for SessionState class."""

    def test_get_page_data(self) -> None:
        """Validates pagination functionality is respecting the defined dictionary format."""
        session = SessionState(
            session_id="test_session",
            _num_items_per_page=TestConstants.ITEMS_PER_PAGE,
            _session_ttl_minutes=TestConstants.SESSION_TTL_MINUTES
        )
        results = [f"log_{i}" for i in range(TestConstants.SAMPLE_RESULTS_COUNT_25)]

        # Test no cached result
        page_data = session.get_page_data(0)
        assert page_data["Error"] == "No previous paginated response in this session."

        session.cache_query_result(results=results)

        # Test first page
        page_data = session.get_page_data(0)
        assert page_data is not None
        assert (
            page_data["items"] ==
            [f"log_{i}" for i in range(TestConstants.ITEMS_PER_PAGE)]
        )
        assert page_data["total_pages"] == TestConstants.EXPECTED_PAGES_25_ITEMS
        assert page_data["total_items"] == TestConstants.SAMPLE_RESULTS_COUNT_25
        assert page_data["num_items_per_page"] == TestConstants.ITEMS_PER_PAGE
        assert page_data["has_next"] is True
        assert page_data["has_previous"] is False

        # Test second page
        page_data = session.get_page_data(1)
        assert page_data is not None
        assert (
            page_data["items"] ==
            [f"log_{i}" for i in range(
                TestConstants.ITEMS_PER_PAGE, TestConstants.ITEMS_PER_PAGE * 2
            )]
        )
        assert page_data["has_next"] is True
        assert page_data["has_previous"] is True

        # Test last page
        page_data = session.get_page_data(2)
        assert (
            page_data["items"] ==
            [f"log_{i}" for i in range(
                TestConstants.ITEMS_PER_PAGE * 2, TestConstants.SAMPLE_RESULTS_COUNT_25
            )]
        )
        assert page_data["has_next"] is False
        assert page_data["has_previous"] is True

        # Test invalid page
        page_data = session.get_page_data(3)
        assert page_data["Error"] == "Page index is out of bounds."

    def test_session_expiration(self) -> None:
        """Validates session expiration check."""
        session = SessionState(
            session_id="test_session",
            _num_items_per_page=TestConstants.ITEMS_PER_PAGE,
            _session_ttl_minutes=TestConstants.SESSION_TTL_MINUTES
        )

        assert session.is_expired() is False

        # Simulate old session
        session.last_accessed = datetime.now(timezone.utc) - timedelta(minutes=61)
        assert session.is_expired() is True


class TestSessionManager:
    """Unit tests for SessionManager class."""

    @pytest.fixture
    def active_session_manager(self) -> SessionManager:
        """Create a SessionManager with a test session where get_instructions() has run."""
        manager = SessionManager(session_ttl_minutes=TestConstants.SESSION_TTL_MINUTES)
        session = manager.get_or_create_session("test_session")
        # Simulate get_instructions was run
        session.is_instructions_retrieved = True
        return manager

    def test_get_or_create_session(self, active_session_manager: SessionManager) -> None:
        """Validates session creation and retrieval."""
        # Get existing session
        session1 = active_session_manager.get_or_create_session("test_session")
        assert session1.session_id == "test_session"
        assert "test_session" in active_session_manager.sessions

    def test_cached_query_result(self, active_session_manager: SessionManager) -> None:
        """Validates caching query results returns correct first page data."""
        results = [f"log_{i}" for i in range(TestConstants.SAMPLE_RESULTS_COUNT_25)]

        first_page = active_session_manager.cache_query_result(
            session_id="test_session", query_results=results
        )

        assert (
            first_page["items"] ==
            [f"log_{i}" for i in range(TestConstants.ITEMS_PER_PAGE)]
        )
        assert first_page["total_items"] == TestConstants.SAMPLE_RESULTS_COUNT_25

    def test_get_nth_page(self, active_session_manager: SessionManager) -> None:
        """Validates retrieving specific pages."""
        results = [f"log_{i}" for i in range(TestConstants.SAMPLE_RESULTS_COUNT_25)]

        active_session_manager.cache_query_result(session_id="test_session", query_results=results)

        # Get second page (index 1)
        page_data = active_session_manager.get_nth_page("test_session", 1)
        assert "Error" not in page_data
        assert (
            page_data["items"] ==
            [f"log_{i}" for i in range(
                TestConstants.ITEMS_PER_PAGE, TestConstants.ITEMS_PER_PAGE * 2
            )]
        )

        # Test invalid page
        page_data = active_session_manager.get_nth_page("test_session", 10)
        assert "Page index is out of bounds." in page_data["Error"]

        # Test non-existent session - this will create a new session, but no cached query
        active_session_manager.get_or_create_session("non_existent")
        # Simulate get_instructions was run
        active_session_manager.sessions["non_existent"].is_instructions_retrieved = True
        page_data = active_session_manager.get_nth_page("non_existent", 0)
        assert "No previous paginated response in this session." in page_data["Error"]

    def test_no_get_instruction(self, active_session_manager: SessionManager) -> None:
        """Validates error handling when get_instructions() has not been called."""
        session = active_session_manager.get_or_create_session("test_session")
        # Simulate get_instructions was NOT run
        session.is_instructions_retrieved = False

        first_page = active_session_manager.cache_query_result(
            "test_session", [f"log_{i}" for i in range(TestConstants.SAMPLE_RESULTS_COUNT_25)]
        )
        assert "Please call get_instructions()" in first_page["Error"]
        page_data = active_session_manager.get_nth_page("test_session", 0)
        assert "Please call get_instructions()" in page_data["Error"]

    def test_cleanup_expired_sessions(self) -> None:
        """Validates the cleanup of expired sessions."""
        manager = SessionManager(session_ttl_minutes=TestConstants.SESSION_TTL_MINUTES)

        # Create sessions with different ages
        session1 = manager.get_or_create_session("session1")
        session2 = manager.get_or_create_session("session2")
        manager.get_or_create_session("session3")

        # Make session1 and session2 expired
        session1.last_accessed = (
            datetime.now(timezone.utc) -
            timedelta(minutes=TestConstants.EXPIRED_SESSION_TTL_MINUTES)
        )
        session2.last_accessed = (
            datetime.now(timezone.utc) -
            timedelta(minutes=TestConstants.EXPIRED_SESSION_TTL_MINUTES)
        )

        manager.cleanup_expired_sessions()

        assert "session1" not in manager.sessions
        assert "session2" not in manager.sessions
        assert "session3" in manager.sessions

    @pytest.mark.asyncio
    async def test_async_expiration_for_cleanup_loop(self) -> None:
        """Verifies that _cleanup_loop runs asynchronously and deletes expired sessions."""
        with patch.object(constants, 'EXPIRED_SESSION_SWEEP_INTERVAL_SECONDS', 0.05):
            manager = SessionManager(session_ttl_minutes=TestConstants.SESSION_TTL_MINUTES)
            await manager.start()

            # Create sessions that we will expire and NOT access again
            expired_session_ids = [f"expired_{i}" for i in range(20)]
            for sid in expired_session_ids:
                session = manager.get_or_create_session(sid)
                session.last_accessed = (
                    datetime.now(timezone.utc) -
                    timedelta(minutes=TestConstants.EXPIRED_SESSION_TTL_MINUTES)
                )
            
            # Create active sessions that we WILL keep accessing
            active_session_ids = [f"active_{i}" for i in range(20)]
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

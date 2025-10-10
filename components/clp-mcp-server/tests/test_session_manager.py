"""Unit tests for the SessionManager."""

from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timedelta, timezone

import pytest

from clp_mcp_server.server.constants import CLPMcpConstants
from clp_mcp_server.server.session_manager import (
    QueryResult,
    SessionManager,
    SessionState,
)


class TestConstants:
    """Constants for the test suite."""

    # Default values from CLPMcpConstants
    ITEMS_PER_PAGE = CLPMcpConstants.ITEM_PER_PAGE
    SESSION_TTL_MINUTES = CLPMcpConstants.SESSION_TTL_MINUTES

    # Number of logs tested in unit test and its expected page counts
    EXPECTED_PAGES_25_ITEMS = 3
    SAMPLE_RESULTS_COUNT_25 = 25


class TestQueryResult:
    """Unit tests for QueryResult class."""

    def test_query_result_initialization(self) -> None:
        """Validates QueryResult errors out on too many results."""
        # Create a result with more than max_cached_results
        large_results = [f"log_{i}" for i in range(1500)]
        with pytest.raises(ValueError, match="exceeds maximum allowed cached results"):
            QueryResult(
                total_results=large_results,
                items_per_page=TestConstants.ITEMS_PER_PAGE
            )

    def test_get_page(self) -> None:
        """Validates pagination functionality."""
        results = [f"log_{i}" for i in range(25)]
        query_result = QueryResult(
            total_results=results,
            items_per_page=TestConstants.ITEMS_PER_PAGE
        )

        # Test first page
        page1 = query_result.get_page(1)
        assert page1 is not None
        assert list(page1) == [f"log_{i}" for i in range(10)]
        assert page1.page == 1
        assert page1.page_count == TestConstants.EXPECTED_PAGES_25_ITEMS

        # Test second page
        page2 = query_result.get_page(2)
        assert page2 is not None
        assert list(page2) == [f"log_{i}" for i in range(10, 20)]

        # Test last page
        page3 = query_result.get_page(3)
        assert page3 is not None
        assert (
            list(page3) ==
            [f"log_{i}" for i in range(20, 25)]
        )

        # Test invalid page
        page4 = query_result.get_page(4)
        assert page4 is None


class TestSessionState:
    """Unit tests for SessionState class."""

    def test_get_page_data(self) -> None:
        """Validates pagination functionality is respecting the defined dictionary format."""
        session = SessionState(
            session_id="test_session",
            items_per_page=TestConstants.ITEMS_PER_PAGE,
            session_ttl_minutes=TestConstants.SESSION_TTL_MINUTES
        )
        results = [f"log_{i}" for i in range(25)]

        # Test no cached result
        page_data = session.get_page_data(1)
        assert page_data["Error"] == "No previous paginated response in this session."

        session.cache_query_result(results=results)

        # Test first page
        page_data = session.get_page_data(1)
        assert page_data is not None
        assert (
            page_data["items"] ==
            [f"log_{i}" for i in range(TestConstants.ITEMS_PER_PAGE)]
        )
        assert page_data["total_pages"] == TestConstants.EXPECTED_PAGES_25_ITEMS
        assert page_data["total_items"] == TestConstants.SAMPLE_RESULTS_COUNT_25
        assert page_data["items_per_page"] == TestConstants.ITEMS_PER_PAGE
        assert page_data["has_next"] is True
        assert page_data["has_previous"] is False

        # Test second page
        page_data = session.get_page_data(2)
        assert page_data is not None
        assert (
            page_data["items"] ==
            [f"log_{i}" for i in range(TestConstants.ITEMS_PER_PAGE, 20)]
        )
        assert page_data["has_next"] is True
        assert page_data["has_previous"] is True

        # Test last page
        page_data = session.get_page_data(3)
        assert (
            page_data["items"] ==
            [f"log_{i}" for i in range(20, TestConstants.SAMPLE_RESULTS_COUNT_25)]
        )
        assert page_data["has_next"] is False
        assert page_data["has_previous"] is True

        # Test invalid page
        page_data = session.get_page_data(4)
        assert page_data["Error"] == "Page index is out of bounds."

    def test_session_expiration(self) -> None:
        """Validates session expiration check."""
        session = SessionState(
            session_id="test_session",
            items_per_page=TestConstants.ITEMS_PER_PAGE,
            session_ttl_minutes=TestConstants.SESSION_TTL_MINUTES
        )

        assert session.is_expired() is False

        # Simulate old session
        session.last_accessed = datetime.now(timezone.utc) - timedelta(minutes=61)
        assert session.is_expired() is True


class TestSessionManager:
    """Unit tests for SessionManager class."""

    def test_get_or_create_session(self) -> None:
        """Validates session creation and retrieval."""
        manager = SessionManager(session_ttl_minutes=TestConstants.SESSION_TTL_MINUTES)

        # Create new session
        session1 = manager.get_or_create_session("session1")
        assert session1.session_id == "session1"
        assert session1.items_per_page == TestConstants.ITEMS_PER_PAGE
        assert session1.session_ttl_minutes == TestConstants.SESSION_TTL_MINUTES
        assert "session1" in manager.sessions

        # Get existing session
        session1_again = manager.get_or_create_session("session1")
        assert session1_again is session1

    def test_cached_query_result(self) -> None:
        """Validates caching query results."""
        manager = SessionManager(session_ttl_minutes=TestConstants.SESSION_TTL_MINUTES)
        results = [f"log_{i}" for i in range(25)]
        manager.get_or_create_session("test_session")
        # Simulate get_instructions was run
        manager.sessions["test_session"].ran_instructions = True

        first_page = manager.cache_query_result(session_id="test_session", query_results=results)

        assert (
            first_page["items"] ==
            [f"log_{i}" for i in range(TestConstants.ITEMS_PER_PAGE)]
        )
        assert first_page["total_items"] == TestConstants.SAMPLE_RESULTS_COUNT_25

    def test_get_nth_page(self) -> None:
        """Validates retrieving specific pages."""
        manager = SessionManager(session_ttl_minutes=TestConstants.SESSION_TTL_MINUTES)
        results = [f"log_{i}" for i in range(TestConstants.SAMPLE_RESULTS_COUNT_25)]
        manager.get_or_create_session("test_session")
        # Simulate get_instructions was run
        manager.sessions["test_session"].ran_instructions = True

        manager.cache_query_result(session_id="test_session", query_results=results)

        # Get second page (index 1)
        page_data = manager.get_nth_page("test_session", 1)
        assert "Error" not in page_data
        assert (
            page_data["items"] ==
            [f"log_{i}" for i in range(TestConstants.ITEMS_PER_PAGE, 20)]
        )

        # Test invalid page
        page_data = manager.get_nth_page("test_session", 10)
        assert "Page index is out of bounds." in page_data["Error"]

        # Test non-existent session - this will create a new session, but no cached query
        manager.get_or_create_session("non_existent")
        # Simulate get_instructions was run
        manager.sessions["non_existent"].ran_instructions = True
        page_data = manager.get_nth_page("non_existent", 0)
        assert "No previous paginated response in this session." in page_data["Error"]

    def test_no_get_instruction(self) -> None:
        """Validates the error handling when get_instruction is not run."""
        manager = SessionManager(session_ttl_minutes=TestConstants.SESSION_TTL_MINUTES)
        manager.get_or_create_session("test_session")

        first_page = manager.cache_query_result("test_session", [f"log_{i}" for i in range(15)])
        assert "Please call get_instructions()" in first_page["Error"]
        page_data = manager.get_nth_page("test_session", 0)
        assert "Please call get_instructions()" in page_data["Error"]

    def test_cleanup_expired_sessions(self) -> None:
        """Validates the cleanup of expired sessions."""
        manager = SessionManager(session_ttl_minutes=TestConstants.SESSION_TTL_MINUTES)

        # Create sessions with different ages
        session1 = manager.get_or_create_session("session1")
        session2 = manager.get_or_create_session("session2")
        manager.get_or_create_session("session3")  # Keep session3 but don't assign to variable

        # Make session1 and session2 expired
        session1.last_accessed = datetime.now(timezone.utc) - timedelta(minutes=61)
        session2.last_accessed = datetime.now(timezone.utc) - timedelta(minutes=65)

        manager.cleanup_expired_sessions()

        assert "session1" not in manager.sessions
        assert "session2" not in manager.sessions
        assert "session3" in manager.sessions

    @pytest.mark.repeat(10)
    def test_thread_safety_cleanup_and_get_or_create_session(self) -> None:
        """Validates thread safety of cleanup_expired_sessions and get_or_create_session."""
        manager = SessionManager(session_ttl_minutes=10)

        def cleanup_task() -> None:
            """Continuously cleanup expired sessions."""
            for _ in range(10000):
                for i in range(50):
                    session = manager.get_or_create_session(f"session_{i}")
                    if i < TestConstants.SAMPLE_RESULTS_COUNT_25:
                        session.last_accessed = datetime.now(timezone.utc) - timedelta(minutes=20)
                manager.cleanup_expired_sessions()

        def access_task() -> None:
            """Continuously create and access sessions."""
            for i in range(10000):
                session_id = f"session_{i % 50}"
                manager.get_or_create_session(session_id)

        # Run cleanup and access operations concurrently
        with ThreadPoolExecutor(max_workers=10) as executor:
            futures = []
            futures.append(executor.submit(cleanup_task))
            futures.append(executor.submit(access_task))

            for future in futures:
                future.result() # ensure no run time exceptions


if __name__ == "__main__":
    pytest.main([__file__, "-v"])

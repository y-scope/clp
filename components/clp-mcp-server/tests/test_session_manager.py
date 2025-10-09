"""Tests for the SessionManager class."""

from datetime import datetime, timedelta, timezone

import pytest

from clp_mcp_server.server.constants import CLPMcpConstants
from clp_mcp_server.server.session_manager import (
    QueryResult,
    SessionManager,
    SessionState,
)


class TestConstants:
    """Constants used throughout the test suite."""

    # Default values from CLPMcpConstants
    ITEMS_PER_PAGE_DEFAULT = CLPMcpConstants.ITEM_PER_PAGE
    MAX_CACHED_RESULTS_DEFAULT = CLPMcpConstants.MAX_CACHED_RESULTS
    SESSION_TTL_MINUTES_DEFAULT = CLPMcpConstants.SESSION_TTL_MINUTES

    # Expected page counts for different result sizes
    EXPECTED_PAGES_20_ITEMS = 2
    EXPECTED_PAGES_25_ITEMS = 3
    EXPECTED_PAGES_35_ITEMS = 4

    # Sample result counts for testing
    SAMPLE_RESULTS_COUNT_12 = 12
    SAMPLE_RESULTS_COUNT_15 = 15
    SAMPLE_RESULTS_COUNT_25 = 25
    SAMPLE_RESULTS_COUNT_50 = 50

    # Custom items per page values
    ITEMS_PER_PAGE_15 = 15

    # Session TTL values in minutes
    SESSION_TTL_30_MIN = 30
    SESSION_TTL_60_MIN = 60

    # Page numbers
    PAGE_NUMBER_2 = 2

    # Session counts
    REMOVED_SESSIONS_COUNT = 2

    # Large result count for testing max limit
    LARGE_RESULTS_COUNT = 1500


class TestQueryResult:
    """Test cases for QueryResult class."""

    def test_query_result_initialization(self) -> None:
        """Test QueryResult initialization and error on too many results."""
        # Create a result with more than max_cached_results
        large_results = [f"log_{i}" for i in range(TestConstants.LARGE_RESULTS_COUNT)]
        with pytest.raises(ValueError, match="exceeds maximum allowed cached results"):
            QueryResult(
                total_results=large_results,
                items_per_page=TestConstants.ITEMS_PER_PAGE_DEFAULT
            )

    def test_get_page(self) -> None:
        """Test pagination functionality."""
        results = [f"log_{i}" for i in range(TestConstants.SAMPLE_RESULTS_COUNT_25)]
        query_result = QueryResult(
            total_results=results,
            items_per_page=TestConstants.ITEMS_PER_PAGE_DEFAULT
        )

        # Test first page
        page1 = query_result.get_page(1)
        assert page1 is not None
        assert list(page1) == [f"log_{i}" for i in range(TestConstants.ITEMS_PER_PAGE_DEFAULT)]
        assert page1.page == 1
        assert page1.page_count == TestConstants.EXPECTED_PAGES_25_ITEMS

        # Test second page
        page2 = query_result.get_page(2)
        assert page2 is not None
        assert list(page2) == [f"log_{i}" for i in range(TestConstants.ITEMS_PER_PAGE_DEFAULT, 20)]

        # Test last page
        page3 = query_result.get_page(3)
        assert page3 is not None
        assert (
            list(page3) ==
            [f"log_{i}" for i in range(20, TestConstants.SAMPLE_RESULTS_COUNT_25)]
        )

        # Test invalid page
        page4 = query_result.get_page(4)
        assert page4 is None

    def test_total_pages(self) -> None:
        """Test total pages calculation."""
        # Test with exact multiple
        query_result = QueryResult(
            total_results=["log"] * 20,
            items_per_page=TestConstants.ITEMS_PER_PAGE_DEFAULT
        )
        assert query_result.total_pages == TestConstants.EXPECTED_PAGES_20_ITEMS

        # Test with remainder
        query_result = QueryResult(
            total_results=["log"] * TestConstants.SAMPLE_RESULTS_COUNT_25,
            items_per_page=TestConstants.ITEMS_PER_PAGE_DEFAULT
        )
        assert query_result.total_pages == TestConstants.EXPECTED_PAGES_25_ITEMS

        # Test with empty results
        query_result = QueryResult(
            total_results=[],
            items_per_page=TestConstants.ITEMS_PER_PAGE_DEFAULT
        )
        assert query_result.total_pages == 0


class TestSessionState:
    """Test cases for SessionState class."""

    def test_get_page_data(self) -> None:
        """Test getting page data in dictionary format."""
        session = SessionState(
            session_id="test_session",
            items_per_page=TestConstants.ITEMS_PER_PAGE_DEFAULT,
            session_ttl_minutes=TestConstants.SESSION_TTL_60_MIN
        )
        results = [f"log_{i}" for i in range(TestConstants.SAMPLE_RESULTS_COUNT_25)]

        session.cache_query_result(results=results)

        # Test first page
        page_data = session.get_page_data(1)
        assert page_data is not None
        assert (
            page_data["items"] ==
            [f"log_{i}" for i in range(TestConstants.ITEMS_PER_PAGE_DEFAULT)]
        )
        assert page_data["total_pages"] == TestConstants.EXPECTED_PAGES_25_ITEMS
        assert page_data["total_items"] == TestConstants.SAMPLE_RESULTS_COUNT_25
        assert page_data["items_per_page"] == TestConstants.ITEMS_PER_PAGE_DEFAULT
        assert page_data["has_next"] is True
        assert page_data["has_previous"] is False

        # Test second page
        page_data = session.get_page_data(2)
        assert page_data is not None
        assert (
            page_data["items"] ==
            [f"log_{i}" for i in range(TestConstants.ITEMS_PER_PAGE_DEFAULT, 20)]
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

        # Test with no cached result
        session.cached_query_result = None
        page_data = session.get_page_data(0)
        assert page_data["Error"] == "No previous paginated response in this session."

    def test_session_expiration(self) -> None:
        """Test session expiration check."""
        session = SessionState(
            session_id="test_session",
            items_per_page=TestConstants.ITEMS_PER_PAGE_DEFAULT,
            session_ttl_minutes=TestConstants.SESSION_TTL_60_MIN
        )

        # Test not expired
        assert session.is_expired() is False

        # Simulate old session
        session.last_accessed = datetime.now(timezone.utc) - timedelta(minutes=61)
        assert session.is_expired() is True

    def test_custom_ttl(self) -> None:
        """Test SessionState with custom TTL."""
        session = SessionState(
            session_id="test_session",
            items_per_page=TestConstants.ITEMS_PER_PAGE_DEFAULT,
            session_ttl_minutes=TestConstants.SESSION_TTL_30_MIN
        )

        # Test not expired
        assert session.is_expired() is False

        # Simulate session that's 31 minutes old
        session.last_accessed = datetime.now(timezone.utc) - timedelta(minutes=31)
        assert session.is_expired() is True

        # Simulate session that's 29 minutes old
        session.last_accessed = datetime.now(timezone.utc) - timedelta(minutes=29)
        assert session.is_expired() is False


class TestSessionManager:
    """Test cases for SessionManager class."""

    def test_get_or_create_session(self) -> None:
        """Test session creation and retrieval."""
        manager = SessionManager(session_ttl_minutes=TestConstants.SESSION_TTL_MINUTES_DEFAULT)

        # Create new session
        session1 = manager.get_or_create_session("session1")
        assert session1.session_id == "session1"
        assert session1.items_per_page == TestConstants.ITEMS_PER_PAGE_DEFAULT
        assert session1.session_ttl_minutes == TestConstants.SESSION_TTL_MINUTES_DEFAULT
        assert "session1" in manager.sessions

        # Get existing session
        session1_again = manager.get_or_create_session("session1")
        assert session1_again is session1

    def test_cached_query_result(self) -> None:
        """Test caching query results through manager."""
        manager = SessionManager(session_ttl_minutes=TestConstants.SESSION_TTL_MINUTES_DEFAULT)
        results = [f"log_{i}" for i in range(TestConstants.SAMPLE_RESULTS_COUNT_12)]
        manager.get_or_create_session("test_session")
        # Simulate that instructions were run
        manager.sessions["test_session"].ran_instructions = True
        first_page = manager.cache_query_result(
            session_id="test_session",
            results=results,
        )

        assert (
            first_page["items"] ==
            [f"log_{i}" for i in range(TestConstants.ITEMS_PER_PAGE_DEFAULT)]
        )
        assert first_page["total_items"] == TestConstants.SAMPLE_RESULTS_COUNT_12

    def test_get_nth_page(self) -> None:
        """Test retrieving specific pages."""
        manager = SessionManager(session_ttl_minutes=TestConstants.SESSION_TTL_MINUTES_DEFAULT)
        results = [f"log_{i}" for i in range(TestConstants.SAMPLE_RESULTS_COUNT_25)]
        manager.get_or_create_session("test_session")
        # Simulate that instructions were run
        manager.sessions["test_session"].ran_instructions = True

        manager.cache_query_result(session_id="test_session", results=results)

        # Get second page (index 1)
        page_data = manager.get_nth_page("test_session", 1)
        assert "Error" not in page_data
        assert (
            page_data["items"] ==
            [f"log_{i}" for i in range(TestConstants.ITEMS_PER_PAGE_DEFAULT, 20)]
        )

        # Test invalid page
        page_data = manager.get_nth_page("test_session", 10)
        assert "Error" in page_data
        assert "Page index is out of bounds." in page_data["Error"]

        # Test non-existent session - this will create a new session, so no error
        manager.get_or_create_session("non_existent")
        # Simulate that instructions were run
        manager.sessions["non_existent"].ran_instructions = True
        page_data = manager.get_nth_page("non_existent", 0)
        assert "Error" in page_data
        assert "No previous paginated response in this session." in page_data["Error"]

    def test_session_expiration(self) -> None:
        """Test session expiration handling."""
        manager = SessionManager(session_ttl_minutes=1)
        manager.get_or_create_session("test_session")
        # Simulate that instructions were run
        manager.sessions["test_session"].ran_instructions = True


        session = manager.get_or_create_session("test_session")
        session.last_accessed = datetime.now(timezone.utc) - timedelta(minutes=2)

        # When we try to get the session again, it should be recreated (expired session is deleted)
        new_session = manager.get_or_create_session("test_session")
        # Simulate that instructions were run
        manager.sessions["test_session"].ran_instructions = True
        assert new_session.session_id == "test_session"
        # The session should be newly created, so no cached query
        page_data = manager.get_nth_page("test_session", 0)
        assert "Error" in page_data
        assert "No previous paginated response in this session." in page_data["Error"]

    def test_no_cached_query(self) -> None:
        """Test handling when no query has been cached."""
        manager = SessionManager(session_ttl_minutes=TestConstants.SESSION_TTL_MINUTES_DEFAULT)
        manager.get_or_create_session("test_session")
        # Simulate that instructions were run
        manager.sessions["test_session"].ran_instructions = True
        page_data = manager.get_nth_page("test_session", 0)
        assert "Error" in page_data
        assert "No previous paginated response in this session." in page_data["Error"]

    def test_cleanup_expired_sessions(self) -> None:
        """Test cleanup of expired sessions."""
        manager = SessionManager(session_ttl_minutes=TestConstants.SESSION_TTL_30_MIN)

        # Create sessions with different ages
        session1 = manager.get_or_create_session("session1")
        session2 = manager.get_or_create_session("session2")
        manager.get_or_create_session("session3")  # Keep session3 but don't assign to variable

        # Make session1 and session2 expired
        session1.last_accessed = datetime.now(timezone.utc) - timedelta(minutes=31)
        session2.last_accessed = datetime.now(timezone.utc) - timedelta(minutes=35)

        manager.cleanup_expired_sessions()

        assert "session1" not in manager.sessions
        assert "session2" not in manager.sessions
        assert "session3" in manager.sessions


if __name__ == "__main__":
    pytest.main([__file__, "-v"])

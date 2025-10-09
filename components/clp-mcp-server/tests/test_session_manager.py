"""Tests for the SessionManager class."""

from datetime import datetime, timedelta, timezone

import pytest

from clp_mcp_server.server.constants import CLPMcpConstants
from clp_mcp_server.server.session_manager import (
    QueryResult,
    SessionManager,
    SessionState,
)

# Test constants to avoid magic values
ITEMS_PER_PAGE_DEFAULT = CLPMcpConstants.ITEM_PER_PAGE
MAX_CACHED_RESULTS_DEFAULT = CLPMcpConstants.MAX_CACHED_RESULTS
EXPECTED_PAGES_25_ITEMS = 3
EXPECTED_PAGES_20_ITEMS = 2
EXPECTED_PAGES_35_ITEMS = 4
SAMPLE_RESULTS_COUNT_50 = 50
SAMPLE_RESULTS_COUNT_25 = 25
SAMPLE_RESULTS_COUNT_15 = 15
SAMPLE_RESULTS_COUNT_12 = 12
ITEMS_PER_PAGE_15 = 15
SESSION_TTL_30_MIN = 30
SESSION_TTL_60_MIN = 60
PAGE_NUMBER_2 = 2
REMOVED_SESSIONS_COUNT = 2


class TestQueryResult:
    """Test cases for QueryResult class."""

    def test_query_result_initialization(self) -> None:
        """Test QueryResult initialization and error on too many results."""
        # Create a result with more than max_cached_results
        large_results = [f"log_{i}" for i in range(1500)]
        with pytest.raises(ValueError) as excinfo:
            QueryResult(total_results=large_results, items_per_page=ITEMS_PER_PAGE_DEFAULT)
        assert "exceeds maximum allowed cached results" in str(excinfo.value)

    def test_get_page(self) -> None:
        """Test pagination functionality."""
        results = [f"log_{i}" for i in range(SAMPLE_RESULTS_COUNT_25)]
        query_result = QueryResult(total_results=results, items_per_page=ITEMS_PER_PAGE_DEFAULT)

        # Test first page
        page1 = query_result.get_page(1)
        assert page1 is not None
        assert list(page1) == [f"log_{i}" for i in range(ITEMS_PER_PAGE_DEFAULT)]
        assert page1.page == 1
        assert page1.page_count == EXPECTED_PAGES_25_ITEMS

        # Test second page
        page2 = query_result.get_page(2)
        assert page2 is not None
        assert list(page2) == [f"log_{i}" for i in range(ITEMS_PER_PAGE_DEFAULT, 20)]

        # Test last page
        page3 = query_result.get_page(3)
        assert page3 is not None
        assert list(page3) == [f"log_{i}" for i in range(20, SAMPLE_RESULTS_COUNT_25)]

        # Test invalid page
        page4 = query_result.get_page(4)
        assert page4 is None

    def test_total_pages(self) -> None:
        """Test total pages calculation."""
        # Test with exact multiple
        query_result = QueryResult(total_results=["log"] * 20, items_per_page=ITEMS_PER_PAGE_DEFAULT)
        assert query_result.total_pages == EXPECTED_PAGES_20_ITEMS

        # Test with remainder
        query_result = QueryResult(total_results=["log"] * SAMPLE_RESULTS_COUNT_25, items_per_page=ITEMS_PER_PAGE_DEFAULT)
        assert query_result.total_pages == EXPECTED_PAGES_25_ITEMS

        # Test with empty results
        query_result = QueryResult(total_results=[], items_per_page=ITEMS_PER_PAGE_DEFAULT)
        assert query_result.total_pages == 0


class TestSessionState:
    """Test cases for SessionState class."""

    def test_cached_query_result(self) -> None:
        """Test caching query results."""
        session = SessionState(
            session_id="test_session",
            items_per_page=ITEMS_PER_PAGE_DEFAULT,
            session_ttl_minutes=SESSION_TTL_60_MIN
        )
        results = [f"log_{i}" for i in range(SAMPLE_RESULTS_COUNT_50)]

        query_result = session.cache_query_result(results=results)

        assert session.cached_query_result is not None
        assert len(session.cached_query_result.total_results) == SAMPLE_RESULTS_COUNT_50
        assert query_result is session.cached_query_result
        assert query_result.items_per_page == ITEMS_PER_PAGE_DEFAULT

    def test_get_page_data(self) -> None:
        """Test getting page data in dictionary format."""
        session = SessionState(
            session_id="test_session",
            items_per_page=ITEMS_PER_PAGE_DEFAULT,
            session_ttl_minutes=SESSION_TTL_60_MIN
        )
        results = [f"log_{i}" for i in range(SAMPLE_RESULTS_COUNT_25)]

        session.cache_query_result(results=results)

        # Test first page
        page_data = session.get_page_data(1)
        assert page_data is not None
        assert page_data["items"] == [f"log_{i}" for i in range(ITEMS_PER_PAGE_DEFAULT)]
        assert page_data["page_number"] == 1
        assert page_data["total_pages"] == EXPECTED_PAGES_25_ITEMS
        assert page_data["total_items"] == SAMPLE_RESULTS_COUNT_25
        assert page_data["items_per_page"] == ITEMS_PER_PAGE_DEFAULT
        assert page_data["has_next"] is True
        assert page_data["has_previous"] is False

        # Test second page
        page_data = session.get_page_data(2)
        assert page_data is not None
        assert page_data["items"] == [f"log_{i}" for i in range(ITEMS_PER_PAGE_DEFAULT, 20)]
        assert page_data["page_number"] == PAGE_NUMBER_2
        assert page_data["has_next"] is True
        assert page_data["has_previous"] is True

        # Test last page
        page_data = session.get_page_data(3)
        assert page_data["items"] == [f"log_{i}" for i in range(20, SAMPLE_RESULTS_COUNT_25)]
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
            items_per_page=ITEMS_PER_PAGE_DEFAULT,
            session_ttl_minutes=SESSION_TTL_60_MIN
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
            items_per_page=ITEMS_PER_PAGE_DEFAULT,
            session_ttl_minutes=SESSION_TTL_30_MIN
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
        manager = SessionManager(items_per_page=ITEMS_PER_PAGE_DEFAULT, session_ttl_minutes=CLPMcpConstants.SESSION_TTL_MINUTES)

        # Create new session
        session1 = manager.get_or_create_session("session1")
        assert session1.session_id == "session1"
        assert session1.items_per_page == ITEMS_PER_PAGE_DEFAULT
        assert session1.session_ttl_minutes == CLPMcpConstants.SESSION_TTL_MINUTES
        assert "session1" in manager.sessions

        # Get existing session
        session1_again = manager.get_or_create_session("session1")
        assert session1_again is session1

    def test_cached_query_result(self) -> None:
        """Test caching query results through manager."""
        manager = SessionManager(items_per_page=ITEMS_PER_PAGE_DEFAULT, session_ttl_minutes=CLPMcpConstants.SESSION_TTL_MINUTES)
        results = [f"log_{i}" for i in range(SAMPLE_RESULTS_COUNT_12)]
        manager.get_or_create_session("test_session")
        manager.sessions["test_session"].ran_instructions = True  # Simulate that instructions were run

        first_page, total_pages = manager.cache_query_result(
            session_id="test_session",
            results=results,
        )

        assert first_page["items"] == [f"log_{i}" for i in range(ITEMS_PER_PAGE_DEFAULT)]
        assert total_pages == 2  # 12 items with page size 10 = 2 pages
        assert first_page["total_items"] == SAMPLE_RESULTS_COUNT_12

    def test_get_nth_page(self) -> None:
        """Test retrieving specific pages."""
        manager = SessionManager(items_per_page=ITEMS_PER_PAGE_DEFAULT, session_ttl_minutes=CLPMcpConstants.SESSION_TTL_MINUTES)
        results = [f"log_{i}" for i in range(SAMPLE_RESULTS_COUNT_25)]
        manager.get_or_create_session("test_session")
        manager.sessions["test_session"].ran_instructions = True  # Simulate that instructions were run

        # Cache results first
        manager.cache_query_result(
            session_id="test_session",
            results=results,
        )

        # Get second page (index 1)
        page_data = manager.get_nth_page("test_session", 1)
        assert "Error" not in page_data
        assert page_data["items"] == [f"log_{i}" for i in range(ITEMS_PER_PAGE_DEFAULT, 20)]
        assert page_data["page_number"] == PAGE_NUMBER_2

        # Test invalid page
        page_data = manager.get_nth_page("test_session", 10)
        assert "Error" in page_data
        assert "Page index is out of bounds." in page_data["Error"]

        # Test non-existent session - this will create a new session, so no error
        manager.get_or_create_session("non_existent")
        manager.sessions["non_existent"].ran_instructions = True  # Simulate that instructions were run
        page_data = manager.get_nth_page("non_existent", 0)
        assert "Error" in page_data
        assert "No previous paginated response in this session." in page_data["Error"]

    def test_session_expiration(self) -> None:
        """Test session expiration handling."""
        manager = SessionManager(items_per_page=ITEMS_PER_PAGE_DEFAULT, session_ttl_minutes=1)
        manager.get_or_create_session("test_session")
        manager.sessions["test_session"].ran_instructions = True  # Simulate that instructions were run


        session = manager.get_or_create_session("test_session")
        session.last_accessed = datetime.now(timezone.utc) - timedelta(minutes=2)

        # When we try to get the session again, it should be recreated (expired session is deleted)
        new_session = manager.get_or_create_session("test_session")
        manager.sessions["test_session"].ran_instructions = True  # Simulate that instructions were run
        assert new_session.session_id == "test_session"
        # The session should be newly created, so no cached query
        page_data = manager.get_nth_page("test_session", 0)
        assert "Error" in page_data
        assert "No previous paginated response in this session." in page_data["Error"]

    def test_no_cached_query(self) -> None:
        """Test handling when no query has been cached."""
        manager = SessionManager(items_per_page=ITEMS_PER_PAGE_DEFAULT, session_ttl_minutes=CLPMcpConstants.SESSION_TTL_MINUTES)
        manager.get_or_create_session("test_session")
        manager.sessions["test_session"].ran_instructions = True  # Simulate that instructions were run
        page_data = manager.get_nth_page("test_session", 0)
        assert "Error" in page_data
        assert "No previous paginated response in this session." in page_data["Error"]

    def test_cleanup_expired_sessions(self) -> None:
        """Test cleanup of expired sessions."""
        manager = SessionManager(items_per_page=ITEMS_PER_PAGE_DEFAULT, session_ttl_minutes=SESSION_TTL_30_MIN)

        # Create sessions with different ages
        session1 = manager.get_or_create_session("session1")
        session2 = manager.get_or_create_session("session2")
        manager.get_or_create_session("session3")  # Keep session3 but don't assign to variable

        # Make session1 and session2 expired
        session1.last_accessed = datetime.now(timezone.utc) - timedelta(minutes=31)
        session2.last_accessed = datetime.now(timezone.utc) - timedelta(minutes=35)

        removed_count = manager.cleanup_expired_sessions()

        assert removed_count == REMOVED_SESSIONS_COUNT
        assert "session1" not in manager.sessions
        assert "session2" not in manager.sessions
        assert "session3" in manager.sessions

    def test_custom_page_size(self) -> None:
        """Test SessionManager with custom page size."""
        manager = SessionManager(items_per_page=5, session_ttl_minutes=CLPMcpConstants.SESSION_TTL_MINUTES)
        manager.get_or_create_session("test_session")
        manager.sessions["test_session"].ran_instructions = True  # Simulate that instructions were run
        
        assert manager.items_per_page == 5
        results = [f"log_{i}" for i in range(SAMPLE_RESULTS_COUNT_12)]
        first_page, total_pages = manager.cache_query_result(
            session_id="test_session",
            results=results,
        )
        assert len(first_page["items"]) == 5
        assert total_pages == 3  # 12 items with items_per_page 5 = 3 pages
        # Check that session has the correct items_per_page
        session = manager.get_or_create_session("test_session")
        assert session.items_per_page == 5



if __name__ == "__main__":
    pytest.main([__file__, "-v"])

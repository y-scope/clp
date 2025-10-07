"""Tests for the SessionManager class."""

import time
from datetime import datetime, timedelta

import pytest

from clp_mcp_server.server.session_manager import (
    QueryResult,
    SessionManager,
    SessionState,
)


class TestQueryResult:
    """Test cases for QueryResult class."""
    
    def test_query_result_initialization(self):
        """Test QueryResult initialization and truncation."""
        # Create a result with more than max_cached_results
        large_results = [f"log_{i}" for i in range(1500)]
        query_result = QueryResult(
            timestamp=datetime.now(),
            total_results=large_results,
            max_cached_results=1000,
        )
        
        # Should be truncated to max_cached_results
        assert len(query_result.total_results) == 1000
        assert query_result.total_results[0] == "log_0"
        assert query_result.total_results[-1] == "log_999"
    
    def test_get_page(self):
        """Test pagination functionality."""
        results = [f"log_{i}" for i in range(25)]
        query_result = QueryResult(
            timestamp=datetime.now(),
            total_results=results,
            page_size=10,
        )
        
        # Test first page
        page1 = query_result.get_page(1)
        assert page1 is not None
        assert list(page1) == [f"log_{i}" for i in range(10)]
        assert page1.page == 1
        assert page1.page_count == 3
        
        # Test second page
        page2 = query_result.get_page(2)
        assert page2 is not None
        assert list(page2) == [f"log_{i}" for i in range(10, 20)]
        
        # Test last page
        page3 = query_result.get_page(3)
        assert page3 is not None
        assert list(page3) == [f"log_{i}" for i in range(20, 25)]
        
        # Test invalid page
        page4 = query_result.get_page(4)
        assert page4 is None
    
    def test_get_total_pages(self):
        """Test total pages calculation."""
        # Test with exact multiple
        query_result = QueryResult(
            timestamp=datetime.now(),
            total_results=["log"] * 20,
            page_size=10,
        )
        assert query_result.get_total_pages() == 2
        
        # Test with remainder
        query_result = QueryResult(
            timestamp=datetime.now(),
            total_results=["log"] * 25,
            page_size=10,
        )
        assert query_result.get_total_pages() == 3
        
        # Test with empty results
        query_result = QueryResult(
            timestamp=datetime.now(),
            total_results=[],
            page_size=10,
        )
        assert query_result.get_total_pages() == 0


class TestSessionState:
    """Test cases for SessionState class."""
    
    def test_session_initialization(self):
        """Test SessionState initialization."""
        session = SessionState(session_id="test_session")
        
        assert session.session_id == "test_session"
        assert session.current_query_result is None
        assert session.flags["ran_instructions"] is False
        assert isinstance(session.created_at, datetime)
        assert isinstance(session.last_accessed, datetime)
    
    def test_cache_query_result(self):
        """Test caching query results."""
        session = SessionState(session_id="test_session")
        results = [f"log_{i}" for i in range(50)]
        
        cached_result = session.cache_query_result(
            results=results,
            page_size=15,
        )
        
        assert session.current_query_result is not None
        assert len(session.current_query_result.total_results) == 50
        assert session.current_query_result.page_size == 15
    
    def test_get_page_data(self):
        """Test getting page data in dictionary format."""
        session = SessionState(session_id="test_session")
        results = [f"log_{i}" for i in range(25)]
        
        session.cache_query_result(
            results=results,
            page_size=10,
        )
        
        # Test first page (0-based index)
        page_data = session.get_page_data(0)
        assert page_data is not None
        assert page_data["items"] == [f"log_{i}" for i in range(10)]
        assert page_data["page_number"] == 1
        assert page_data["total_pages"] == 3
        assert page_data["total_items"] == 25
        assert page_data["items_per_page"] == 10
        assert page_data["has_next"] is True
        assert page_data["has_previous"] is False
        
        # Test second page
        page_data = session.get_page_data(1)
        assert page_data is not None
        assert page_data["items"] == [f"log_{i}" for i in range(10, 20)]
        assert page_data["page_number"] == 2
        assert page_data["has_next"] is True
        assert page_data["has_previous"] is True
        
        # Test last page
        page_data = session.get_page_data(2)
        assert page_data["items"] == [f"log_{i}" for i in range(20, 25)]
        assert page_data["has_next"] is False
        assert page_data["has_previous"] is True
        
        # Test with no cached result
        session.current_query_result = None
        assert session.get_page_data(0) is None
    
    def test_session_expiration(self):
        """Test session expiration check."""
        session = SessionState(session_id="test_session")
        
        # Test not expired
        assert session.is_expired(session_ttl_minutes=60) is False
        
        # Simulate old session
        session.last_accessed = datetime.now() - timedelta(minutes=61)
        assert session.is_expired(session_ttl_minutes=60) is True


class TestSessionManager:
    """Test cases for SessionManager class."""
    
    def test_get_or_create_session(self):
        """Test session creation and retrieval."""
        manager = SessionManager()
        
        # Create new session
        session1 = manager.get_or_create_session("session1")
        assert session1.session_id == "session1"
        assert "session1" in manager.sessions
        
        # Get existing session
        session1_again = manager.get_or_create_session("session1")
        assert session1_again is session1
    
    def test_cache_query_result(self):
        """Test caching query results through manager."""
        manager = SessionManager(page_size=5)
        results = [f"log_{i}" for i in range(12)]
        
        first_page, total_pages = manager.cache_query_result(
            session_id="test_session",
            results=results,
        )
        
        assert first_page["items"] == [f"log_{i}" for i in range(5)]
        assert total_pages == 3
        assert first_page["total_items"] == 12
    
    def test_get_nth_page(self):
        """Test retrieving specific pages."""
        manager = SessionManager(page_size=10)
        results = [f"log_{i}" for i in range(25)]
        
        # Cache results first
        manager.cache_query_result(
            session_id="test_session",
            results=results,
        )
        
        # Get second page (index 1)
        page_data = manager.get_nth_page("test_session", 1)
        assert "error" not in page_data
        assert page_data["items"] == [f"log_{i}" for i in range(10, 20)]
        assert page_data["page_number"] == 2
        
        # Test invalid page
        page_data = manager.get_nth_page("test_session", 10)
        assert "error" in page_data
        assert "Invalid page index" in page_data["error"]
        
        # Test non-existent session
        page_data = manager.get_nth_page("non_existent", 0)
        assert "error" in page_data
        assert "Session not found" in page_data["error"]
    
    def test_session_expiration(self):
        """Test session expiration handling."""
        manager = SessionManager(session_ttl_minutes=0.01)  # Very short TTL for testing
        
        session = manager.get_or_create_session("test_session")
        session.last_accessed = datetime.now() - timedelta(minutes=1)
        
        page_data = manager.get_nth_page("test_session", 0)
        assert "error" in page_data
        assert "Session expired" in page_data["error"]
        assert "test_session" not in manager.sessions
    
    def test_no_cached_query(self):
        """Test handling when no query has been cached."""
        manager = SessionManager()
        
        # Create session but don't cache any query
        manager.get_or_create_session("test_session")
        
        page_data = manager.get_nth_page("test_session", 0)
        assert "error" in page_data
        assert "No cached query results" in page_data["error"]
    
    def test_cleanup_expired_sessions(self):
        """Test cleanup of expired sessions."""
        manager = SessionManager(session_ttl_minutes=30)
        
        # Create sessions with different ages
        session1 = manager.get_or_create_session("session1")
        session2 = manager.get_or_create_session("session2")
        session3 = manager.get_or_create_session("session3")
        
        # Make session1 and session2 expired
        session1.last_accessed = datetime.now() - timedelta(minutes=31)
        session2.last_accessed = datetime.now() - timedelta(minutes=35)
        
        removed_count = manager.cleanup_expired_sessions()
        
        assert removed_count == 2
        assert "session1" not in manager.sessions
        assert "session2" not in manager.sessions
        assert "session3" in manager.sessions
    
    def test_get_session_info(self):
        """Test getting session information."""
        manager = SessionManager()
        results = [f"log_{i}" for i in range(15)]
        
        manager.cache_query_result(
            session_id="test_session",
            results=results,
        )
        
        info = manager.get_session_info("test_session")
        assert info is not None
        assert info["session_id"] == "test_session"
        assert info["has_cached_query"] is True
        assert info["total_results"] == 15
        assert info["total_pages"] == 2
        assert info["page_size"] == 10
        assert "flags" in info
        assert "created_at" in info
        assert "last_accessed" in info
        
        # Test non-existent session
        assert manager.get_session_info("non_existent") is None


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
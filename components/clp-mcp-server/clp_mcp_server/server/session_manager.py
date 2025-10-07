"""Session management for CLP MCP Server with pagination support."""

from dataclasses import dataclass, field
from datetime import datetime, timedelta
from typing import Any, Dict, List, Optional, Tuple

from paginate import Page


@dataclass
class QueryResult:
    """Represents a cached query result with metadata."""
    
    query: str
    timestamp: datetime
    total_results: List[str]  # List of log entries
    page_size: int = 10
    max_cached_results: int = 1000
    
    def __post_init__(self) -> None:
        """Validate and truncate results if necessary."""
        if len(self.total_results) > self.max_cached_results:
            self.total_results = self.total_results[:self.max_cached_results]
    
    def get_page(self, page_number: int) -> Optional[Page]:
        """
        Get a specific page from the cached results.
        
        :param page_number: 1-based page number
        :return: Page object or None if page number is invalid
        """
        if not self.total_results:
            return None
            
        try:
            return Page(
                self.total_results,
                page=page_number,
                items_per_page=self.page_size,
            )
        except (ValueError, IndexError):
            return None
    
    def get_total_pages(self) -> int:
        """Calculate the total number of pages."""
        if not self.total_results:
            return 0
        return (len(self.total_results) + self.page_size - 1) // self.page_size


@dataclass
class SessionState:
    """Represents the state of a single user session."""
    
    session_id: str
    created_at: datetime = field(default_factory=datetime.now)
    last_accessed: datetime = field(default_factory=datetime.now)
    current_query_result: Optional[QueryResult] = None
    flags: Dict[str, Any] = field(default_factory=dict)
    metadata: Dict[str, Any] = field(default_factory=dict)
    
    def __post_init__(self) -> None:
        """Initialize default flags if not provided."""
        if "ran_instructions" not in self.flags:
            self.flags["ran_instructions"] = False
    
    def update_access_time(self) -> None:
        """Update the last accessed timestamp."""
        self.last_accessed = datetime.now()
    
    def cache_query_result(
        self,
        query: str,
        results: List[str],
        page_size: int = 10,
        max_cached_results: int = 1000,
    ) -> QueryResult:
        """
        Cache a new query result for this session.
        
        :param query: The query string
        :param results: List of log entries
        :param page_size: Number of items per page
        :param max_cached_results: Maximum number of results to cache
        :return: The cached QueryResult object
        """
        self.current_query_result = QueryResult(
            query=query,
            timestamp=datetime.now(),
            total_results=results,
            page_size=page_size,
            max_cached_results=max_cached_results,
        )
        self.update_access_time()
        return self.current_query_result
    
    def get_page(self, page_index: int) -> Optional[Page]:
        """
        Get a specific page from the current cached query result.
        
        :param page_index: 0-based page index (will be converted to 1-based for paginate)
        :return: Page object or None if no cached result or invalid page
        """
        self.update_access_time()
        
        if not self.current_query_result:
            return None
        
        # Convert 0-based index to 1-based page number for paginate library
        page_number = page_index + 1
        return self.current_query_result.get_page(page_number)
    
    def get_page_data(self, page_index: int) -> Optional[Dict[str, Any]]:
        """
        Get page data in a dictionary format.
        
        :param page_index: 0-based page index
        :return: Dictionary with page data or None if unavailable
        """
        page = self.get_page(page_index)
        if not page:
            return None
        
        return {
            "items": list(page),
            "page_number": page.page,
            "total_pages": page.page_count,
            "total_items": page.item_count,
            "items_per_page": page.items_per_page,
            "has_next": page.next_page is not None,
            "has_previous": page.previous_page is not None,
        }
    
    def is_expired(self, session_ttl_minutes: int = 60) -> bool:
        """
        Check if the session has expired.
        
        :param session_ttl_minutes: Session time-to-live in minutes
        :return: True if expired, False otherwise
        """
        return datetime.now() - self.last_accessed > timedelta(minutes=session_ttl_minutes)


class SessionManager:
    """Manages multiple user sessions with pagination support."""
    
    def __init__(
        self,
        page_size: int = 10,
        max_cached_results: int = 1000,
        session_ttl_minutes: int = 60,
    ) -> None:
        """
        Initialize the session manager.
        
        :param page_size: Default number of items per page
        :param max_cached_results: Maximum number of results to cache per query
        :param session_ttl_minutes: Session expiration time in minutes
        """
        self.sessions: Dict[str, SessionState] = {}
        self.page_size = page_size
        self.max_cached_results = max_cached_results
        self.session_ttl_minutes = session_ttl_minutes
    
    def get_or_create_session(self, session_id: str) -> SessionState:
        """
        Get an existing session or create a new one.
        
        :param session_id: Unique session identifier
        :return: SessionState object
        """
        if session_id not in self.sessions:
            self.sessions[session_id] = SessionState(session_id=session_id)
        
        session = self.sessions[session_id]
        session.update_access_time()
        return session
    
    def cache_query_result(
        self,
        session_id: str,
        query: str,
        results: List[str],
        page_size: Optional[int] = None,
    ) -> Tuple[Dict[str, Any], int]:
        """
        Cache query results for a session and return the first page.
        
        :param session_id: Session identifier
        :param query: Query string
        :param results: List of log entries
        :param page_size: Optional page size override
        :return: Tuple of (first page data, total pages)
        """
        session = self.get_or_create_session(session_id)
        
        query_result = session.cache_query_result(
            query=query,
            results=results,
            page_size=page_size or self.page_size,
            max_cached_results=self.max_cached_results,
        )
        
        # Get the first page
        first_page_data = session.get_page_data(0)
        if not first_page_data:
            return {"items": [], "error": "No results found"}, 0
        
        return first_page_data, query_result.get_total_pages()
    
    def get_nth_page(self, session_id: str, page_index: int) -> Dict[str, Any]:
        """
        Retrieve a specific page from the cached query results.
        
        :param session_id: Session identifier
        :param page_index: 0-based page index
        :return: Dictionary with page data or error message
        """
        if session_id not in self.sessions:
            return {"error": "Session not found. Please run a query first."}
        
        session = self.sessions[session_id]
        
        # Check if session has expired
        if session.is_expired(self.session_ttl_minutes):
            del self.sessions[session_id]
            return {"error": "Session expired. Please run a new query."}
        
        # Check if there's a cached query result
        if not session.current_query_result:
            return {"error": "No cached query results. Please run a query first."}
        
        # Get the requested page
        page_data = session.get_page_data(page_index)
        if not page_data:
            total_pages = session.current_query_result.get_total_pages()
            return {
                "error": f"Invalid page index. Valid range: 0-{total_pages - 1}",
                "total_pages": total_pages,
            }
        
        return page_data
    
    def get_session_info(self, session_id: str) -> Optional[Dict[str, Any]]:
        """
        Get information about a session.
        
        :param session_id: Session identifier
        :return: Session information or None if not found
        """
        if session_id not in self.sessions:
            return None
        
        session = self.sessions[session_id]
        info = {
            "session_id": session_id,
            "created_at": session.created_at.isoformat(),
            "last_accessed": session.last_accessed.isoformat(),
            "has_cached_query": session.current_query_result is not None,
            "flags": session.flags,
            "metadata": session.metadata,
        }
        
        if session.current_query_result:
            info.update({
                "cached_query": session.current_query_result.query,
                "query_timestamp": session.current_query_result.timestamp.isoformat(),
                "total_results": len(session.current_query_result.total_results),
                "total_pages": session.current_query_result.get_total_pages(),
                "page_size": session.current_query_result.page_size,
            })
        
        return info
    
    def cleanup_expired_sessions(self) -> int:
        """
        Remove expired sessions from memory.
        
        :return: Number of sessions removed
        """
        expired_sessions = [
            sid for sid, session in self.sessions.items()
            if session.is_expired(self.session_ttl_minutes)
        ]
        
        for sid in expired_sessions:
            del self.sessions[sid]
        
        return len(expired_sessions)
"""Session management for CLP MCP Server with pagination support."""

import threading
from dataclasses import dataclass, field
from datetime import datetime, timedelta, timezone
from typing import Any
import time

from paginate import Page

from .constants import CLPMcpConstants


@dataclass(frozen=True)
class QueryResult:
    """Cached results from previous query's response."""

    total_results: list[str]
    items_per_page: int

    _total_pages: int = field(init=False, repr=False)

    def __post_init__(self) -> None:
        """
        Validate that the number of log entries in the cached response is up to MAX_CACHED_RESULTS.
        """
        if len(self.total_results) > CLPMcpConstants.MAX_CACHED_RESULTS:
            err_msg = (
                f"QueryResult exceeds maximum allowed cached results: "
                f"{len(self.total_results)} > {CLPMcpConstants.MAX_CACHED_RESULTS}. "
            )
            raise ValueError(err_msg)

        object.__setattr__(
            self, '_total_pages',
            (len(self.total_results) + self.items_per_page - 1)
            // self.items_per_page
        )

    def get_page(self, page_number: int) -> Page | None:
        """
        Get a specific page from the cached response.

        :param page_number: One-based indexing, e.g., 1 for the first page
        :return: Page object or None if page number is out of bounds
        """
        if page_number > self.total_pages or page_number <= 0:
            return None

        return Page(
            self.total_results,
            page=page_number,
            items_per_page=self.items_per_page,
        )

    @property
    def total_pages(self) -> int:
        """:return: Total number of pages."""
        return self._total_pages


@dataclass
class SessionState:
    """States of a single user session."""

    session_id: str
    items_per_page: int
    session_ttl_minutes: int
    last_accessed: datetime = field(default_factory=lambda: datetime.now(timezone.utc))
    cached_query_result: QueryResult | None = None
    ran_instructions: bool = False

    def cache_query_result(
        self,
        results: list[str],
    ) -> None:
        """
        Caches the lastest query result of the session.

        :param results: List of log entries.
        """
        self.cached_query_result = QueryResult(
            total_results=results,
            items_per_page=self.items_per_page
        )

    def get_page_data(self, page_number: int) -> dict[str, Any]:
        """
        Get page data in a dictionary format.

        :param page_number: One-based indexing, e.g., 1 for the first page
        :return: Dictionary with page data or None if unavailable
        """
        if self.cached_query_result is None:
            return {
                "Error": "No previous paginated response in this session."
            }

        page = self.cached_query_result.get_page(page_number)
        if page is None:
            return { "Error": "Page index is out of bounds."}

        return {
            "items": list(page),
            "total_pages": page.page_count,
            "total_items": page.item_count,
            "items_per_page": page.items_per_page,
            "has_next": page.next_page is not None,
            "has_previous": page.previous_page is not None,
        }

    def is_expired(self) -> bool:
        """
        :return: whether the session has expired.
        """
        time_diff = datetime.now(timezone.utc) - self.last_accessed
        return time_diff > timedelta(minutes=self.session_ttl_minutes)
    
    def update_access_time(self) -> None:
        """Update the last accessed timestamp."""
        self.last_accessed = datetime.now(timezone.utc)


class SessionManager:
    """Session manager for all user sessions."""

    def __init__(
        self,
        session_ttl_minutes: int
    ):
        """
        Initializes the SessionManager.
        
        :param session_ttl_minutes:
        """
        self.session_ttl_minutes = session_ttl_minutes
        # sessions is a shared variable as there may be multiple session attached to the MCP server
        # session state is NOT a shared variable because each session is accessed by only one
        # connection at a time, and all calls are synchronous.
        self._sessions_lock = threading.Lock()
        self.sessions: dict[str, SessionState] = {}
        self._cleanup_thread = threading.Thread(target=self._cleanup_loop, daemon=True)
        self._cleanup_thread.start()

    def _cleanup_loop(self) -> None:
        """Background thread to periodically clean up expired sessions."""
        while True:
            time.sleep(CLPMcpConstants.CLEAN_UP_SECONDS)
            self.cleanup_expired_sessions()

    def cleanup_expired_sessions(self) -> None:
        """Cleanup all expired sessions."""
        with self._sessions_lock:
            expired_sessions = [
                sid for sid, session in self.sessions.items()
                if session.is_expired()
            ]

            for sid in expired_sessions:
                del self.sessions[sid]

    def get_or_create_session(self, session_id: str) -> SessionState:
        """
        Gets an existing session or creates a new one.
        
        :param session_id: Unique identifier for the session
        :return: The SessionState object for the given session_id
        """
        with self._sessions_lock:
            if session_id in self.sessions and self.sessions[session_id].is_expired():
                del self.sessions[session_id]
            
            if session_id not in self.sessions:
                self.sessions[session_id] = SessionState(
                    session_id, CLPMcpConstants.ITEM_PER_PAGE, self.session_ttl_minutes
                )
            
            session = self.sessions[session_id]

            session.update_access_time()
            return session

    def cache_query_result(self, session_id: str, results: list[str]) -> dict[str, Any]:
        """
        Caches query results for a session and return the first page.
        
        :param session_id: Unique identifier for the session
        :param results: List of log entries to cache
        :return: Tuple of (first page data as dict, total number of pages)
        """
        session = self.get_or_create_session(session_id)
        if session.ran_instructions is False:
            return {
                "Error": "Please call get_instructions() first to understand how to use this MCP server."
            }

        session.cache_query_result(results=results)

        return session.get_page_data(1)

    def get_nth_page(self, session_id: str, page_index: int) -> dict[str, Any]:
        """
        Retrieves the n-th page of a paginated response from the previous query.
        
        :param session_id: Unique identifier for the session
        :param page_index: Zero-based index, e.g., 0 for the first page
        :return: On success, dictionary containing paged log entrie and pagination metadata. 
        On error, dictionary with ``{"Error": "error message describing the failure"}``.
        """
        session = self.get_or_create_session(session_id)
        if session.ran_instructions is False:
            return { 
                "Error": "Please call get_instructions() first to understand how to use this MCP server."
            }

        page_number = page_index + 1  # Convert zero-based to one-based
        return session.get_page_data(page_number)
        

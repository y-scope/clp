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

        :param page_number: 1-based indexing, e.g., 1 for the first page
        :return: Page object or None if failed to get the page
        """
        if self.total_results is None:
            return None

        if page_number > self.total_pages or page_number <= 0:
            return None

        try:
            return Page(
                self.total_results,
                page=page_number,
                items_per_page=self.items_per_page,
            )
        except (ValueError, IndexError):
            return None

    @property
    def total_pages(self) -> int:
        """:return: Total number of pages."""
        return self._total_pages


@dataclass
class SessionState:
    """Represents the state of a single user session."""

    session_id: str
    items_per_page: int
    session_ttl_minutes: int
    last_accessed: datetime = field(default_factory=lambda: datetime.now(timezone.utc))
    current_query_result: QueryResult | None = None
    ran_instructions: bool = False

    def update_access_time(self) -> None:
        """Update the last accessed timestamp."""
        self.last_accessed = datetime.now(timezone.utc)

    def cache_query_result(
        self,
        results: list[str],
    ) -> QueryResult:
        """
        Cache a new query result for this session.

        :param results: List of log entries
        :return: The cached QueryResult object
        """
        self.current_query_result = QueryResult(
            total_results=results,
            items_per_page=self.items_per_page
        )
        return self.current_query_result

    def get_page_data(self, page_index: int) -> dict[str, Any] | None:
        """
        Get page data in a dictionary format.

        :param page_index: 0-based page index
        :return: Dictionary with page data or None if unavailable
        """
        if not self.current_query_result:
            return None

        # Convert 0-based index to 1-based page number for paginate library
        page_number = page_index + 1
        page = self.current_query_result.get_page(page_number)

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

    def is_expired(self) -> bool:
        """
        Check if the session has expired.

        :return: True if expired, False otherwise
        """
        time_diff = datetime.now(timezone.utc) - self.last_accessed
        return time_diff > timedelta(minutes=self.session_ttl_minutes)


class SessionManager:
    """Session manager for handling multiple user sessions."""

    def __init__(
        self,
        items_per_page: int,
        session_ttl_minutes: int
    ):
        """
        Initialize the SessionManager.
        
        :param page_size: Number of items per page (defaults to CLPMcpConstants.PAGE_SIZE)
        :param session_ttl_minutes: Session TTL in minutes (defaults to CLPMcpConstants.SESSION_TTL_MINUTES)
        """
        self.items_per_page = items_per_page
        self.session_ttl_minutes = session_ttl_minutes
        self._sessions_lock = threading.Lock()
        self.sessions: dict[str, SessionState] = {}
        self._cleanup_thread = threading.Thread(target=self._cleanup_loop, daemon=True)
        self._cleanup_thread.start()

    def _cleanup_loop(self) -> None:
        while True:
            time.sleep(CLPMcpConstants.CLEAN_UP_SECONDS)
            self.cleanup_expired_sessions()

    def get_or_create_session(self, session_id: str) -> SessionState:
        """
        Get an existing session or create a new one (thread-safe).
        
        Multiple requests can call this simultaneously, so we need
        to protect the dict operations.
        """
        with self._sessions_lock:
            if session_id in self.sessions and self.sessions[session_id].is_expired():
                del self.sessions[session_id]
            
            if session_id not in self.sessions:
                self.sessions[session_id] = SessionState(
                    session_id, self.items_per_page, self.session_ttl_minutes
                )
            
            session = self.sessions[session_id]

            session.update_access_time()
            return session

    def cache_query_result(
        self,
        session_id: str,
        results: list[str],
        items_per_page: int | None = None,
    ) -> tuple[dict[str, Any], int]:
        """
        Cache query results for a session and return the first page.
        
        No lock needed after getting session - session object operations
        are safe (synchronous per session_id).
        """
        session = self.get_or_create_session(session_id)

        # Session operations don't need lock - synchronous per session
        query_result = session.cache_query_result(results=results)

        first_page_data = session.get_page_data(0)
        if not first_page_data:
            return {"items": [], "error": "No results found"}, 0

        return first_page_data, query_result.get_total_pages()

    def get_nth_page(self, session_id: str, page_index: int) -> dict[str, Any]:
        """
        Retrieve a specific page from the cached query results.
        
        Locks when accessing dict, then uses session outside lock.
        """
        session = self.get_or_create_session(session_id)

        if not session.current_query_result:
            return {"error": "No cached query results. Please run a query first."}

        page_data = session.get_page_data(page_index)
        if not page_data:
            total_pages = session.current_query_result.get_total_pages()
            return {
                "error": f"Invalid page index. Valid range: 0-{total_pages - 1}",
                "total_pages": total_pages,
            }

        return page_data

    def cleanup_expired_sessions(self) -> int:
        """
        Cleanup all expired sessions.
        
        Can be called from cron job, admin endpoint, or health check.
        Thread-safe with lock.
        """
        with self._sessions_lock:
            expired_sessions = [
                sid for sid, session in self.sessions.items()
                if session.is_expired()
            ]

            for sid in expired_sessions:
                del self.sessions[sid]

            return len(expired_sessions)

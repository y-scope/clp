"""Session management for CLP MCP Server with pagination support."""

import threading
from dataclasses import dataclass, field
from datetime import datetime, timedelta, timezone
from typing import Any

from paginate import Page

from .constants import CLPMcpConstants


@dataclass
class QueryResult:
    """Cached results from previous query's response."""

    total_results: list[str]
    total_pages: int = field(init=False)

    def __post_init__(self) -> None:
        """
        Truncate results if they exceed MAX_CACHED_RESULTS.
        """
        if len(self.total_results) > CLPMcpConstants.MAX_CACHED_RESULTS:
            self.total_results = self.total_results[:CLPMcpConstants.MAX_CACHED_RESULTS]

        self.total_pages = (
            (len(self.total_results) + CLPMcpConstants.PAGE_SIZE - 1)
            // CLPMcpConstants.PAGE_SIZE
        )

    def get_page(self, page_number: int) -> Page | None:
        """
        Get a specific page from the cached results.

        :param page_number: 1-based page number
        :return: Page object or None if page number is invalid
        """
        if not self.total_results:
            return None

        if page_number > self.total_pages or page_number <= 0:
            return None

        try:
            return Page(
                self.total_results,
                page=page_number,
                items_per_page=CLPMcpConstants.PAGE_SIZE,
            )
        except (ValueError, IndexError):
            return None

    def get_total_pages(self) -> int:
        """Get the total number of pages."""
        return self.total_pages


@dataclass
class SessionState:
    """Represents the state of a single user session."""

    session_id: str
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
        self.current_query_result = QueryResult(total_results=results)
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

    def is_expired(self, session_ttl_minutes: int = 60) -> bool:
        """
        Check if the session has expired.

        :param session_ttl_minutes: Session time-to-live in minutes
        :return: True if expired, False otherwise
        """
        time_diff = datetime.now(timezone.utc) - self.last_accessed
        return time_diff > timedelta(minutes=session_ttl_minutes)


@dataclass
class SessionManager:
    """
    Session manager with thread-safe dict access.
    
    Thread-safety: Lock protects the sessions dict from concurrent
    modifications by multiple request handlers.
    """

    sessions: dict[str, SessionState] = field(default_factory=dict)
    session_ttl_minutes: int = 60

    # Lock to protect sessions dict
    _lock: threading.Lock = field(
        default_factory=threading.Lock, init=False, repr=False
    )

    # Timer for periodic cleanup
    _cleanup_timer: threading.Timer | None = field(
        default=None, init=False, repr=False
    )
    _cleanup_interval_seconds: int = field(default=600, init=False)  # 10 minutes

    def __post_init__(self) -> None:
        """Initialize the cleanup timer."""
        self.start_cleanup_timer()

    def __del__(self) -> None:
        """Cleanup timer when SessionManager is destroyed."""
        self.stop_cleanup_timer()

    def start_cleanup_timer(self) -> None:
        """Start the periodic cleanup timer (thread-safe)."""
        with self._lock:
            if self._cleanup_timer is not None:
                self._cleanup_timer.cancel()

            self._cleanup_timer = threading.Timer(
                self._cleanup_interval_seconds,
                self._periodic_cleanup
            )
            self._cleanup_timer.daemon = True  # Don't prevent program exit
            self._cleanup_timer.start()

    def stop_cleanup_timer(self) -> None:
        """Stop the periodic cleanup timer (thread-safe)."""
        with self._lock:
            if self._cleanup_timer is not None:
                self._cleanup_timer.cancel()
                self._cleanup_timer = None

    def shutdown(self) -> None:
        """
        Graceful shutdown of the SessionManager.
        
        Call this method when your application is shutting down to ensure
        proper cleanup of resources.
        """
        self.stop_cleanup_timer()
        # Perform final cleanup of expired sessions
        self.cleanup_all_expired()

    def _periodic_cleanup(self) -> None:
        """
        Internal method for periodic cleanup.
        
        This runs in a separate timer thread and schedules the next cleanup.
        """
        try:
            expired_count = self.cleanup_all_expired()
            # Log cleanup results if needed (optional)
            # print(f"Cleaned up {expired_count} expired sessions")
        except Exception:
            # Silently handle any cleanup errors to prevent timer thread crashes
            pass
        finally:
            # Schedule next cleanup (only if not shutting down)
            with self._lock:
                if self._cleanup_timer is not None:  # Check if we're still active
                    self.start_cleanup_timer()

    def get_or_create_session(self, session_id: str) -> SessionState:
        """
        Get an existing session or create a new one (thread-safe).
        
        Multiple requests can call this simultaneously, so we need
        to protect the dict operations.
        """
        with self._lock:
            if session_id in self.sessions:
                session = self.sessions[session_id]
                if session.is_expired(self.session_ttl_minutes):
                    del self.sessions[session_id]

            # Create new session if doesn't exist
            if session_id not in self.sessions:
                self.sessions[session_id] = SessionState(session_id=session_id)

            session = self.sessions[session_id]

        session.update_access_time()
        return session

    def cache_query_result(
        self,
        session_id: str,
        results: list[str],
        page_size: int | None = None,
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

        # Have session reference, safe to use outside lock
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

    def cleanup_all_expired(self) -> int:
        """
        Cleanup all expired sessions.
        
        Can be called from cron job, admin endpoint, or health check.
        Thread-safe with lock.
        """
        with self._lock:
            expired_sessions = [
                sid for sid, session in self.sessions.items()
                if session.is_expired(self.session_ttl_minutes)
            ]

            for sid in expired_sessions:
                del self.sessions[sid]

            return len(expired_sessions)

    def cleanup_expired_sessions(self) -> int:
        """
        Cleanup expired sessions (alias for cleanup_all_expired for backward compatibility).
        
        :return: Number of sessions removed
        """
        return self.cleanup_all_expired()

    def get_session_info(self, session_id: str) -> dict[str, Any] | None:
        """
        Get information about a session.
        
        :param session_id: The session ID to get info for
        :return: Dictionary with session info or None if session doesn't exist
        """
        with self._lock:
            if session_id not in self.sessions:
                return None
            
            session = self.sessions[session_id]
        
        # Build session info outside the lock
        info = {
            "session_id": session.session_id,
            "has_cached_query": session.current_query_result is not None,
            "ran_instructions": session.ran_instructions,
            "created_at": session.last_accessed.isoformat(),  # Using last_accessed as created_at since we don't have created_at field
            "last_accessed": session.last_accessed.isoformat(),
        }
        
        if session.current_query_result:
            info.update({
                "total_results": len(session.current_query_result.total_results),
                "total_pages": session.current_query_result.get_total_pages(),
                "page_size": CLPMcpConstants.PAGE_SIZE,
            })
        
        return info

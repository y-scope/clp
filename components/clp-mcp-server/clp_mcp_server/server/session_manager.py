"""Session management for CLP MCP Server."""

import threading
import time
from dataclasses import dataclass, field
from datetime import datetime, timedelta, timezone
from typing import Any

from paginate import Page

from .constants import CLPMcpConstants


@dataclass(frozen=True)
class QueryResult:
    """Cached results from the previous query's response."""

    cached_response: list[str]
    num_items_per_page: int

    _total_pages: int = field(init=False, repr=False)

    def __post_init__(self) -> None:
        """
        Validates that cached results don't exceed MAX_CACHED_RESULTS.

        :raise: ValueError if the number of cached results or num_items_per_page is invalid.
        """
        if len(self.cached_response) > CLPMcpConstants.MAX_CACHED_RESULTS:
            err_msg = (
                f"QueryResult exceeds maximum allowed cached results:"
                f" {len(self.cached_response)} > {CLPMcpConstants.MAX_CACHED_RESULTS}."
            )
            raise ValueError(err_msg)

        if self.num_items_per_page <= 0:
            err_msg = (
                f"Invalid num_items_per_page: {self.num_items_per_page}, it must be a positive integer. "
            )
            raise ValueError(err_msg)

        object.__setattr__(
            self,
            "_total_pages",
            (len(self.cached_response) + self.num_items_per_page - 1) // self.num_items_per_page,
        )

    def get_page(self, page_number: int) -> Page | None:
        """
        Gets a specific page from the cached response.

        :param page_number: One-based indexing, e.g., 1 for the first page.
        :return: Page object or None if page number is out of bounds.
        """
        if page_number > self._total_pages or page_number <= 0:
            return None

        return Page(
            self.cached_response,
            page=page_number,
            items_per_page=self.num_items_per_page,
        )


@dataclass
class SessionState:
    """State of a single user session."""

    session_id: str
    num_items_per_page: int
    session_ttl_minutes: int
    last_accessed: datetime = field(default_factory=lambda: datetime.now(timezone.utc))
    cached_query_result: QueryResult | None = None
    ran_instructions: bool = False

    def cache_query_result(
        self,
        results: list[str],
    ) -> None:
        """
        Caches the latest query result of the session.

        :param results: Complete log entries from previous query for caching.
        """
        self.cached_query_result = QueryResult(
            cached_response=results, num_items_per_page=self.num_items_per_page
        )

    def get_page_data(self, page_number: int) -> dict[str, Any]:
        """
        Gets page data and its metadata in a dictionary format.

        :param page_number: One-based indexing, e.g., 1 for the first page.
        :return: On success, dictionary containing paged log entries and the paging metadata.
        On error, dictionary with ``{"Error": "error message describing the failure"}``.
        """
        if self.cached_query_result is None:
            return {"Error": "No previous paginated response in this session."}

        page = self.cached_query_result.get_page(page_number)
        if page is None:
            return {"Error": "Page index is out of bounds."}

        return {
            "items": list(page),
            "total_pages": page.page_count,
            "total_items": page.item_count,
            "num_items_per_page": page.items_per_page,
            "has_next": page.next_page is not None,
            "has_previous": page.previous_page is not None,
        }

    def is_expired(self) -> bool:
        """:return: Whether the session has expired."""
        time_diff = datetime.now(timezone.utc) - self.last_accessed
        return time_diff > timedelta(minutes=self.session_ttl_minutes)

    def update_access_time(self) -> None:
        """Updates the last accessed timestamp."""
        self.last_accessed = datetime.now(timezone.utc)


class SessionManager:
    """Session manager for concurrent user sessions."""

    def __init__(self, session_ttl_minutes: int) -> None:
        """
        Initializes the SessionManager and starts background cleanup thread.

        :param session_ttl_minutes: Session time-to-live in minutes.
        """
        self.session_ttl_minutes = session_ttl_minutes
        """
        MCP Server Concurrency Model:
        The server supports multiple concurrent clients, where each client only makes synchronous
        API calls. This assumptions leads to the following design decisions:
        sessions is a shared variable as there may be multiple session attached to the MCP server
        session state is NOT a shared variable because each session is accessed by only one
        connection at a time because API calls for a single session are synchronous.
        """
        self._sessions_lock = threading.Lock()
        self.sessions: dict[str, SessionState] = {}
        self._cleanup_thread = threading.Thread(target=self._cleanup_loop, daemon=True)
        self._cleanup_thread.start()

    def _cleanup_loop(self) -> None:
        """Cleans up all expired sessions periodically in a separate cleanup thread."""
        while True:
            time.sleep(CLPMcpConstants.EXPEIRED_SESSION_SWEEP_INTERVAL_SECONDS)
            self.cleanup_expired_sessions()

    def cleanup_expired_sessions(self) -> None:
        """Cleans up all expired sessions."""
        with self._sessions_lock:
            expired_sessions = [
                sid for sid, session in self.sessions.items() if session.is_expired()
            ]

            for sid in expired_sessions:
                del self.sessions[sid]

    def get_or_create_session(self, session_id: str) -> SessionState:
        """
        Gets an existing session or creates a new one.

        :param session_id: Unique identifier for the session.
        :return: The SessionState object for the given session_id.
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

    def cache_query_result(self, session_id: str, query_results: list[str]) -> dict[str, Any]:
        """
        Caches query results for a session and returns the first page and the paging metadata.

        :param session_id: Unique identifier for the session.
        :param query_results: Complete log entries from previous query for caching.
        :return: On success, dictionary containing the first page of log entries and
        pagination metadata. On error, dictionary with
        ``{"Error": "error message describing the failure"}``.
        """
        session = self.get_or_create_session(session_id)
        if session.ran_instructions is False:
            return {
                "Error": "Please call get_instructions() first "
                "to understand how to use this MCP server."
            }

        session.cache_query_result(results=query_results)

        return session.get_page_data(1)

    def get_nth_page(self, session_id: str, page_index: int) -> dict[str, Any]:
        """
        Retrieves the n-th page of a paginated response with the paging metadata from the previous
        query.

        :param session_id: Unique identifier for the session.
        :param page_index: Zero-based index, e.g., 0 for the first page.
        :return: Forwards `SessionState.get_page_data`'s return values.
        """
        session = self.get_or_create_session(session_id)
        if session.ran_instructions is False:
            return {
                "Error": "Please call get_instructions() first "
                "to understand how to use this MCP server."
            }

        page_number = page_index + 1  # Convert zero-based to one-based
        return session.get_page_data(page_number)

"""Session management for CLP MCP Server."""

import asyncio
from dataclasses import dataclass, field
from datetime import datetime, timedelta, timezone
from typing import Any, ClassVar

from paginate import Page

from . import constants


class PaginatedQueryResult:
    """Paginates the cached log entries returned from a query's response."""

    def __init__(self, result_log_entries: list[str], num_items_per_page: int) -> None:
        """
        :param result_log_entries: List of cached log entries to paginate.
        :param num_items_per_page:
        :raise: ValueError if the number of cached results or num_items_per_page is invalid.
        """
        if len(result_log_entries) > constants.MAX_CACHED_RESULTS:
            err_msg = (
                "PaginatedQueryResult exceeds maximum allowed cached results:"
                f" {len(result_log_entries)} > {constants.MAX_CACHED_RESULTS}."
            )
            raise ValueError(err_msg)

        if num_items_per_page <= 0:
            err_msg = (
                f"Invalid num_items_per_page: {num_items_per_page}, it must be a positive integer."
            )
            raise ValueError(err_msg)

        self.result_log_entries = result_log_entries
        self.num_items_per_page = num_items_per_page

        self._num_pages = (len(result_log_entries) + num_items_per_page - 1) // num_items_per_page

    def get_page(self, page_index: int) -> Page | None:
        """
        :param page_index: The number of page to retrieve (zero-based index; 0 is the first page).
        :return: A `Page` object for the specified page.
        :return: None if `page_index` is out of bounds.
        """
        # Convert zero-based to one-based
        page_number = page_index + 1
        if page_number <= 0 or self._num_pages < page_number:
            return None

        return Page(
            self.result_log_entries,
            page=page_number,
            items_per_page=self.num_items_per_page,
        )


@dataclass
class SessionState:
    """
    Represents the state of a user session.
    `SessionState` respects `FastMCP` asynchronous concurrency model that is built on Python's
    asyncio runtime:
    Asyncio achieves concurrency on a single thread by allowing tasks to yield control at `await`
    points. This means when a `mcp.tool` API calls executes an await expression, it gets suspended,
    and the event loop executes the next `mcp.tool` API calls from received requests issued by
    clients.
    Therefore, `SessionState` does not need thread-safe operations because a session cannot be
    accessed from two threads at the same time.
    """

    session_id: str
    _num_items_per_page: int
    _session_ttl_minutes: int

    is_instructions_retrieved: bool = False
    last_accessed: datetime = field(default_factory=lambda: datetime.now(timezone.utc))
    _cached_query_result: PaginatedQueryResult | None = None

    def cache_query_result(
        self,
        results: list[str],
    ) -> None:
        """
        Caches the latest query result of the session.

        :param results: Complete log entries from previous query for caching.
        """
        self._cached_query_result = PaginatedQueryResult(
            result_log_entries=results, num_items_per_page=self._num_items_per_page
        )

    def get_page_data(self, page_index: int) -> dict[str, Any]:
        """
        Retrieves the n-th page of a paginated response with the paging metadata from the previous
        query.

        NOTE: This docstring must be synchronized with `get_nth_page`'s MCP tool call.

        :param page_index: Zero-based index, e.g., 0 for the first page.
        :param ctx: The `FastMCP` context containing the metadata of the underlying MCP session.
        :return: A dictionary containing the following key-value pairs on success:
            - "item": A list of log entries in the requested page.
            - "total_pages": Total number of pages available from the query as an integer.
            - "total_items": Total number of log entries available from the query as an integer.
            - "num_items_per_page": Number of log entries per page.
            - "has_next": Whether a page exists after the returned one.
            - "has_previous": Whether a page exists before the returned one.
        :return: A dictionary with the following key-value pair on failures:
            - "Error": An error message describing the failure.
        """
        if self._cached_query_result is None:
            return {"Error": "No previous paginated response in this session."}

        page = self._cached_query_result.get_page(page_index)
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
        return time_diff > timedelta(minutes=self._session_ttl_minutes)

    def update_access_time(self) -> None:
        """Updates the last accessed timestamp."""
        self.last_accessed = datetime.now(timezone.utc)


class SessionManager:
    """
    Session manager for concurrent user sessions.
    `SessionManager` respects `FastMCP` asynchronous concurrency model that is built on Python's
    asyncio runtime:
    Asyncio achieves concurrency on a single thread by allowing tasks to yield control at `await`
    points. This means when a `mcp.tool` API calls executes an await expression, it gets suspended,
    and the event loop executes the next `mcp.tool` API calls from received requests issued by
    clients.
    The mutations of the shared variable `sessions` are performed by an asynchronous task. The
    above concurrency model guarantees that the mutations of `sessions` are atomic because a session
    cannot be accessed from two threads at the same time.
    """

    _GET_INSTRUCTIONS_NOT_RUN_ERROR: ClassVar[dict[str, str]] = {
        "Error": "Please call get_instructions() first to understand how to use this MCP server."
    }

    def __init__(self, session_ttl_minutes: int) -> None:
        """:param session_ttl_minutes: Session time-to-live in minutes."""
        self.sessions: dict[str, SessionState] = {}
        self._session_ttl_minutes = session_ttl_minutes
        self._cleanup_task: asyncio.Task | None = None

    async def start(self) -> None:
        """Starts the asynchronous cleanup task."""
        if self._cleanup_task is None:
            self._cleanup_task = asyncio.create_task(self._cleanup_loop())

    async def _cleanup_loop(self) -> None:
        """Cleans up all expired sessions periodically as an async task."""
        while True:
            await asyncio.sleep(constants.EXPIRED_SESSION_SWEEP_INTERVAL_SECONDS)
            self.cleanup_expired_sessions()

    def cleanup_expired_sessions(self) -> None:
        """Cleans up all expired sessions."""
        expired_sessions = [sid for sid, session in self.sessions.items() if session.is_expired()]

        for sid in expired_sessions:
            del self.sessions[sid]

    def cache_query_result(self, session_id: str, query_results: list[str]) -> dict[str, Any]:
        """
        Caches query results for a session and returns the first page and the paging metadata.

        :param session_id: Unique identifier for the session.
        :param query_results: Complete log entries from previous query for caching.
        :return: Dictionary containing the first page log entries and the paging metadata if the
            first page can be retrieved.
        :return: Dictionary with ``{"Error": "error message describing the failure"}`` if fails to
            retrieve the first page.
        """
        session = self.get_or_create_session(session_id)
        if session.is_instructions_retrieved is False:
            return self._GET_INSTRUCTIONS_NOT_RUN_ERROR.copy()

        session.cache_query_result(results=query_results)

        return session.get_page_data(0)

    def get_nth_page(self, session_id: str, page_index: int) -> dict[str, Any]:
        """
        Retrieves the n-th page of a paginated response with the paging metadata from the previous
        query.

        :param session_id: Unique identifier for the session.
        :param page_index: The number of page to retrieve (zero-based index; 0 is the first page).
        :return: Forwards `SessionState.get_page_data`'s return values.
        """
        session = self.get_or_create_session(session_id)
        if session.is_instructions_retrieved is False:
            return self._GET_INSTRUCTIONS_NOT_RUN_ERROR.copy()

        return session.get_page_data(page_index)

    def get_or_create_session(self, session_id: str) -> SessionState:
        """
        Gets an existing session or creates a new one.

        :param session_id: Unique identifier for the session.
        :return: The SessionState object for the given session_id.
        """
        if session_id in self.sessions and self.sessions[session_id].is_expired():
            del self.sessions[session_id]

        if session_id not in self.sessions:
            self.sessions[session_id] = SessionState(
                session_id, constants.NUM_ITEMS_PER_PAGE, self._session_ttl_minutes
            )

        session = self.sessions[session_id]

        session.update_access_time()
        return session

"""Session management for CLP MCP Server."""

import asyncio
from dataclasses import dataclass, field
from datetime import datetime, timedelta, timezone
from typing import Any, ClassVar

from paginate import Page

from clp_mcp_server.server import constants


class PaginatedQueryResult:
    """Paginates the cached log entries returned from a query's response."""

    def __init__(self, log_entries: list[str], num_items_per_page: int) -> None:
        """
        :param log_entries: Log entries to paginate.
        :param num_items_per_page:
        :raise: ValueError if the number of cached results or `num_items_per_page` is invalid.
        """
        if len(log_entries) > constants.MAX_CACHED_RESULTS:
            err_msg = (
                "PaginatedQueryResult exceeds maximum allowed cached results:"
                f" {len(log_entries)} > {constants.MAX_CACHED_RESULTS}."
            )
            raise ValueError(err_msg)

        if num_items_per_page <= 0:
            err_msg = (
                f"Invalid num_items_per_page: {num_items_per_page}, it must be a positive integer."
            )
            raise ValueError(err_msg)

        self._num_items_per_page: int = num_items_per_page
        self._num_pages: int = (len(log_entries) + num_items_per_page - 1) // num_items_per_page
        self._log_entries: list[str] = log_entries

    def get_page(self, page_index: int) -> Page | None:
        """
        :param page_index: Zero-based index, e.g., 0 for the first page.
        :return: A `Page` object for the specified page.
        :return: None if `page_index` is out of bounds.
        """
        # Convert zero-based to one-based
        page_number = page_index + 1
        if page_number <= 0 or self._num_pages < page_number:
            return None

        return Page(
            self._log_entries,
            page=page_number,
            items_per_page=self._num_items_per_page,
        )


@dataclass
class SessionState:
    """
    Represents the state of a user session, following the same concurrency model as
    `SessionManager`.

    NOTE: All methods are intended to be executed exclusively by coroutines within the same event
    loop in a single-threaded context.
    """

    _num_items_per_page: int
    _session_id: str
    _session_ttl_seconds: float

    _cached_query_result: PaginatedQueryResult | None = None
    _last_accessed: datetime = field(default_factory=lambda: datetime.now(timezone.utc))
    _is_instructions_retrieved: bool = False

    _GET_INSTRUCTIONS_NOT_RUN_ERROR: ClassVar[dict[str, str]] = {
        "Error": "Please call `get_instructions()` first to understand how to use this MCP server."
    }

    def cache_query_result_and_get_first_page(
        self,
        results: list[str],
    ) -> dict[str, Any]:
        """
        :param results: Log entries from the query to cache.
        :return: Forwards `SessionState.get_page_data`'s return values.
        :return: A dictionary with the following key-value pair on failures:
            - "Error": An error message describing the failure.
        """
        if self._is_instructions_retrieved is False:
            return self._GET_INSTRUCTIONS_NOT_RUN_ERROR.copy()

        if len(results) == 0:
            return {"Error": "No log events found matching the KQL query."}

        self._cached_query_result = PaginatedQueryResult(
            log_entries=results, num_items_per_page=self._num_items_per_page
        )

        return self.get_page_data(0)

    def get_instructions(self) -> str:
        """
        Gets a pre-defined "system prompt" that guides the LLM behavior.

        :return: A string of "system prompt".
        """
        self._is_instructions_retrieved = True
        return constants.SYSTEM_PROMPT

    def get_page_data(self, page_index: int) -> dict[str, Any]:
        """
        Retrieves the n-th page of a paginated response with the paging metadata from the previous
        query.

        NOTE: This docstring must be synchronized with MCP tool calls: `get_nth_page`,
        `search_by_kql`, and `search_by_kql_with_timestamp_range`.


        :param page_index: Zero-based index, e.g., 0 for the first page.
        :return: A dictionary containing the following key-value pairs on success:
            - "items": A list of log entries in the requested page.
            - "num_total_pages": Total number of pages available from the query as an integer.
            - "num_total_items": Total number of log entries available from the query as an integer.
            - "num_items_per_page": Number of log entries per page.
            - "has_next": Whether a page exists after the returned one.
            - "has_previous": Whether a page exists before the returned one.
        :return: A dictionary with the following key-value pair on failures:
            - "Error": An error message describing the failure.
        :return: _GET_INSTRUCTIONS_NOT_RUN_ERROR if `get_instructions` has not been called in this
            session.
        """
        if self._is_instructions_retrieved is False:
            return self._GET_INSTRUCTIONS_NOT_RUN_ERROR.copy()

        if self._cached_query_result is None:
            return {"Error": "No previous paginated response in this session."}

        page = self._cached_query_result.get_page(page_index)
        if page is None:
            return {"Error": "Page index is out of bounds."}

        return {
            "items": list(page),
            "num_total_pages": page.page_count,
            "num_total_items": page.item_count,
            "num_items_per_page": page.items_per_page,
            "has_next": page.next_page is not None,
            "has_previous": page.previous_page is not None,
        }

    def is_expired(self) -> bool:
        """:return: Whether the session has expired."""
        time_diff = datetime.now(timezone.utc) - self._last_accessed
        return time_diff > timedelta(seconds=self._session_ttl_seconds)

    def update_access_time(self) -> None:
        """Updates the last accessed timestamp."""
        self._last_accessed = datetime.now(timezone.utc)


class SessionManager:
    """
    Manages concurrent user sessions, following the same concurrency model as `FastMCP`.

    All data structures managed by this class are accessed exclusively by coroutines running
    within the same event loop in a single-thread context. This design implies:

    - No thread-level concurrency control (e.g., `threading.Lock`) is necessary, since operations
      occur in a single-threaded async context.
    - Async synchronization primitives (e.g., `asyncio.Lock`) are only necessary when an `await`
      occurs inside a critical section, to prevent interleaving of coroutine execution.
    """

    def __init__(self, session_ttl_seconds: float) -> None:
        """:param session_ttl_seconds: Session time-to-live in seconds."""
        self.sessions: dict[str, SessionState] = {}
        self._session_ttl_seconds: float = session_ttl_seconds
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

    def cache_query_result_and_get_first_page(
        self, session_id: str, query_results: list[str]
    ) -> dict[str, Any]:
        """
        :param session_id:
        :param query_results: Log entries from the query to cache.
        :return: Forwards `SessionState.cache_query_result_and_get_first_page`'s return values.
        """
        session = self.get_or_create_session(session_id)

        return session.cache_query_result_and_get_first_page(results=query_results)

    def get_nth_page(self, session_id: str, page_index: int) -> dict[str, Any]:
        """
        Retrieves the n-th page of a paginated response with the paging metadata from the previous
        query.

        :param session_id:
        :param page_index: Index of the page to retrieve (zero-based, e.g., 0 is the first page).
        :return: Forwards `SessionState.get_page_data`'s return values.
        """
        session = self.get_or_create_session(session_id)

        return session.get_page_data(page_index)

    def get_or_create_session(self, session_id: str) -> SessionState:
        """
        Gets an existing session or creates a new one.

        :param session_id:
        :return: The `SessionState` object for the given `session_id`.
        """
        if session_id in self.sessions and self.sessions[session_id].is_expired():
            del self.sessions[session_id]

        if session_id not in self.sessions:
            self.sessions[session_id] = SessionState(
                constants.NUM_ITEMS_PER_PAGE, session_id, self._session_ttl_seconds
            )

        session = self.sessions[session_id]

        session.update_access_time()
        return session

"""MCP Server implementation."""

from typing import Any

from clp_py_utils.clp_config import ClpConfig
from fastmcp import Context, FastMCP
from starlette.requests import Request
from starlette.responses import PlainTextResponse

from clp_mcp_server.clp_connector import ClpConnector
from clp_mcp_server.server import constants
from clp_mcp_server.server.session_manager import SessionManager
from clp_mcp_server.server.utils import (
    format_query_results,
    parse_timestamp_range,
    sort_by_timestamp,
)


def create_mcp_server(clp_config: ClpConfig) -> FastMCP:
    """
    Creates and defines API tool calls for the CLP MCP server.

    :param clp_config:
    :return: A configured `FastMCP` instance.
    :raise: Propagates `FastMCP.__init__`'s exceptions.
    :raise: Propagates `FastMCP.tool`'s exceptions.
    """
    mcp = FastMCP(name=constants.SERVER_NAME)

    session_manager = SessionManager(session_ttl_seconds=constants.SESSION_TTL_SECONDS)

    connector = ClpConnector(clp_config)

    async def _execute_kql_query(
        session_id: str,
        kql_query: str,
        begin_ts: int | None = None,
        end_ts: int | None = None,
    ) -> dict[str, Any]:
        """
        Executes a KQL query search with optional timestamp range and returns paginated results.

        :param session_id:
        :param kql_query:
        :param begin_ts: The beginning of the time range (inclusive).
        :param end_ts: The end of the time range (inclusive).
        :return: Forwards `SessionManager.cache_query_result_and_get_first_page`'s return values on
            success.
        :return: A dictionary with the following key-value pair on failures:
            - "Error": An error message describing the failure.
        """
        try:
            query_id = await connector.submit_query(kql_query, begin_ts, end_ts)
            await connector.wait_query_completion(query_id)
            results = await connector.read_results(query_id)
        except (ValueError, RuntimeError, TimeoutError) as e:
            return {"Error": str(e)}

        sorted_results = sort_by_timestamp(results)
        formatted_results = format_query_results(sorted_results)
        return session_manager.cache_query_result_and_get_first_page(session_id, formatted_results)

    @mcp.tool
    async def get_instructions(ctx: Context) -> str:
        """
        Gets a pre-defined "system prompt" that guides the LLM behavior.
        This function must be invoked before any other `FastMCP.tool`.

        :param ctx: The `FastMCP` context containing the metadata of the underlying MCP session.
        :return: A string of "system prompt".
        """
        await session_manager.start()
        return session_manager.get_or_create_session(ctx.session_id).get_instructions()

    @mcp.tool
    async def get_nth_page(page_index: int, ctx: Context) -> dict[str, Any]:
        """
        Retrieves the n-th page of a paginated response with the paging metadata from the previous
        query.

        :param page_index: Zero-based index, e.g., 0 for the first page.
        :param ctx: The `FastMCP` context containing the metadata of the underlying MCP session.
        :return: A dictionary containing the following key-value pairs on success:
            - "items": A list of log entries in the requested page.
            - "num_total_pages": Total number of pages available from the query as an integer.
            - "num_total_items": Total number of log entries available from the query as an integer.
            - "num_items_per_page": Number of log entries per page.
            - "has_next": Whether a page exists after the returned one.
            - "has_previous": Whether a page exists before the returned one.
        :return: A dictionary with the following key-value pair on failures:
            - "Error": An error message describing the failure.
        """
        await session_manager.start()
        return session_manager.get_nth_page(ctx.session_id, page_index)

    @mcp.tool
    def hello_world(name: str = "clp-mcp-server user") -> dict[str, Any]:
        """
        Provides a simple hello world greeting.

        :param name:
        :return: A greeting message to the given `name`.
        """
        return {
            "message": f"Hello World, {name.strip()}!",
            "server": constants.SERVER_NAME,
            "status": "running",
        }

    @mcp.tool
    async def search_by_kql(kql_query: str, ctx: Context) -> dict[str, Any]:
        """
        Searches log events that match the given Kibana Query Language (KQL) query. The resulting
        events are ordered by timestamp in descending order (latest to oldest), cached for
        subsequent pagination, and returned with the first page of results.

        :param kql_query:
        :param ctx: The `FastMCP` context containing the metadata of the underlying MCP session.
        :return: A dictionary containing the following key-value pairs on success:
            - "items": A list of log entries in the requested page.
            - "num_total_pages": Total number of pages available from the query as an integer.
            - "num_total_items": Total number of log entries available from the query as an integer.
            - "num_items_per_page": Number of log entries per page.
            - "has_next": Whether a page exists after the returned one.
            - "has_previous": Whether a page exists before the returned one.
        :return: A dictionary with the following key-value pair on failures:
            - "Error": An error message describing the failure.
        """
        await session_manager.start()

        return await _execute_kql_query(ctx.session_id, kql_query)

    @mcp.tool
    async def search_by_kql_with_timestamp_range(
        kql_query: str, formatted_begin_timestamp: str, formatted_end_timestamp: str, ctx: Context
    ) -> dict[str, Any]:
        """
        Searches log events that match the given Kibana Query Language (KQL) query within the given
        time range. Timestamps must follow the ISO 8601 UTC format (`YYYY-MM-DDTHH:mm:ss.fffZ`),
        where the trailing `Z` indicates UTC. Timestamps that do not follow this format will be
        rejected.

        :param kql_query:
        :param formatted_begin_timestamp: The beginning of the time range (inclusive).
        :param formatted_end_timestamp: The end of the time range (inclusive).
        :param ctx: The `FastMCP` context containing the metadata of the underlying MCP session.
        :return: A dictionary containing the following key-value pairs on success:
            - "items": A list of log entries in the requested page.
            - "num_total_pages": Total number of pages available from the query as an integer.
            - "num_total_items": Total number of log entries available from the query as an integer.
            - "num_items_per_page": Number of log entries per page.
            - "has_next": Whether a page exists after the returned one.
            - "has_previous": Whether a page exists before the returned one.
        :return: A dictionary with the following key-value pair on failures:
            - "Error": An error message describing the failure.
        """
        await session_manager.start()

        try:
            begin_ts, end_ts = parse_timestamp_range(
                formatted_begin_timestamp, formatted_end_timestamp
            )
        except ValueError as e:
            return {"Error": str(e)}

        return await _execute_kql_query(ctx.session_id, kql_query, begin_ts, end_ts)

    @mcp.custom_route("/health", methods=["GET"])
    async def health_check(_request: Request) -> PlainTextResponse:
        """
        Health check endpoint.

        :param _request: An HTTP request object.
        :return: A plain text response indicating server is healthy.
        """
        return PlainTextResponse("OK")

    return mcp

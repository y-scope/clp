"""MCP Server implementation."""

from typing import Any

from clp_py_utils.clp_config import CLPConfig
from fastmcp import Context, FastMCP

from clp_mcp_server.clp_connector import ClpConnector

from . import constants
from .session_manager import SessionManager
from .utils import format_query_results, sort_by_timestamp


def create_mcp_server(clp_config: CLPConfig) -> FastMCP:
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

        try:
            query_id = await connector.submit_query(kql_query)
            await connector.wait_query_completion(query_id)
            results = await connector.read_results(query_id)
        except (ValueError, RuntimeError, TimeoutError) as e:
            return {"Error": str(e)}

        sorted_results = sort_by_timestamp(results)
        formatted_results = format_query_results(sorted_results)
        return session_manager.cache_query_result_and_get_first_page(
            ctx.session_id, formatted_results
        )


    @mcp.tool
    async def search_by_kql_with_timestamp(
        kql_query: str, begin_timestamp: str, end_timestamp: str, ctx: Context
    ) -> dict[str, object]:
        """
        Performs `FastMCP.tool.search_kql_query`'s search query with a given timestamp range. The
        timestamps must be ISO 8601 formatted date strings with up to millisecond precision
        (YYYY-MM-DDTHH:mm:ss.fffZ).

        :param kql_query:
        :param begin_timestamp:
        :param end_timestamp:
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
            begin_epoch, end_epoch = parse_timestamp_range(begin_timestamp, end_timestamp)

            query_id = await connector.submit_query(kql_query, begin_epoch, end_epoch)
            await connector.wait_query_completion(query_id)
            results = await connector.read_results(query_id)
        except (ValueError, RuntimeError, TimeoutError) as e:
            return {"Error": str(e)}

        sorted_results = sort_query_results(results)
        filtered_results = filter_query_results(sorted_results)
        return session_manager.cache_query_result_and_get_first_page(
            ctx.session_id, filtered_results
        )

    return mcp

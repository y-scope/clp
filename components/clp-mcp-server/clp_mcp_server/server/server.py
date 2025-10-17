"""MCP Server implementation."""

from types import SimpleNamespace
from typing import Any

from fastmcp import Context, FastMCP

from clp_mcp_server.clp_connector import ClpConnector

from . import constants
from .session_manager import SessionManager
from .utils import clean_query_results


def create_mcp_server() -> FastMCP:
    """
    Creates and defines API tool calls for the CLP MCP server.

    :return: A configured `FastMCP` instance.
    :raise: Propagates `FastMCP.__init__`'s exceptions.
    :raise: Propagates `FastMCP.tool`'s exceptions.
    """
    mcp = FastMCP(name=constants.SERVER_NAME)

    session_manager = SessionManager(session_ttl_seconds=constants.SESSION_TTL_SECONDS)

    clp_config = SimpleNamespace(
        results_cache=SimpleNamespace(
            host="results-cache", port=27017, db_name="clp-query-results"
        ),
        database=SimpleNamespace(host="database", port=3306, name="clp-db"),
    )
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
    async def search_kql_query(kql_query: str, ctx: Context) -> dict[str, object]:
        """
        Searches the logs for the specified query and returns the latest 10 log messages matching
        the query, i.e. the first page of the complete results.

        :param kql_query:
        :param ctx: The `FastMCP` context containing the metadata of the underlying MCP session.
        :return: Forwards `FastMCP.tool`''s `get_nth_page`'s return values on success:
        :return: A dictionary with the following key-value pair on failures:
            - "Error": An error status indicating the reason of the query failure.
        """
        await session_manager.start()

        try:
            query_id = await connector.submit_query(kql_query)
            await connector.wait_query_completion(query_id)
            results = await connector.read_results(query_id)

            cleaned_results = clean_query_results(results)
            return session_manager.cache_query_result_and_get_first_page(
                ctx.session_id, cleaned_results
            )
        except Exception as e:
            return {"Error": str(e)}

    return mcp

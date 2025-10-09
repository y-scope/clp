"""MCP Server implementation."""

from datetime import datetime
from typing import Any

from fastmcp import Context, FastMCP

from .constants import CLPMcpConstants
from .session_manager import SessionManager


def create_mcp_server() -> FastMCP:
    """
    Creates and defines API tool calls for the CLP MCP server.

    :return: A configured `FastMCP` instance.
    :raise: Propagates `FastMCP.__init__`'s exceptions.
    :raise: Propagates `FastMCP.tool`'s exceptions.
    """
    mcp = FastMCP(name=CLPMcpConstants.SERVER_NAME)

    session_manager = SessionManager(CLPMcpConstants.PAGE_SIZE, CLPMcpConstants.SESSION_TTL_MINUTES)

    def validate_session_initialized(session_id: str) -> str | None:
        """
        Validates that get_instructions has been called once per session.

        :param session_id: The session identifier
        :return: Error message if session hasn't been initialized, None otherwise
        """
        session = session_manager.get_or_create_session(session_id)
        if not session.ran_instructions:
            return "Please call get_instructions() first to understand how to use this MCP server."
        return None

    @mcp.tool
    def get_instructions(ctx: Context) -> str:
        """
        Provides instructions on how to use this MCP server.

        Returns:
            instructions: A string of instructions on how to use this MCP server.

        """
        session = session_manager.get_or_create_session(ctx.session_id)
        session.ran_instructions = True
        return CLPMcpConstants.SYSTEM_PROMPT

    @mcp.tool
    def search_kql_query(kql_query: str, ctx: Context) -> dict[str, object]:
        """
        Search the logs for the specified query and returns the latest 10 log messages
        matching the query (first page of results) and the total number of pages available

        Args:
         kql_query: KQL (Kabana Query Language) query to find matching log messages
         ctx: The MCP context containing session information

        Returns:
         A dictionary with two keys:
          - FirstPage: The latest 10 log messages matching the query.
          - NumPages: Total number of pages that can be indexed

        """
        error_msg = validate_session_initialized(ctx.session_id)
        if error_msg:
            return {"error": error_msg}

        # TODO: Replace with actual CLP log search implementation
        queried_logs = [
            f"Log entry {i}: Query '{kql_query}' matched this log message"
            for i in range(25)  # Simulate 25 results (3 pages)
        ]

        first_page_data, total_pages = session_manager.cache_query_result(
            session_id=ctx.session_id,
            results=queried_logs,
        )

        return {
            "FirstPage": first_page_data.get("items", []),
            "NumPages": total_pages,
            "TotalResults": first_page_data.get("total_items", 0),
        }

    @mcp.tool
    def search_kql_query_with_timestamp(
        kql_query: str,
        begin_timestamp: datetime,
        end_timestamp: datetime,
        ctx: Context
    ) -> dict[str, object]:
        """
        Search the logs for the specified query and returns the latest 10 log messages
        matching the query (first page of results) and the total number of pages available

        Args:
         kql_query: KQL (Kabana Query Language) query to find matching log messages
         begin_timestamp: Log messages with timestamp before begin_timestamp will not be matched
         end_timestamp: Log messages with timestamp after end_timestamp will not be matched.
         ctx: The MCP context containing session information

        Returns:
         A dictionary with keys:
          - FirstPage: The latest 10 log messages matching the query and timestamp range
          - NumPages: Total number of pages that can be indexed
          - TotalResults: Total number of matching results

        """
        error_msg = validate_session_initialized(ctx.session_id)
        if error_msg:
            return {"error": error_msg}

        # TODO: Replace with actual log search implementation with timestamp filtering
        queried_logs = [
            f"Log entry {i}: Query '{kql_query}' matched between "
            f"{begin_timestamp.isoformat()} and {end_timestamp.isoformat()}"
            for i in range(35)  # Simulate 35 results (4 pages)
        ]

        first_page_data, total_pages = session_manager.cache_query_result(
            session_id=ctx.session_id,
            results=queried_logs,
        )

        return {
            "FirstPage": first_page_data.get("items", []),
            "NumPages": total_pages,
            "TotalResults": first_page_data.get("total_items", 0),
        }

    @mcp.tool
    def get_nth_page(page_index: int, ctx: Context) -> dict[str, Any]:
        """
        CLP:
        Retrieve the last paginated response by index.

        Args:
            page_index: The zero-based index of the page to retrieve.
            ctx: The MCP context containing session information

        Returns:
            The part of the response at page index, or an error message if unavailable.

        """
        error_msg = validate_session_initialized(ctx.session_id)
        if error_msg:
            return {"error": error_msg}

        page_data = session_manager.get_nth_page(ctx.session_id, page_index)

        if "error" in page_data:
            return page_data

        return {
            "Page": page_data.get("items", []),
            "PageNumber": page_data.get("page_number", 0),
            "TotalPages": page_data.get("total_pages", 0),
            "HasNext": page_data.get("has_next", False),
            "HasPrevious": page_data.get("has_previous", False),
        }

    @mcp.tool
    def hello_world(name: str = "clp-mcp-server user") -> dict[str, Any]:
        """
        Provides a simple hello world greeting.

        :param name:
        :return: A greeting message to the given `name`.
        """
        return {
            "message": f"Hello World, {name.strip()}!",
            "server": CLPMcpConstants.SERVER_NAME,
            "status": "running",
        }

    return mcp

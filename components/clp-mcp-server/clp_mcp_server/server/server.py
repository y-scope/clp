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

    session_manager = SessionManager(
        CLPMcpConstants.ITEM_PER_PAGE, 
        CLPMcpConstants.SESSION_TTL_MINUTES
    )

    @mcp.tool
    def get_instructions(ctx: Context) -> str:
        """
        Provides instructions on how to use this MCP server.
        
        :param ctx: The MCP context containing session information
        :return: A string of instructions on how to use this MCP server.
        """
        session = session_manager.get_or_create_session(ctx.session_id)
        session.ran_instructions = True
        return CLPMcpConstants.SYSTEM_PROMPT

    @mcp.tool
    def get_nth_page(page_index: int, ctx: Context) -> dict[str, Any]:
        """
        Retrieve the last paginated response by index.

        :param page_index: Zero-based index, e.g., 0 for the first page
        :param ctx: The MCP context containing session information
        :return: The part of the response at page index, or an error message if unavailable.
        """
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
            "server": CLPMcpConstants.SERVER_NAME,
            "status": "running",
        }

    return mcp

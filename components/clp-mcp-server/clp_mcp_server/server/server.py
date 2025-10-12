"""MCP Server implementation."""

from typing import Any

from fastmcp import Context, FastMCP

from . import constants
from .session_manager import SessionManager


def create_mcp_server() -> FastMCP:
    """
    Creates and defines API tool calls for the CLP MCP server.

    :return: A configured `FastMCP` instance.
    :raise: Propagates `FastMCP.__init__`'s exceptions.
    :raise: Propagates `FastMCP.tool`'s exceptions.
    """
    mcp = FastMCP(name=constants.SERVER_NAME)

    session_manager = SessionManager(constants.SESSION_TTL_MINUTES)

    @mcp.tool
    def get_instructions(ctx: Context) -> str:
        """
        Gets a pre-defined “system prompt” that guides the LLM behavior.
        This function must be invoked before any other `FastMCP.tool`.

        :param ctx: The `FastMCP` context containing the metadata of the underlying MCP session.
        :return: A string of “system prompt”.
        """
        session = session_manager.get_or_create_session(ctx.session_id)
        session.is_instructions_retrieved = True
        return constants.SYSTEM_PROMPT

    @mcp.tool
    def get_nth_page(page_index: int, ctx: Context) -> dict[str, Any]:
        """
        Retrieves the n-th page of a paginated response with the paging metadata from the previous
        query.

        :param page_index: Zero-based index, e.g., 0 for the first page.
        :param ctx: The `FastMCP` context containing the metadata of the underlying MCP session.
        :return: Dictionary containing paged log entries and the paging metadata if the
        page `page_index` can be retrieved.
        :return: Dictionary with ``{"Error": "error message describing the failure"}`` if fails to
        retrieve page `page_index`.
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
            "server": constants.SERVER_NAME,
            "status": "running",
        }

    return mcp

"""Minimal MCP Server Implementation for CLP."""

from typing import Any

from fastmcp import FastMCP


class CLPMCPServerConfig:
    """Configuration constants for the CLP MCP Server."""

    SERVER_NAME = "clp-mcp-server"

    # Tool names
    TOOL_HELLO_WORLD = "hello_world"
    TOOL_GET_SERVER_INFO = "get_server_info"

    @classmethod
    def get_capabilities(cls) -> list[str]:
        """Returns a list of capabilities (tool names) supported by the server."""
        return [cls.TOOL_HELLO_WORLD, cls.TOOL_GET_SERVER_INFO]


def create_mcp_server(**settings: Any) -> FastMCP:
    """
    Creates and defines API tool calls for CLP MCP server.

    :param settings: Additional settings for the MCP server configuration.
    :return: A configured FastMCP instance ready to run.
    """
    mcp = FastMCP(name=CLPMCPServerConfig.SERVER_NAME, **settings)

    @mcp.tool()
    def get_server_info() -> dict[str, Any]:
        """
        Gets basic information about the MCP server.

        :return: The server information with a list of capabilities.
        """
        return {
            "name": CLPMCPServerConfig.SERVER_NAME,
            "capabilities": CLPMCPServerConfig.get_capabilities(),
            "status": "running",
        }

    @mcp.tool()
    def hello_world(name: str = "clp-mcp-server user") -> dict[str, Any]:
        """
        Provides a simple hello world greeting.

        :param name: The name to greet.
        :return: A greeting message with metadata.
        """
        return {
            "message": f"Hello World, {name.strip()}!",
            "server": CLPMCPServerConfig.SERVER_NAME,
            "status": "running",
        }

    return mcp

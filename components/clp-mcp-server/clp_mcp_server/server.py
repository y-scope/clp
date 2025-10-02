"""
Minimal MCP Server implementation for CLP.
"""

from typing import Any, Dict

from fastmcp import FastMCP


class CLPMCPServerConfig:
    """Configuration constants for the CLP MCP Server."""

    SERVER_NAME = "clp-mcp-server"

    # Tool names
    TOOL_HELLO_WORLD = "hello_world"
    TOOL_GET_SERVER_INFO = "get_server_info"

    # Status constants
    STATUS_SUCCESS = "success"
    STATUS_ERROR = "error"
    STATUS_RUNNING = "running"

    @classmethod
    def get_capabilities(cls) -> list[str]:
        """Get list of available tool capabilities."""
        return [cls.TOOL_HELLO_WORLD, cls.TOOL_GET_SERVER_INFO]


def CLPMcpServer(**settings: Any) -> FastMCP:
    """
    Create and configure the CLP MCP Server.

    Args:
        **settings: Additional settings for the MCP server

    Returns:
        A configured FastMCP instance
    """
    config = CLPMCPServerConfig()

    # Initialize the MCP server with basic metadata
    mcp = FastMCP(name=config.SERVER_NAME, **settings)

    @mcp.tool()
    async def hello_world(name: str = "clp-mcp-server user") -> Dict[str, Any]:
        """
        A simple hello world function.

        Args:
            name: The name to greet

        Returns:
            A greeting message with metadata
        """
        return {
            "message": f"Hello World, {name.strip()}!",
            "server": config.SERVER_NAME,
            "status": config.STATUS_SUCCESS,
        }

    @mcp.tool()
    async def get_server_info() -> Dict[str, Any]:
        """
        Get basic information about the MCP server.

        Returns:
            Server information including version and capabilities
        """

        return {
            "name": config.SERVER_NAME,
            "capabilities": config.get_capabilities(),
            "status": config.STATUS_RUNNING,
        }

    return mcp

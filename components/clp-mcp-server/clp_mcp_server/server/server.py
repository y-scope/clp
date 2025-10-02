"""Minimal MCP Server implementation for CLP."""

from enum import Enum
from typing import Any

from fastmcp import FastMCP


class CLPMCPServerConfig:
    """Configuration constants for the CLP MCP Server."""

    SERVER_NAME = "clp-mcp-server"

    # Tool names
    TOOL_HELLO_WORLD = "hello_world"
    TOOL_GET_SERVER_INFO = "get_server_info"

    class Status(Enum):
        SUCCESS = "success"
        ERROR = "error"
        RUNNING = "running"

    @classmethod
    def get_capabilities(cls) -> list[str]:
        """Get list of available tool capabilities."""
        return [cls.TOOL_HELLO_WORLD, cls.TOOL_GET_SERVER_INFO]


def create_mcp_server(**settings: Any) -> FastMCP:
    """
    Creates and defines API tool calls for CLP MCP server.

    :param settings: Additional settings for the MCP server configuration.
    :return: A configured FastMCP instance ready to run.
    :raise: Any exceptions from FastMCP initialization.
    """
    config = CLPMCPServerConfig()

    # Initialize the MCP server with basic metadata
    mcp = FastMCP(name=config.SERVER_NAME, **settings)

    @mcp.tool()
    def get_server_info() -> dict[str, Any]:
        """
        Gets basic information about the MCP server.

        :return: Server information including version and capabilities.
        """
        return {
            "name": config.SERVER_NAME,
            "capabilities": config.get_capabilities(),
            "status": config.Status.RUNNING.value,
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
            "server": config.SERVER_NAME,
            "status": config.Status.SUCCESS.value,
        }

    return mcp

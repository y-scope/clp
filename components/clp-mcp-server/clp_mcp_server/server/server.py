"""MCP Server implementation."""

from typing import Any

from fastmcp import FastMCP
from starlette.requests import Request
from starlette.responses import JSONResponse


class ProtocolConstant:
    """Constants for the CLP MCP Server."""

    SERVER_NAME = "clp-mcp-server"

    # Tool names
    TOOL_HELLO_WORLD = "hello_world"
    TOOL_GET_SERVER_INFO = "get_server_info"

    @classmethod
    def get_capabilities(cls) -> list[str]:
        """
        Gets the capabilities of the server.
        :return: A list of tool names supported by the server.
        """
        return [cls.TOOL_HELLO_WORLD, cls.TOOL_GET_SERVER_INFO]


def create_mcp_server() -> FastMCP:
    """
    Creates and defines API tool calls for the CLP MCP server.

    :return: A configured `FastMCP` instance.
    :raise: Propagates `FastMCP.__init__`'s exceptions.
    :raise: Propagates `FastMCP.tool`'s exceptions.
    """
    mcp = FastMCP(name=ProtocolConstant.SERVER_NAME)

    @mcp.custom_route("/health", methods=["GET"])
    async def health_check(_request: Request) -> JSONResponse:
        """Health check endpoint.
        
        :arg _request: The incoming request object.
        :return: A JSON response indicating the health status of the server.
        """
        health = {
            "status": "running",
            "service": ProtocolConstant.SERVER_NAME
        }

        return JSONResponse(health)


    @mcp.tool()
    def get_server_info() -> dict[str, Any]:
        """
        Gets the MCP server's information.

        :return: The server's information with a list of capabilities.
        """
        return {
            "name": ProtocolConstant.SERVER_NAME,
            "capabilities": ProtocolConstant.get_capabilities(),
            "status": "running",
        }

    @mcp.tool()
    def hello_world(name: str = "clp-mcp-server user") -> dict[str, Any]:
        """
        Provides a simple hello world greeting.

        :param name:
        :return: A greeting message to the given `name`.
        """
        return {
            "message": f"Hello World, {name.strip()}!",
            "server": ProtocolConstant.SERVER_NAME,
            "status": "running",
        }

    return mcp

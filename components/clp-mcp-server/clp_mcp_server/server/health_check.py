from fastmcp import FastMCP
from starlette.requests import Request
from starlette.responses import JSONResponse

from .server import ProtocolConstant

def add_health_check(mcp: FastMCP) -> None:
    """
    Adds a health check endpoint to the MCP server.

    :param mcp: The FastMCP instance to add the health check to.
    """
    @mcp.custom_route("/health", methods=["GET"])
    async def health_check(_request: Request) -> JSONResponse:
        """Health check endpoint for Docker/monitoring."""
        health = {
            "status": "healthy",
            "service": ProtocolConstant.SERVER_NAME
        }

        return JSONResponse(health)
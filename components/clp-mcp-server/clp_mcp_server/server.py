"""
Minimal MCP Server implementation for CLP.
"""

from typing import Any, Dict
from fastmcp import FastMCP
from starlette.requests import Request
from starlette.responses import JSONResponse


def CLPMcpServer(**settings: Any) -> FastMCP:
    """
    Create and configure the CLP MCP Server.
    
    Args:
        **settings: Additional settings for the MCP server
        
    Returns:
        A configured FastMCP instance
    """
    # Initialize the MCP server with basic metadata
    mcp = FastMCP(
        name="clp-mcp-server",
        **settings
    )
    
    @mcp.custom_route("/health", methods=["GET"])
    async def health_check(request: Request) -> JSONResponse:
        """
        Health check endpoint for monitoring and load balancers.
        
        Returns:
            JSON response indicating server health status
        """
        return JSONResponse({
            "status": "healthy",
            "service": "clp-mcp-server",
            "version": "0.1.0",
            "timestamp": request.headers.get("date", "unknown")
        })

    @mcp.tool()
    async def hello_world(name: str = "World") -> Dict[str, Any]:
        """
        A simple hello world function.
        
        Args:
            name: The name to greet (default: "World")
            
        Returns:
            A greeting message with metadata
        """
        return {
            "message": f"Hello, {name}!",
            "server": "clp-mcp-server",
            "status": "success"
        }
    
    @mcp.tool()
    async def get_server_info() -> Dict[str, Any]:
        """
        Get basic information about the MCP server.
        
        Returns:
            Server information including version and capabilities
        """
        return {
            "name": "clp-mcp-server",
            "version": "0.1.0",
            "capabilities": ["hello_world", "get_server_info"],
            "status": "running"
        }
    
    return mcp
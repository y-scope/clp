"""CLP MCP Server package."""

import ipaddress
import logging
import socket
import sys

import click

from .server.server import create_mcp_server


@click.command()
@click.option(
    "--host", type=str, default="127.0.0.1", help="The server's host address (default: 127.0.0.1)."
)
@click.option("--port", type=int, default=8000, help="The server's port number (default: 8000).")
def main(host: str, port: int) -> None:
    """Run the CLP MCP Server with HTTP transport."""
    # Configure logging
    logging.basicConfig(
        level=logging.INFO, format="%(asctime)s - %(name)s - %(levelname)s - %(message)s"
    )
    logger = logging.getLogger(__name__)

    # Validate host and port
    if not host.strip():
        logger.error("Host cannot be empty")
        sys.exit(1)

    # Validate host format (IP address or resolvable hostname)
    try:
        # Try to parse as IP address
        ipaddress.ip_address(host)
    except ValueError:
        # If not an IP, try to resolve as hostname
        try:
            socket.gethostbyname(host)
        except OSError:
            logger.exception(
                "Invalid host: %s is neither a valid IP address nor a resolvable hostname", host
            )
            sys.exit(1)

    max_port = 65535
    if port <= 0 or port > max_port:
        logger.error("Port must be between 1 and %d, got: %d", max_port, port)
        sys.exit(1)

    try:
        # Create the MCP server instance
        mcp = create_mcp_server()

        logger.info("Starting CLP MCP Server on %s:%d", host, port)

        # Run the server with HTTP transport
        mcp.run(transport="streamable-http", host=host, port=port)

    except (OSError, RuntimeError):
        logger.exception("Failed to start MCP server")
        sys.exit(1)


if __name__ == "__main__":
    main()

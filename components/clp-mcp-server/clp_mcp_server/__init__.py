import ipaddress
import logging
import socket
import sys

import click

from .server import CLPMcpServer


@click.command()
@click.option(
    "--host", type=str, default="0.0.0.0", help="The server's host address (default: 0.0.0.0)."
)
@click.option("--port", type=int, default=8000, help="The server's port number (default: 8000).")
def main(host: str, port: int) -> None:
    """
    Run the CLP MCP Server with HTTP transport.
    """
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
        except socket.error:
            logger.error(
                f"Invalid host: {host} is neither a valid IP address nor a resolvable hostname"
            )
            sys.exit(1)

    if port <= 0 or port > 65535:
        logger.error(f"Port must be between 1 and 65535, got: {port}")
        sys.exit(1)

    try:
        # Create the MCP server instance
        mcp = CLPMcpServer()

        logger.info(f"Starting CLP MCP Server on {host}:{port}")

        # Run the server with HTTP transport
        mcp.run(transport="streamable-http", host=host, port=port)

    except Exception as e:
        logger.error(f"Failed to start MCP server: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()

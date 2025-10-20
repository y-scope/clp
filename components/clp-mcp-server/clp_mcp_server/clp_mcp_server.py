"""CLP MCP Server entry point."""

import ipaddress
import logging
import socket
import sys
from pathlib import Path

import click
from clp_py_utils.clp_config import CLPConfig
from clp_py_utils.core import read_yaml_config_file
from pydantic import ValidationError

from .server import create_mcp_server


@click.command()
@click.option(
    "--host", type=str, default="127.0.0.1", help="The server's host address (default: 127.0.0.1)."
)
@click.option("--port", type=int, default=8000, help="The server's port number (default: 8000).")
@click.option(
    "--config",
    type=click.Path(exists=True),
    default="/etc/clp-config.yml",
    help="The path to server's configuration file (default: /etc/clp-config.yml).",
)
def main(host: str, port: int, config: Path) -> None:
    """
    Runs the CLP MCP server with HTTP transport.

    :param host: The server's host address (IP address or hostname).
    :param port: The server's port number (1-65535).
    :param config: The path to server's configuration file.
    """
    logging.basicConfig(
        level=logging.INFO, format="%(asctime)s - %(name)s - %(levelname)s - %(message)s"
    )
    logger = logging.getLogger(__name__)

    # Validate host and port
    if len(host.strip()) == 0:
        logger.error("Host cannot be empty.")
        sys.exit(1)

    # Validate host format (IP address or resolvable hostname)
    try:
        ipaddress.ip_address(host)
    except ValueError:
        # If not an IP, try to resolve as hostname
        try:
            socket.gethostbyname(host)
        except OSError:
            logger.exception(
                "Host validation failed: '%s' is not a valid IP address and DNS resolution failed.",
                host,
            )
            sys.exit(1)

    max_port = 65535
    if port <= 0 or port > max_port:
        logger.error("Port must be between 1 and %d, got: %d.", max_port, port)
        sys.exit(1)

    try:
        clp_config = CLPConfig.model_validate(read_yaml_config_file(config))
    except ValidationError:
        logger.exception("Configuration validation failed.")
        sys.exit(1)
    except Exception:
        logger.exception("Failed to load configuration.")
        sys.exit(1)

    try:
        mcp = create_mcp_server(clp_config)
        logger.info("Starting CLP MCP Server on %s:%d.", host, port)
        mcp.run(transport="streamable-http", host=host, port=port)
    except Exception:
        logger.exception("Failed to start MCP server.")
        sys.exit(1)


if __name__ == "__main__":
    main()

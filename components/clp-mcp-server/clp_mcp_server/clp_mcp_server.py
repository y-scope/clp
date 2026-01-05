"""CLP MCP Server entry point."""

import ipaddress
import os
import socket
import sys

import click
from clp_py_utils.clp_config import ClpConfig, MCP_SERVER_COMPONENT_NAME
from clp_py_utils.clp_logging import get_logger, set_logging_level
from clp_py_utils.core import read_yaml_config_file
from pydantic import ValidationError

from clp_mcp_server.server import create_mcp_server

logger = get_logger(MCP_SERVER_COMPONENT_NAME)


@click.command()
@click.option(
    "--host", type=str, default="127.0.0.1", help="The server's host address (default: 127.0.0.1)."
)
@click.option("--port", type=int, default=8000, help="The server's port number (default: 8000).")
@click.option(
    "--config-path",
    type=click.Path(exists=True),
    default="/etc/clp-config.yaml",
    help="The path to server's configuration file (default: /etc/clp-config.yaml).",
)
def main(host: str, port: int, config_path: Path) -> int:
    """
    Runs the CLP MCP server with HTTP transport.

    :param host: The server's host address (IP address or hostname).
    :param port: The server's port number (1-65535).
    :param config_path: The path to server's configuration file.
    :return: Exit code (0 for success, non-zero for failure).
    """
    # Set logging level from environment
    set_logging_level(logger, os.getenv("CLP_LOGGING_LEVEL"))

    exit_code = 0

    # Validate host and port
    if len(host.strip()) == 0:
        logger.error("Host cannot be empty.")
        exit_code = 1

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
            exit_code = 1

    max_port = 65535
    if port <= 0 or port > max_port:
        logger.error("Port must be between 1 and %d, got: %d.", max_port, port)
        exit_code = 1

    try:
        clp_config = ClpConfig.model_validate(read_yaml_config_file(config_path))
    except ValidationError:
        logger.exception("Configuration validation failed.")
        exit_code = 1
    except Exception:
        logger.exception("Failed to load configuration.")
        exit_code = 1

    try:
        mcp = create_mcp_server(clp_config)
        logger.info("Starting CLP MCP Server on %s:%d.", host, port)
        mcp.run(transport="streamable-http", host=host, port=port)
    except Exception:
        logger.exception("Failed to start MCP server.")
        exit_code = 1

    return exit_code


if __name__ == "__main__":
    sys.exit(main())

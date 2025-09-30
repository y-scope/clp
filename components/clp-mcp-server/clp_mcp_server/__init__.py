import click
import logging
import os
import sys
import enum
from typing import Optional
from .server import CLPMcpServer


class Transport(enum.Enum):
    stdio = enum.auto()
    sse = enum.auto()
    http = enum.auto()


@click.command()
@click.option(
    "--sse",
    is_flag=True,
    help="Run the MCP server in SSE mode. This option is overridden if you also specify a protocol with --transport.",
)
@click.option(
    "--transport",
    type=click.Choice([t.name for t in Transport], case_sensitive=False),
    help="Specify the transport protocol.",
)
@click.option("--host", type=str, default="0.0.0.0", help="The server's host.")
@click.option("--port", type=int, default=8000, help="The server's port number")
def main(
    sse: bool, transport: Optional[str], host: str, port: int
) -> None:
    """
    Run the CLP MCP Server with the specified transport.
    """

    # Setup logging to file and stdout
    log_dir = os.environ.get("CLP_MCP_LOGS_DIR", "/var/log/mcp-server")
    os.makedirs(log_dir, exist_ok=True)
    log_path = os.path.join(log_dir, "mcp_server.log")

    sys.stdout = open(log_path, "a", buffering=1)
    sys.stderr = open(log_path, "a", buffering=1)

    # Create the MCP server instance
    mcp = CLPMcpServer()

    # Determine transport if not specified
    if transport is None:
        transport_enum = Transport.sse if sse else Transport.stdio
    else:
        transport_enum = Transport[transport]

    # Run the server with the appropriate transport
    match transport_enum:
        case Transport.stdio:
            mcp.run(transport="stdio")
        case Transport.sse:
            mcp.run(transport="sse", host=host, port=port)
        case Transport.http:
            mcp.run(transport="streamable-http", host=host, port=port)


if __name__ == "__main__":
    main()
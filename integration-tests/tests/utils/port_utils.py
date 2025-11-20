"""Functions for facilitating the clp package's port connections."""

from clp_py_utils.clp_config import ClpConfig

REQUIRED_PORTS = 10
SYSTEM_PORTS_BOUNDARY = 1023
MAX_PORT = 65535


def _validate_base_port(base_port: int) -> None:
    """
    Validate that `base_port` plus `REQUIRED_PORTS` stays within [1024, 65535].

    :param base_port:
    :raise ValueError: if the range exceeds the valid TCP port range.
    """
    if base_port <= SYSTEM_PORTS_BOUNDARY:
        err_msg = f"BASE_PORT should be at least 1024, got {base_port}."
        raise ValueError(err_msg)
    if base_port + REQUIRED_PORTS - 1 > MAX_PORT:
        err_msg = f"BASE_PORT={base_port} leaves insufficient headroom for {REQUIRED_PORTS} ports."
        raise ValueError(err_msg)


def assign_ports_from_base(clp_config: ClpConfig, base_port: int) -> None:
    """
    Assign ports for all components that require a port in `clp_config`. Ports are assigned
    relative to `base_port`.

    :param clp_config:
    :param base_port:
    """
    # Ensure base_port is valid and that there's enough room to assign all ports.
    _validate_base_port(base_port=base_port)

    current_port = base_port

    clp_config.database.port = current_port
    current_port += 1

    clp_config.queue.port = current_port
    current_port += 1

    clp_config.redis.port = current_port
    current_port += 1

    # Reducer uses `base_port` instead of `port`
    clp_config.reducer.base_port = current_port
    current_port += 1

    clp_config.results_cache.port = current_port
    current_port += 1

    clp_config.query_scheduler.port = current_port
    current_port += 1

    clp_config.api_server.port = current_port
    current_port += 1

    clp_config.webui.port = current_port
    current_port += 1

    # Optional services
    if clp_config.mcp_server is not None:
        clp_config.mcp_server.port = current_port
        current_port += 1

    if clp_config.presto is not None:
        clp_config.presto.port = current_port
        current_port += 1

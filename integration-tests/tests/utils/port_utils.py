"""Functions for facilitating the port connections for the CLP package."""

from clp_py_utils.clp_config import ClpConfig

# MAX_REQUIRED_PORTS is equal to the max number of CLP package components that need a port.
MAX_REQUIRED_PORTS = 10
MIN_NON_PRIVILEGED_PORT = 1024
MAX_PORT = 65535


def assign_ports_from_base(base_port: int, clp_config: ClpConfig) -> None:
    """
    Assign ports for all components that require a port in `clp_config`. Ports are assigned
    relative to `base_port`.

    :param base_port:
    :param clp_config:
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

    # Reducer binds to a range of ports starting from a `base_port`.
    clp_config.reducer.base_port = current_port
    current_port += 1

    clp_config.results_cache.port = current_port
    current_port += 1

    clp_config.query_scheduler.port = current_port
    current_port += 1

    clp_config.webui.port = current_port
    current_port += 1

    # Optional services
    if clp_config.api_server is not None:
        clp_config.api_server.port = current_port
    current_port += 1

    if clp_config.mcp_server is not None:
        clp_config.mcp_server.port = current_port
    current_port += 1

    if clp_config.presto is not None:
        clp_config.presto.port = current_port
    current_port += 1


def _validate_base_port(base_port: int) -> None:
    """
    Validate that `base_port` plus `MAX_REQUIRED_PORTS` stays within [1024, 65535].

    :param base_port:
    :raise ValueError: if the range exceeds the valid TCP port range.
    """
    if base_port < MIN_NON_PRIVILEGED_PORT:
        err_msg = f"Base port number should be at least {MIN_NON_PRIVILEGED_PORT}, got {base_port}."
        raise ValueError(err_msg)
    if base_port + MAX_REQUIRED_PORTS - 1 > MAX_PORT:
        err_msg = (
            f"The port range from {base_port} to {base_port + MAX_REQUIRED_PORTS - 1} must not"
            f" extend past the maximum port {MAX_PORT}."
        )
        raise ValueError(err_msg)

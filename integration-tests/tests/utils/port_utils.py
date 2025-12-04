"""Functions for facilitating the port connections for the CLP package."""

import socket
from typing import Any

from clp_py_utils.clp_config import ClpConfig

# Port constants.
MIN_NON_PRIVILEGED_PORT = 1024
MAX_PORT = 65535
VALID_PORT_RANGE = range(MIN_NON_PRIVILEGED_PORT, MAX_PORT + 1)

# CLP constants.
REDUCER_MAX_PORTS = 128
PORT_LIKE_ATTR_NAMES = [
    "port",
    "base_port",
]


def assign_ports_from_base(base_port: int, clp_config: ClpConfig) -> None:
    """
    Assign ports for all components described in `clp_config` that require a port. Ports are
    assigned relative to `base_port`.

    :param base_port:
    :param clp_config:
    :raise ValueError: if the base port is out of range or if any required port is in use.
    """
    # Discover which components in ClpConfig have a port-like data member.
    component_port_targets: list[tuple[Any, str, int]] = []
    for attr_name, attr_value in vars(clp_config).items():
        if attr_name.startswith("_") or attr_value is None:
            continue

        port_attr_name: str | None = None
        for port_like_name in PORT_LIKE_ATTR_NAMES:
            if hasattr(attr_value, port_like_name):
                port_attr_name = port_like_name
                break

        if port_attr_name is None:
            # This component doesn't have any port-like attributes.
            continue

        ports_required_for_component = 1
        if attr_name == "reducer":
            # The reducer needs a block of ports starting at its base_port.
            ports_required_for_component = REDUCER_MAX_PORTS

        component_port_targets.append((attr_value, port_attr_name, ports_required_for_component))

    # Ensure desired port range is valid and that all ports in the range are available.
    total_ports_required = sum(
        ports_required_for_component
        for _, _, ports_required_for_component in component_port_targets
    )
    desired_port_range = range(base_port, base_port + total_ports_required)
    _validate_port_range(port_range=desired_port_range)
    _validate_ports_available_in_range(host="127.0.0.1", port_range=desired_port_range)

    # Assign ports to the components.
    current_port = base_port
    for attr_value, port_attr_name, ports_required_for_component in component_port_targets:
        setattr(attr_value, port_attr_name, current_port)
        current_port += ports_required_for_component


def _validate_port_range(port_range: range) -> None:
    """
    Validate that `port_range` falls completely within `VALID_PORT_RANGE`.

    :param port_range:
    :raise ValueError: if any part of `port_range` falls outside `VALID_PORT_RANGE`.
    """
    if port_range.start < VALID_PORT_RANGE.start or port_range.stop > VALID_PORT_RANGE.stop:
        required_start_port = port_range.start
        required_end_port = port_range.stop - 1
        min_valid_port = VALID_PORT_RANGE.start
        max_valid_port = VALID_PORT_RANGE.stop - 1
        err_msg = (
            f"The port range derived from --clp-base-port ('{required_start_port}' to"
            f" '{required_end_port}' inclusive) must fall within the range of valid ports"
            f" ('{min_valid_port}' to '{max_valid_port}' inclusive)."
        )
        raise ValueError(err_msg)


def _validate_ports_available_in_range(host: str, port_range: range) -> None:
    """
    Validate that each port in `port_range` is available on `host`.

    :param host:
    :param port_range:
    :raise ValueError: if any port in the range cannot be bound.
    """
    for port in port_range:
        if not _is_port_free(port=port, host=host):
            start_port = port_range.start
            end_port = port_range.stop - 1
            err_msg = (
                f"Port '{port}' in the desired range '{start_port}' to '{end_port}' is already in"
                " use. Choose a different port range for the test environment."
            )
            raise ValueError(err_msg)


def _is_port_free(port: int, host: str) -> bool:
    """
    Check whether a TCP port is available for binding.

    :param port:
    :param host:
    :return: True if the port can be bound, otherwise False.
    """
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        try:
            sock.bind((host, port))
        except OSError:
            return False
        return True

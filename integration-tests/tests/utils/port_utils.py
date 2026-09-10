"""Functions for facilitating the port connections for the CLP package."""

import socket
from dataclasses import dataclass

from clp_py_utils.clp_config import (
    ClpConfig,
    REDUCER_COMPONENT_NAME,
)
from pydantic import BaseModel

# Port constants.
MIN_NON_PRIVILEGED_PORT = 1024
MAX_PORT = 65535
VALID_PORT_RANGE = range(MIN_NON_PRIVILEGED_PORT, MAX_PORT + 1)

# Practical maximum number of reducer instances.
REDUCER_MAX_PORTS = 128

# Attribute names used to discover port configuration in component objects.
PORT_ATTR_NAMES = ["port", "base_port"]


@dataclass
class ComponentPortAssignment:
    """
    A port assignment for a CLP component.

    :param component_name: The name of the CLP component.
    :param component_config: The component configuration object.
    :param port_attr_name: The name of the port attribute in `component_config`.
    :param start_port: The first port in this component's assignment range.
    :param port_count: The number of ports required by the component.
    """

    component_name: str
    component_config: BaseModel
    port_attr_name: str
    start_port: int
    port_count: int

    @property
    def port_range(self) -> range:
        """:return: The range of port numbers required by this component."""
        return range(self.start_port, self.start_port + self.port_count)


def assign_ports_from_base(base_port: int, clp_config: ClpConfig) -> None:
    """
    Assigns ports to all components in `clp_config` that require them, starting from `base_port`.
    Ports are assigned sequentially, with each component receiving the number of ports it requires.

    :param base_port:
    :param clp_config:
    :raise ValueError: If the base port is out of range, or if any required port is in use.
    """
    port_assignments = _assign_component_ports(base_port, clp_config)
    _check_ports_available(host="127.0.0.1", port_assignments=port_assignments)

    # Write the port assignments to each component config.
    for assignment in port_assignments:
        setattr(assignment.component_config, assignment.port_attr_name, assignment.start_port)


def _assign_component_ports(base_port: int, clp_config: ClpConfig) -> list[ComponentPortAssignment]:
    """
    Assigns port numbers to all components that require them. Validates that the port assignments do
    not exceed the valid port range.

    :param base_port:
    :param clp_config:
    :return: A list of ComponentPortAssignment objects.
    """
    port_assignments: list[ComponentPortAssignment] = []

    current_port = base_port
    for component_name, component_config in vars(clp_config).items():
        # Skip private attributes and None values.
        if component_name.startswith("_") or component_config is None:
            continue

        # Check if this component has a port attribute.
        port_attr_name = None
        for attr_name in PORT_ATTR_NAMES:
            if hasattr(component_config, attr_name):
                port_attr_name = attr_name
                break
        if port_attr_name is None:
            continue

        # Determine how many ports this component needs.
        port_count = REDUCER_MAX_PORTS if component_name == REDUCER_COMPONENT_NAME else 1

        port_assignments.append(
            ComponentPortAssignment(
                component_name=component_name,
                component_config=component_config,
                port_attr_name=port_attr_name,
                start_port=current_port,
                port_count=port_count,
            )
        )

        current_port += port_count

    port_range = range(base_port, current_port)
    _validate_port_range_bounds(port_range)

    return port_assignments


def _check_ports_available(host: str, port_assignments: list[ComponentPortAssignment]) -> None:
    """
    Checks that all ports in the given port assignment list are available for binding.

    :param host:
    :param port_assignments:
    :raise ValueError: If any port in the range is already in use.
    """
    for assignment in port_assignments:
        for port_num in assignment.port_range:
            if not _is_port_free(port=port_num, host=host):
                err_msg = (
                    f"Port '{port_num}' requested by component '{assignment.component_name}' is"
                    " already in use. Choose a different base port for the test environment."
                )
                raise ValueError(err_msg)


def _format_port_range(port_range: range) -> str:
    """
    Format a port range as a human-readable string.

    :param port_range:
    :return: A string like "'1024' to '65535' inclusive".
    """
    start_port = port_range.start
    end_port = port_range.stop - 1
    return f"'{start_port}' to '{end_port}' inclusive"


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


def _validate_port_range_bounds(port_range: range) -> None:
    """
    Validates that the given port range falls within the valid port range.

    :param port_range:
    :raise ValueError: If any part of the range falls outside valid port numbers.
    """
    start_port = port_range.start
    end_port = port_range.stop - 1
    min_valid_port = VALID_PORT_RANGE.start
    max_valid_port = VALID_PORT_RANGE.stop - 1

    if start_port < min_valid_port or end_port > max_valid_port:
        required_range_str = _format_port_range(port_range)
        valid_range_str = _format_port_range(VALID_PORT_RANGE)
        err_msg = (
            f"The port range derived from --base-port ({required_range_str}) must fall within"
            f" the range of valid ports ({valid_range_str})."
        )
        raise ValueError(err_msg)

"""Fixtures that create and remove temporary config files for CLP packages."""

import contextlib
import logging
from collections.abc import Iterator

import pytest

from tests.utils.clp_mode_utils import (
    get_clp_config_from_mode,
    get_required_component_list,
)
from tests.utils.config import PackageConfig, PackagePathConfig
from tests.utils.port_utils import assign_ports_from_base

logger = logging.getLogger(__name__)


@pytest.fixture
def clp_config(
    package_path_config: PackagePathConfig,
    request: pytest.FixtureRequest,
) -> Iterator[PackageConfig]:
    """
    Creates and maintains a PackageConfig object for a specific CLP mode.

    :param request:
    :return: An iterator that yields the PackageConfig object for the specified mode.
    """
    mode_name: str = request.param
    logger.debug("Creating a temporary config file for the %s package.", mode_name)

    # Create the underlying ClpConfig for this mode.
    clp_config_obj = get_clp_config_from_mode(mode_name)

    # Assign ports based on BASE_PORT from ini.
    base_port_string = request.config.getini("BASE_PORT")
    try:
        base_port = int(base_port_string)
    except ValueError as err:
        err_msg = (
            f"Invalid BASE_PORT value '{base_port_string}' in pytest.ini; expected an integer."
        )
        raise ValueError(err_msg) from err

    assign_ports_from_base(clp_config_obj, base_port)

    # Compute the list of required components for this mode.
    required_components = get_required_component_list(clp_config_obj)

    package_config = PackageConfig(
        path_config=package_path_config,
        mode_name=mode_name,
        component_list=required_components,
    )

    # Create the temp config file using the PackageConfig member path.
    PackageConfig.write_temp_config_file(
        clp_config=clp_config_obj,
        package_config=package_config,
    )

    try:
        yield package_config
    finally:
        logger.debug("Removing the temporary config file.")
        with contextlib.suppress(FileNotFoundError):
            package_config.temp_config_file_path.unlink()

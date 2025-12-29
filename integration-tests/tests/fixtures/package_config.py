"""Fixtures that create and remove temporary config files for CLP packages."""

import logging
from collections.abc import Iterator

import pytest

from tests.utils.clp_mode_utils import (
    get_clp_config_from_mode,
    get_required_component_list,
)
from tests.utils.config import PackageConfig, PackagePathConfig
from tests.utils.logging_utils import construct_log_err_msg
from tests.utils.port_utils import assign_ports_from_base

logger = logging.getLogger(__name__)


@pytest.fixture
def fixt_package_config(
    request: pytest.FixtureRequest,
    fixt_package_path_config: PackagePathConfig,
) -> Iterator[PackageConfig]:
    """
    Creates and maintains a PackageConfig object for a specific CLP mode.

    :param request:
    :return: An iterator that yields the PackageConfig object for the specified mode.
    :raise ValueError: if the CLP base port's value is invalid.
    """
    mode_name: str = request.param
    logger.info("Setting up the '%s' package...", mode_name)

    logger.debug("Getting the ClpConfig object for the '%s' package...", mode_name)
    clp_config_obj = get_clp_config_from_mode(mode_name)

    # Assign ports based on the clp base port CLI option.
    logger.debug("Assigning ports to the components in the '%s' package...", mode_name)
    base_port_string = request.config.getoption("--base-port")
    try:
        base_port = int(base_port_string)
    except ValueError as err:
        err_msg = f"Invalid value '{base_port_string}' for '--base-port'; expected an integer."
        logger.error(construct_log_err_msg(err_msg))
        raise ValueError(err_msg) from err
    assign_ports_from_base(base_port, clp_config_obj)

    logger.debug("Constructing the list of required components for the '%s' package...", mode_name)
    required_components = get_required_component_list(clp_config_obj)

    # Construct PackageConfig.
    package_config = PackageConfig(
        path_config=fixt_package_path_config,
        mode_name=mode_name,
        component_list=required_components,
        clp_config=clp_config_obj,
        base_port=base_port,
    )

    try:
        yield package_config
    finally:
        logger.info("Cleaning up the '%s' package...", mode_name)
        package_config.temp_config_file_path.unlink(missing_ok=True)

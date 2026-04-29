"""Fixtures that create and remove temporary config files for CLP packages."""

# NOTE: this module will be deprecated in favour of
# `integration-tests/tests/package_tests/fixtures.py`.

import logging
from collections.abc import Iterator

import pytest

from tests.utils.config import PackageModeConfig, PackagePathConfig, PackageTestConfig
from tests.utils.port_utils import assign_ports_from_base

logger = logging.getLogger(__name__)


@pytest.fixture(scope="module")
def fixt_package_test_config(
    request: pytest.FixtureRequest,
    fixt_package_path_config: PackagePathConfig,
) -> Iterator[PackageTestConfig]:
    """
    Creates and maintains a module-level PackageTestConfig object for a specific CLP mode. For
    efficiency, group all tests for a given mode in the same module.

    :param request: Provides `PackageModeConfig` via `request.param`.
    :return: An iterator that yields the PackageTestConfig object for the specified mode.
    :raise ValueError: if the CLP base port's value is invalid.
    """
    mode_config: PackageModeConfig = request.param
    clp_config_obj = mode_config.clp_config

    # Assign ports based on the clp base port CLI option.
    log_msg = f"Assigning ports to the '{mode_config.mode_name}' package."
    logger.info(log_msg)
    base_port_string = request.config.getoption("--base-port")
    try:
        base_port = int(base_port_string)
    except ValueError as err:
        err_msg = f"Invalid value '{base_port_string}' for '--base-port'; expected an integer."
        raise ValueError(err_msg) from err
    assign_ports_from_base(base_port, clp_config_obj)

    # Construct PackageTestConfig.
    log_msg = (
        f"Constructing the PackageTestConfig object for the '{mode_config.mode_name}' package."
    )
    logger.info(log_msg)
    package_test_config = PackageTestConfig(
        path_config=fixt_package_path_config,
        mode_config=mode_config,
        base_port=base_port,
    )

    try:
        yield package_test_config
    finally:
        package_test_config.temp_config_file_path.unlink(missing_ok=True)

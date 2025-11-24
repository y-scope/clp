"""Fixtures that create and remove temporary config files for CLP packages."""

import contextlib
from collections.abc import Iterator

import pytest

from tests.utils.clp_mode_utils import (
    get_clp_config_from_mode,
    get_required_component_list,
)
from tests.utils.config import PackageConfig, PackagePathConfig
from tests.utils.port_utils import assign_ports_from_base


@pytest.fixture
def fixt_package_config(
    fixt_package_path_config: PackagePathConfig,
    request: pytest.FixtureRequest,
) -> Iterator[PackageConfig]:
    """
    Creates and maintains a PackageConfig object for a specific CLP mode.

    :param request:
    :return: An iterator that yields the PackageConfig object for the specified mode.
    """
    mode_name: str = request.param

    # Get the ClpConfig for this mode.
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

    assign_ports_from_base(base_port, clp_config_obj)

    # Compute the list of required components for this mode.
    required_components = get_required_component_list(clp_config_obj)

    # Construct PackageConfig.
    package_config = PackageConfig(
        path_config=fixt_package_path_config,
        mode_name=mode_name,
        component_list=required_components,
        clp_config=clp_config_obj,
    )

    try:
        yield package_config
    finally:
        with contextlib.suppress(FileNotFoundError):
            package_config.temp_config_file_path.unlink()

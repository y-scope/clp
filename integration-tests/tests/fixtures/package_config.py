"""Fixtures that create and remove temporary config files for CLP packages."""

from collections.abc import Iterator

import pytest

from tests.utils.clp_mode_utils import (
    get_clp_config_from_mode,
    get_required_component_list,
)
from tests.utils.config import PackageConfig, PackagePathConfig


@pytest.fixture
def fixt_package_config(
    request: pytest.FixtureRequest,
    fixt_package_path_config: PackagePathConfig,
) -> Iterator[PackageConfig]:
    """
    Creates and maintains a PackageConfig object for a specific CLP mode.

    :param request:
    :return: An iterator that yields the PackageConfig object for the specified mode.
    """
    mode_name: str = request.param

    clp_config_obj = get_clp_config_from_mode(mode_name)

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
        package_config.temp_config_file_path.unlink(missing_ok=True)

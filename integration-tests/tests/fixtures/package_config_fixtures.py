"""Fixtures that create and remove temporary config files for CLP packages."""

import logging
from collections.abc import Iterator
from pathlib import Path

import pytest

from tests.utils.clp_mode_utils import CLP_MODE_CONFIGS
from tests.utils.config import PackageConfig
from tests.utils.utils import get_env_var

logger = logging.getLogger(__name__)


def _build_package_config_for_mode(mode_name: str) -> PackageConfig:
    """Construct a PackageConfig for the given `mode_name`."""
    if mode_name not in CLP_MODE_CONFIGS:
        err_msg = f"Unknown CLP mode '{mode_name}'."
        raise KeyError(err_msg)

    clp_package_dir = Path(get_env_var("CLP_PACKAGE_DIR")).expanduser().resolve()
    test_root_dir = Path(get_env_var("CLP_BUILD_DIR")).expanduser().resolve() / "integration-tests"
    build_config = CLP_MODE_CONFIGS[mode_name][0]
    required_components = CLP_MODE_CONFIGS[mode_name][1]

    return PackageConfig(
        clp_package_dir=clp_package_dir,
        test_root_dir=test_root_dir,
        mode_name=mode_name,
        build_config=build_config,
        component_list=required_components,
    )


@pytest.fixture
def clp_config(
    request: pytest.FixtureRequest,
) -> Iterator[PackageConfig]:
    """
    Parameterized fixture that creates and maintains a PackageConfig object for a specific mode of
    operation.
    """
    mode_name: str = request.param
    logger.info("Creating a temporary config file for the %s package...", mode_name)

    package_config = _build_package_config_for_mode(mode_name)

    logger.info("The temporary config file has been written for the %s package.", mode_name)

    try:
        yield package_config
    finally:
        logger.info("Removing the temporary config file...")
        package_config.temp_config_file_path.unlink()
        logger.info("The temporary config file has been removed.")

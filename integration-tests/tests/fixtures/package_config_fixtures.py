"""Fixtures that create and remove temporary config files for CLP packages."""

import contextlib
import logging
from collections.abc import Iterator
from pathlib import Path

import pytest

from tests.utils.clp_mode_utils import CLP_MODE_CONFIGS, get_clp_config_from_mode
from tests.utils.config import PackageConfig
from tests.utils.utils import get_env_var

logger = logging.getLogger(__name__)


def _build_package_config_for_mode(mode_name: str) -> PackageConfig:
    """
    Constructs a PackageConfig for the specified CLP mode.

    :param mode_name:
    :return: A PackageConfig object configured for the given mode.
    :raise KeyError: if the mode name is unknown.
    """
    if mode_name not in CLP_MODE_CONFIGS:
        err_msg = f"Unknown CLP mode '{mode_name}'."
        raise KeyError(err_msg)

    clp_package_dir = Path(get_env_var("CLP_PACKAGE_DIR")).expanduser().resolve()
    test_root_dir = Path(get_env_var("CLP_BUILD_DIR")).expanduser().resolve() / "integration-tests"

    return PackageConfig(
        clp_package_dir=clp_package_dir,
        test_root_dir=test_root_dir,
        mode_name=mode_name,
    )


@pytest.fixture
def clp_config(
    request: pytest.FixtureRequest,
) -> Iterator[PackageConfig]:
    """
    Creates and maintains a PackageConfig object for a specific CLP mode.

    :param request:
    :return: An iterator that yields the PackageConfig object for the specified mode.
    """
    mode_name: str = request.param
    logger.info("Creating a temporary config file for the %s package...", mode_name)

    package_config = _build_package_config_for_mode(mode_name)

    # Create the temp config file.
    clp_config_obj = get_clp_config_from_mode(mode_name)
    temp_config_file_path = PackageConfig.write_temp_config_file(
        clp_config=clp_config_obj,
        temp_config_dir=package_config.temp_config_dir,
        mode_name=mode_name,
    )

    logger.info("The temporary config file has been written for the %s package.", mode_name)

    try:
        yield package_config
    finally:
        logger.info("Removing the temporary config file...")
        with contextlib.suppress(FileNotFoundError):
            temp_config_file_path.unlink()
        logger.info("The temporary config file has been removed.")

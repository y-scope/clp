"""Fixtures that create and remove temporary config files for CLP packages."""

import logging
from collections.abc import Iterator
from pathlib import Path
from typing import Any

import pytest

from tests.utils.config import (
    PackageConfig,
    PackageInstanceConfig,
    PackageModeConfig,
)
from tests.utils.package_utils import (
    CLP_MODE_CONFIGS,
    get_dict_from_mode_name,
    write_temp_config_file,
)

logger = logging.getLogger(__name__)


def _build_package_instance_config(
    mode_name: str,
    package_config: PackageConfig,
) -> PackageInstanceConfig:
    """Construct a PackageInstanceConfig for the given `mode_name`."""
    if mode_name not in CLP_MODE_CONFIGS:
        err_msg = f"Unknown CLP mode '{mode_name}'. Known modes: {list(CLP_MODE_CONFIGS.keys())}"
        raise KeyError(err_msg)

    # Find the corresponding PackageModeConfig object and instantiate PackageInstanceConfig.
    mode_config: PackageModeConfig = CLP_MODE_CONFIGS[mode_name]
    run_config = PackageInstanceConfig(
        package_config=package_config,
        mode_config=mode_config,
    )

    # Write the temporary config file that the instance will use during the test.
    mode_kv_dict: dict[str, Any] = get_dict_from_mode_name(mode_name)
    temp_config_file_path: Path = write_temp_config_file(
        mode_kv_dict=mode_kv_dict,
        temp_config_dir=package_config.temp_config_dir,
        mode_name=mode_name,
    )
    object.__setattr__(run_config, "temp_config_file_path", temp_config_file_path)

    return run_config


@pytest.fixture
def clp_config(
    request: pytest.FixtureRequest,
    package_config: PackageConfig,
) -> Iterator[PackageInstanceConfig]:
    """
    Parameterized fixture that creates and removes a temporary config file for a mode of operation.
    The mode name arrives through request.param from the test's indirect parametrization.
    """
    mode_name: str = request.param
    logger.info("Creating a temporary config file for the %s package...", mode_name)

    run_config = _build_package_instance_config(mode_name, package_config)

    logger.info("The temporary config file has been written for the %s package.", mode_name)

    try:
        yield run_config
    finally:
        logger.info("Removing the temporary config file...")
        run_config.temp_config_file_path.unlink()
        logger.info("The temporary config file has been removed.")

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


@pytest.fixture
def clp_text_config(
    package_config: PackageConfig,
) -> Iterator[PackageInstanceConfig]:
    """Fixture that creates and maintains a config file for clp-text."""
    mode_name = "clp-text"
    logger.info("Creating a temporary config file for the %s package...", mode_name)

    mode_config: PackageModeConfig = CLP_MODE_CONFIGS[mode_name]
    run_config = PackageInstanceConfig(
        package_config=package_config,
        mode_config=mode_config,
    )

    # Create a temporary config file for the package run.
    mode_kv_dict: dict[str, Any] = get_dict_from_mode_name(mode_name)
    temp_config_file_path: Path = write_temp_config_file(
        mode_kv_dict=mode_kv_dict,
        temp_config_dir=package_config.temp_config_dir,
        mode_name=mode_name,
    )
    object.__setattr__(run_config, "temp_config_file_path", temp_config_file_path)

    logger.info("The temporary config file has been written for the %s package.", mode_name)

    yield run_config

    # Remove the temporary config file.
    logger.info("Removing the temporary config file...")

    temp_config_file_path.unlink()

    logger.info("The temporary config file has been removed.")


@pytest.fixture
def clp_json_config(
    package_config: PackageConfig,
) -> Iterator[PackageInstanceConfig]:
    """Fixture that creates and maintains a config file for clp-json."""
    mode_name = "clp-json"
    logger.info("Creating a temporary config file for the %s package...", mode_name)

    mode_config: PackageModeConfig = CLP_MODE_CONFIGS[mode_name]
    run_config = PackageInstanceConfig(
        package_config=package_config,
        mode_config=mode_config,
    )

    # Create a temporary config file for the package run.
    mode_kv_dict: dict[str, Any] = get_dict_from_mode_name(mode_name)
    temp_config_file_path: Path = write_temp_config_file(
        mode_kv_dict=mode_kv_dict,
        temp_config_dir=package_config.temp_config_dir,
        mode_name=mode_name,
    )
    object.__setattr__(run_config, "temp_config_file_path", temp_config_file_path)

    logger.info("The temporary config file has been written for the %s package.", mode_name)

    yield run_config

    # Remove the temporary config file.
    logger.info("Removing the temporary config file...")

    temp_config_file_path.unlink()

    logger.info("The temporary config file has been removed.")

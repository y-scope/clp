"""Define test package config file fixtures."""

import logging
from collections.abc import Iterator
from pathlib import Path
from typing import Any

import pytest

from tests.utils.config import (
    PackageConfig,
    PackageInstanceConfig,
)
from tests.utils.package_utils import (
    get_dict_from_mode,
    write_temp_config_file,
)

logger = logging.getLogger(__name__)


@pytest.fixture
def clp_text_config(
    package_config: PackageConfig,
) -> Iterator[PackageInstanceConfig]:
    """Fixture that creates and maintains a config file for clp-text."""
    mode = "clp-text"

    log_msg = f"Creating a temporary config file for the {mode} package..."
    logger.info(log_msg)

    run_config = PackageInstanceConfig(
        package_config=package_config,
        mode=mode,
    )

    # Create a temporary config file for the package run.
    mode_kv_dict: dict[str, Any] = get_dict_from_mode(mode)
    temp_config_file_path: Path = write_temp_config_file(
        mode_kv_dict=mode_kv_dict,
        temp_config_dir=package_config.temp_config_dir,
        mode=mode,
    )
    object.__setattr__(run_config, "temp_config_file_path", temp_config_file_path)

    log_msg = f"The temporary config file has been written for the {mode} package."
    logger.info(log_msg)

    yield run_config

    # Delete the temporary config file.
    logger.info("Deleting the temporary config file...")

    temp_config_file_path.unlink()

    logger.info("The temporary config file has been deleted.")


@pytest.fixture
def clp_json_config(
    package_config: PackageConfig,
) -> Iterator[PackageInstanceConfig]:
    """Fixture that creates and maintains a config file for clp-json."""
    mode = "clp-json"

    log_msg = f"Creating a temporary config file for the {mode} package..."
    logger.info(log_msg)

    run_config = PackageInstanceConfig(
        package_config=package_config,
        mode=mode,
    )

    # Create a temporary config file for the package run.
    mode_kv_dict: dict[str, Any] = get_dict_from_mode(mode)
    temp_config_file_path: Path = write_temp_config_file(
        mode_kv_dict=mode_kv_dict,
        temp_config_dir=package_config.temp_config_dir,
        mode=mode,
    )
    object.__setattr__(run_config, "temp_config_file_path", temp_config_file_path)

    log_msg = f"The temporary config file has been written for the {mode} package."
    logger.info(log_msg)

    yield run_config

    # Delete the temporary config file.
    logger.info("Deleting the temporary config file...")

    temp_config_file_path.unlink()

    logger.info("The temporary config file has been deleted.")

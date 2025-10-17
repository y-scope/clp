"""Define test packages fixtures."""

import logging
import subprocess
from collections.abc import Iterator
from typing import Literal

import pytest

from tests.utils.config import (
    PackageConfig,
    PackageRun,
)

logger = logging.getLogger(__name__)


@pytest.fixture(scope="session")
def clp_package(
    package_config: PackageConfig,
) -> Iterator[PackageRun]:
    """Fixture that starts up an instance of clp, and stops the package after yield."""
    mode: Literal["clp-text", "clp-json"] = "clp-text"
    instance = _start_clp_package(package_config, mode)

    yield instance
    _stop_clp_package(instance)


def _start_clp_package(
    package_config: PackageConfig,
    mode: Literal["clp-text", "clp-json"],
) -> PackageRun:
    """Starts up an instance of clp."""
    logger.info(f"Starting up the {mode} package...")

    start_script_path = package_config.clp_package_dir / "sbin" / "start-clp.sh"

    try:
        # fmt: off
        start_cmd = [
            start_script_path
        ]
        # fmt: on
        subprocess.run(start_cmd, check=True)
    except Exception as e:
        err_msg = f"Failed to start an instance of the {mode} package."
        raise RuntimeError(err_msg) from e

    package_run = PackageRun(
        package_config=package_config,
        mode=mode,
    )

    logger.info(
        f"An instance of the {package_run.mode} package was started successfully."
        f" Its instance ID is '{package_run.clp_instance_id}'"
    )

    return package_run


def _stop_clp_package(
    instance: PackageRun,
) -> None:
    """Stops an instance of clp."""
    mode = instance.mode
    instance_id = instance.clp_instance_id
    logger.info(f"Now stopping the {mode} package with instance ID '{instance_id}'...")

    stop_script_path = instance.package_config.clp_package_dir / "sbin" / "stop-clp.sh"

    try:
        # fmt: off
        stop_cmd = [
            stop_script_path
        ]
        # fmt: on
        subprocess.run(stop_cmd, check=True)
    except Exception as e:
        err_msg = f"Failed to stop an instance of the {mode} package."
        raise RuntimeError(err_msg) from e

    logger.info(f"The {mode} package with instance ID '{instance_id}' was stopped successfully.")

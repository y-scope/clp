"""Fixtures that start and stop CLP package instances for integration tests."""

import logging
import subprocess
from collections.abc import Iterator

import pytest

from tests.utils.config import (
    PackageConfig,
    PackageInstance,
)
from tests.utils.package_utils import (
    start_clp_package,
    stop_clp_package,
)

logger = logging.getLogger(__name__)


@pytest.fixture
def clp_package(
    clp_config: PackageConfig,
) -> Iterator[PackageInstance]:
    """
    Parameterized fixture that starts a instance of the CLP package in the configuration described
    in PackageConfig, and gracefully stops it at teardown.
    """
    mode_name = clp_config.mode_name
    logger.info("Starting up the %s package...", mode_name)

    start_clp_package(clp_config)

    try:
        instance = PackageInstance(package_config=clp_config)
        instance_id = instance.clp_instance_id
        logger.info(
            "An instance of the %s package was started successfully. Its instance ID is '%s'",
            mode_name,
            instance_id,
        )
        yield instance
    finally:
        logger.info("Now stopping the %s package...", mode_name)
        if instance is not None:
            stop_clp_package(instance)
        else:
            # This means setup failed after start; fall back to calling stop script directly
            subprocess.run([str(clp_config.stop_script_path)], check=True)
        logger.info("The %s package was stopped successfully.", mode_name)

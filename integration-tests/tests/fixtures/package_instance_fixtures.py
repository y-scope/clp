"""Fixtures that start and stop CLP package instances for integration tests."""

import logging
import subprocess
from collections.abc import Iterator

import pytest

from tests.utils.clp_mode_utils import CLP_MODE_CONFIGS
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
    Starts a CLP package instance for the given configuration and stops it during teardown.

    :param clp_config:
    :return: Iterator that yields the running package instance.
    """
    mode_name = clp_config.mode_name
    logger.info("Starting up the %s package...", mode_name)

    # Start the package.
    start_clp_package(clp_config)

    instance: PackageInstance | None = None
    try:
        required_components = CLP_MODE_CONFIGS[mode_name][1]
        instance = PackageInstance(package_config=clp_config, component_list=required_components)
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

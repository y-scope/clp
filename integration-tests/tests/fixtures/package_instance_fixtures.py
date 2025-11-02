"""Fixtures that start and stop CLP package instances for integration tests."""

import logging
from collections.abc import Iterator

import pytest

from tests.utils.config import (
    PackageInstance,
    PackageInstanceConfig,
)
from tests.utils.package_utils import (
    start_clp_package,
    stop_clp_package,
)

logger = logging.getLogger(__name__)


@pytest.fixture
def clp_text_package(
    clp_text_config: PackageInstanceConfig,
) -> Iterator[PackageInstance]:
    """Fixture that launches a clp-text instance, and gracefully stops it at scope."""
    mode_name = clp_text_config.mode_config.name
    log_msg = f"Starting up the {mode_name} package..."
    logger.info(log_msg)

    start_clp_package(clp_text_config)

    instance = PackageInstance(package_instance_config=clp_text_config)
    instance_id = instance.clp_instance_id
    log_msg = (
        f"An instance of the {mode_name} package was started successfully."
        f" Its instance ID is '{instance_id}'"
    )
    logger.info(log_msg)

    yield instance

    log_msg = f"Now stopping the {mode_name} package with instance ID '{instance_id}'..."
    logger.info(log_msg)

    stop_clp_package(instance)

    log_msg = f"The {mode_name} package with instance ID '{instance_id}' was stopped successfully."
    logger.info(log_msg)


@pytest.fixture
def clp_json_package(
    clp_json_config: PackageInstanceConfig,
) -> Iterator[PackageInstance]:
    """Fixture that launches a clp-json instance, and gracefully stops it at scope."""
    mode_name = clp_json_config.mode_config.name
    log_msg = f"Starting up the {mode_name} package..."
    logger.info(log_msg)

    start_clp_package(clp_json_config)

    instance = PackageInstance(package_instance_config=clp_json_config)
    instance_id = instance.clp_instance_id
    log_msg = (
        f"An instance of the {mode_name} package was started successfully."
        f" Its instance ID is '{instance_id}'"
    )
    logger.info(log_msg)

    yield instance

    log_msg = f"Now stopping the {mode_name} package with instance ID '{instance_id}'..."
    logger.info(log_msg)

    stop_clp_package(instance)

    log_msg = f"The {mode_name} package with instance ID '{instance_id}' was stopped successfully."
    logger.info(log_msg)

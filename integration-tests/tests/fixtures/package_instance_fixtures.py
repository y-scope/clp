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
    """Fixture that launches a clp-text instance, and gracefully stops it at scope boundary."""
    mode_name = clp_text_config.mode_config.name
    logger.info("Starting up the %s package...", mode_name)

    start_clp_package(clp_text_config)

    instance = PackageInstance(package_instance_config=clp_text_config)
    instance_id = instance.clp_instance_id
    logger.info(
        "An instance of the %s package was started successfully. Its instance ID is '%s'",
        mode_name,
        instance_id,
    )

    yield instance

    logger.info("Now stopping the %s package with instance ID '%s'...", mode_name, instance_id)

    stop_clp_package(instance)

    logger.info(
        "The %s package with instance ID '%s' was stopped successfully.", mode_name, instance_id
    )


@pytest.fixture
def clp_json_package(
    clp_json_config: PackageInstanceConfig,
) -> Iterator[PackageInstance]:
    """Fixture that launches a clp-json instance, and gracefully stops it at scope boundary."""
    mode_name = clp_json_config.mode_config.name
    logger.info("Starting up the %s package...", mode_name)

    start_clp_package(clp_json_config)

    instance = PackageInstance(package_instance_config=clp_json_config)
    instance_id = instance.clp_instance_id
    logger.info(
        "An instance of the %s package was started successfully. Its instance ID is '%s'",
        mode_name,
        instance_id,
    )

    yield instance

    logger.info("Now stopping the %s package with instance ID '%s'...", mode_name, instance_id)

    stop_clp_package(instance)

    logger.info(
        "The %s package with instance ID '%s' was stopped successfully.", mode_name, instance_id
    )

"""Fixtures that start and stop CLP package instances for integration tests."""

import logging
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
def fixt_package_instance(
    request: pytest.FixtureRequest, fixt_package_config: PackageConfig
) -> Iterator[PackageInstance]:
    """
    Starts a CLP package instance for the given configuration and stops it during teardown.

    :param fixt_package_config:
    :return: Iterator that yields the running package instance.
    """
    mode_name = fixt_package_config.mode_name

    try:
        logger.info("Starting the '%s' package...", mode_name)
        start_clp_package(request, fixt_package_config)
        instance = PackageInstance(package_config=fixt_package_config)
        yield instance
    finally:
        logger.info("Stopping the '%s' package...", mode_name)
        stop_clp_package(request, fixt_package_config)

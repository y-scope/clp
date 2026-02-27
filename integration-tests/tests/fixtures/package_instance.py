"""Fixtures that start and stop CLP package instances for integration tests."""

import logging
from collections.abc import Iterator

import pytest

from tests.utils.config import (
    PackageInstance,
    PackageTestConfig,
)
from tests.utils.package_utils import (
    start_clp_package,
    stop_clp_package,
)

logger = logging.getLogger(__name__)


@pytest.fixture(scope="module")
def fixt_package_instance(
    request: pytest.FixtureRequest, fixt_package_test_config: PackageTestConfig
) -> Iterator[PackageInstance]:
    """
    Starts a CLP package instance for the given configuration and stops it during teardown.

    This fixture relies on `fixt_package_test_config`, and as such, the scope of this fixture should
    never exceed that of `fixt_package_test_config`.

    :param fixt_package_test_config:
    :return: Iterator that yields the running package instance.
    """
    mode_config = fixt_package_test_config.mode_config
    mode_name = mode_config.mode_name

    try:
        logger.info("Starting the '%s' package...", mode_name)
        start_clp_package(request, fixt_package_test_config)
        instance = PackageInstance(package_test_config=fixt_package_test_config)
        yield instance
    finally:
        logger.info("Stopping the '%s' package...", mode_name)
        stop_clp_package(request, fixt_package_test_config)

"""Fixtures that start and stop CLP package instances for integration tests."""

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


@pytest.fixture(scope="module")
def fixt_package_instance(fixt_package_test_config: PackageTestConfig) -> Iterator[PackageInstance]:
    """
    Starts a CLP package instance for the given configuration and stops it during teardown.

    This fixture relies on `fixt_package_test_config`, and as such, the scope of this fixture should
    never exceed that of `fixt_package_test_config`.

    :param fixt_package_test_config:
    :return: Iterator that yields the running package instance.
    """
    try:
        start_clp_package(fixt_package_test_config)
        instance = PackageInstance(package_test_config=fixt_package_test_config)
        yield instance
    finally:
        stop_clp_package(fixt_package_test_config)

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

    :param fixt_package_test_config:
    :return: Iterator that yields the running package instance.
    """
    try:
        start_clp_package(fixt_package_test_config)
        instance = PackageInstance(package_test_config=fixt_package_test_config)
        yield instance
    except RuntimeError:
        mode_config = fixt_package_test_config.mode_config
        mode_name = mode_config.mode_name
        base_port = fixt_package_test_config.base_port
        pytest.fail(
            f"Failed to start the {mode_name} package. This could mean that one of the ports"
            f" derived from base_port={base_port} was unavailable. You can specify a new value for"
            " base_port with the '--base-port' flag."
        )
    finally:
        stop_clp_package(fixt_package_test_config)

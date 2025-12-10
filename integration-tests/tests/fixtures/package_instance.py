"""Fixtures that start and stop CLP package instances for integration tests."""

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


@pytest.fixture
def fixt_package_instance(fixt_package_config: PackageConfig) -> Iterator[PackageInstance]:
    """
    Starts a CLP package instance for the given configuration and stops it during teardown.

    :param fixt_package_config:
    :return: Iterator that yields the running package instance.
    """
    mode_name = fixt_package_config.mode_name

    try:
        start_clp_package(fixt_package_config)
        instance = PackageInstance(package_config=fixt_package_config)
        yield instance
    except RuntimeError:
        base_port = fixt_package_config.base_port
        pytest.fail(
            f"Failed to start the {mode_name} package. This could mean that one of the ports"
            f" derived from base_port={base_port} was unavailable. You can specify a new value for"
            " base_port with the '--base-port' flag."
        )
    finally:
        stop_clp_package(fixt_package_config)

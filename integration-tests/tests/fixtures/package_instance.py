"""Fixtures that start and stop CLP package instances for integration tests."""

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


@pytest.fixture
def fixt_package_instance(
    fixt_package_config: PackageConfig,
    request: pytest.FixtureRequest,
) -> Iterator[PackageInstance]:
    """
    Starts a CLP package instance for the given configuration and stops it during teardown.

    :param fixt_package_config:
    :param request:
    :return: Iterator that yields the running package instance.
    """
    mode_name = fixt_package_config.mode_name
    instance: PackageInstance | None = None

    try:
        start_clp_package(fixt_package_config)
        instance = PackageInstance(package_config=fixt_package_config)
        yield instance
    except RuntimeError:
        base_port_string = request.config.getini("BASE_PORT")
        pytest.fail(
            f"Failed to start the {mode_name} package. This could mean that one of the ports"
            f" derived from BASE_PORT={base_port_string} was unavailable. Try changing BASE_PORT in"
            " .pytest.ini."
        )
    finally:
        if instance is not None:
            stop_clp_package(instance)
        else:
            # This means setup failed after start; fall back to calling stop script directly
            subprocess.run([str(fixt_package_config.path_config.stop_script_path)], check=False)

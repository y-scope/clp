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
def fixt_package_instance(
    request: pytest.FixtureRequest,
    fixt_package_config: PackageConfig,
) -> Iterator[PackageInstance]:
    """
    Starts a CLP package instance for the given configuration and stops it during teardown.

    :param fixt_package_config:
    :return: Iterator that yields the running package instance.
    """
    mode_name = fixt_package_config.mode_name
    no_jobs: bool = bool(request.config.option.NO_JOBS)
    package_job_list = fixt_package_config.package_job_list

    # Do not start this mode if there are no jobs and the '--no-jobs' flag wasn't specified by user.
    if package_job_list is None and not no_jobs:
        pytest.skip(f"No jobs to run for mode {mode_name} with current job filter.")

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
        stop_clp_package(fixt_package_config)

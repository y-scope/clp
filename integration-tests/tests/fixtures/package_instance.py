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
    no_jobs: bool = bool(request.config.option.NO_JOBS)
    package_job_list = fixt_package_config.package_job_list

    # Do not start this mode if there are no jobs and the '--no-jobs' flag wasn't specified by user.
    if package_job_list is None and not no_jobs:
        pytest.skip(f"No jobs to run for mode '{mode_name}' with current job filter.")

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

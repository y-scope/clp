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
    start_presto_cluster,
    stop_clp_package,
    stop_presto_cluster,
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
        logger.info("Starting the '%s' package...", mode_name)
        start_clp_package(fixt_package_config)
        instance = PackageInstance(package_config=fixt_package_config)

        if mode_name == "clp-presto":
            logger.info("Starting the Presto cluster for the '%s' package...", mode_name)
            start_presto_cluster(fixt_package_config)

        yield instance
    finally:
        if mode_name == "clp-presto":
            logger.info("Shutting down the Presto cluster...")
            stop_presto_cluster()

        logger.info("Shutting down the package...")
        stop_clp_package(fixt_package_config)

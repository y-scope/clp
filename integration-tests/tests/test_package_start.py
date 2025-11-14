"""Integration tests verifying that the CLP package can be started and stopped."""

import logging

import pytest

from tests.utils.clp_mode_utils import CLP_MODE_CONFIGS
from tests.utils.config import (
    CompressJob,
    PackageFunctionalityList,
    PackageInstance,
    SearchJob,
)
from tests.utils.package_utils import (
    validate_package_running,
    validate_running_mode_correct,
)

TEST_MODES = CLP_MODE_CONFIGS.keys()

logger = logging.getLogger(__name__)


def _run_compress_jobs(jobs: list[CompressJob]) -> None:
    job_descriptions = [f"{job.job_name}" for job in jobs]
    logger.info(
        "_run_compress_jobs: %d job(s): %s",
        len(jobs),
        job_descriptions,
    )
    # TODO: write this.
    assert True


def _run_search_jobs(jobs: list[SearchJob]) -> None:
    job_descriptions = [f"{job.job_name}" for job in jobs]
    logger.info(
        "_run_search_jobs: %d job(s): %s",
        len(jobs),
        job_descriptions,
    )
    # TODO: write this.
    assert True


def _dispatch_test_jobs(jobs_list: PackageFunctionalityList) -> None:
    logger.info("_dispatch_test_jobs")
    if jobs_list.compress_jobs:
        _run_compress_jobs(jobs_list.compress_jobs)
    if jobs_list.search_jobs:
        _run_search_jobs(jobs_list.search_jobs)


@pytest.mark.package
@pytest.mark.parametrize("clp_config", TEST_MODES, indirect=True)
def test_clp_package(
    clp_package: PackageInstance,
    request: pytest.FixtureRequest,
) -> None:
    """
    Validate that all of the components of the CLP package start up successfully for the selected
    mode of operation.

    :param clp_package:
    """
    mode_name = clp_package.package_config.mode_name
    instance_id = clp_package.clp_instance_id

    # Ensure that all package components are running.
    logger.info(
        "Checking if all components of %s package with instance ID '%s' are running properly...",
        mode_name,
        instance_id,
    )

    running, fail_msg = validate_package_running(clp_package)
    if not running:
        assert fail_msg is not None
        pytest.fail(fail_msg)

    logger.info(
        "All components of the %s package with instance ID '%s' are running properly.",
        mode_name,
        instance_id,
    )

    # Ensure that the package is running in the correct mode.
    logger.info(
        "Checking that the %s package with instance ID '%s' is running in the correct mode...",
        mode_name,
        instance_id,
    )

    correct, fail_msg = validate_running_mode_correct(clp_package)
    if not correct:
        assert fail_msg is not None
        pytest.fail(fail_msg)

    logger.info(
        "The %s package with instance ID '%s' is running in the correct mode.",
        mode_name,
        instance_id,
    )

    no_jobs: bool = bool(request.config.option.NO_JOBS)
    if no_jobs:
        logger.info(
            "The --no-jobs flag is set. The test will not create or execute any CLP test jobs "
            "for package %s instance '%s'.",
            mode_name,
            instance_id,
        )
        return

    # Run all jobs.
    package_functionality_list = clp_package.package_config.package_functionality_list
    _dispatch_test_jobs(package_functionality_list)

"""Provides utilities related to the test jobs for the CLP package."""

import logging
from pathlib import Path

from clp_py_utils.clp_config import CLP_DEFAULT_ARCHIVES_DIRECTORY_PATH

from tests.data.jobs.compression.compression_jobs import PACKAGE_COMPRESSION_JOBS
from tests.data.jobs.post_compression.admin_tools_jobs import PACKAGE_ADMIN_TOOLS_JOBS
from tests.data.jobs.post_compression.search_jobs import PACKAGE_SEARCH_JOBS
from tests.data.jobs.presto.presto_filters import PRESTO_FILTER_JOBS
from tests.utils.config import (
    PackageCompressionJob,
    PackageInstance,
    PackageJobList,
    PackagePostCompressionJob,
    PrestoFilterJob,
)
from tests.utils.package_utils import (
    run_package_compression_script,
    run_package_script,
    run_presto_filter,
)
from tests.utils.utils import unlink

logger = logging.getLogger(__name__)


def _matches_keyword(job_name: str, keyword_filter: str) -> bool:
    """Return True if this job should be included given the current -k filter."""
    if not keyword_filter:
        return True
    return keyword_filter.lower() in job_name.lower()


def _build_compression_jobs_list(
    mode_name: str,
    job_filter: str,
) -> list[PackageCompressionJob]:
    package_compression_jobs: list[PackageCompressionJob] = []
    for job_name, package_compression_job in PACKAGE_COMPRESSION_JOBS.items():
        if package_compression_job.mode == mode_name and _matches_keyword(job_name, job_filter):
            package_compression_jobs.append(package_compression_job)
    return package_compression_jobs


def _build_post_compression_jobs_list(
    mode_name: str,
    job_filter: str,
    package_compression_jobs: list[PackageCompressionJob],
) -> list[PackagePostCompressionJob]:
    package_post_compression_jobs: list[PackagePostCompressionJob] = []

    # Collect search jobs.
    for job_name, package_search_job in PACKAGE_SEARCH_JOBS.items():
        compression_job = package_search_job.compression_job
        if compression_job.mode == mode_name and _matches_keyword(job_name, job_filter):
            package_post_compression_jobs.append(package_search_job)
            if compression_job not in package_compression_jobs:
                package_compression_jobs.append(compression_job)

    # Collect admin-tools jobs.
    for job_name, package_admin_job in PACKAGE_ADMIN_TOOLS_JOBS.items():
        compression_job = package_admin_job.compression_job
        if compression_job.mode == mode_name and _matches_keyword(job_name, job_filter):
            package_post_compression_jobs.append(package_admin_job)
            if compression_job not in package_compression_jobs:
                package_compression_jobs.append(compression_job)

    return package_post_compression_jobs


def _build_presto_filter_jobs_list(
    mode_name: str,
    job_filter: str,
    package_compression_jobs: list[PackageCompressionJob],
) -> list[PrestoFilterJob]:
    presto_filter_jobs: list[PrestoFilterJob] = []

    for job_name, presto_filter_job in PRESTO_FILTER_JOBS.items():
        compression_job = presto_filter_job.compression_job
        if compression_job.mode == mode_name and _matches_keyword(job_name, job_filter):
            presto_filter_jobs.append(presto_filter_job)
            if compression_job not in package_compression_jobs:
                package_compression_jobs.append(compression_job)

    return presto_filter_jobs


def build_package_job_list(mode_name: str, job_filter: str) -> PackageJobList | None:
    """
    Builds the list of package jobs for this test run.

    :param mode_name:
    :param job_filter:
    :return: PackageJobList if there are jobs for this mode, None if not.
    """
    # Get compression jobs.
    package_compression_jobs: list[PackageCompressionJob] = _build_compression_jobs_list(
        mode_name, job_filter
    )

    # Add post-compression jobs (and any referenced compression jobs).
    package_post_compression_jobs: list[PackagePostCompressionJob] = (
        _build_post_compression_jobs_list(
            mode_name,
            job_filter,
            package_compression_jobs,
        )
    )

    # Add Presto filter jobs (and any referenced compression jobs).
    presto_filter_jobs: list[PrestoFilterJob] = _build_presto_filter_jobs_list(
        mode_name,
        job_filter,
        package_compression_jobs,
    )

    if (
        not package_compression_jobs
        and not package_post_compression_jobs
        and not presto_filter_jobs
    ):
        return None

    return PackageJobList(
        package_compression_jobs=package_compression_jobs,
        package_post_compression_jobs=package_post_compression_jobs,
        presto_filter_jobs=presto_filter_jobs,
    )


def _remove_directory_children(directory: Path) -> None:
    """Remove all children of `directory`."""
    if not directory.exists():
        return
    for child in directory.iterdir():
        unlink(child)


def dispatch_test_jobs(package_instance: PackageInstance) -> None:
    """
    Dispatches all package jobs for this package test run. Each compression job is
    followed immediately by the jobs that depend on it.

    :param request:
    :param package_instance:
    """
    package_config = package_instance.package_config
    package_job_list = package_config.package_job_list
    if package_job_list is None:
        err_msg = "Package job list is not configured for this package instance."
        raise RuntimeError(err_msg)

    package_compression_jobs = package_job_list.package_compression_jobs
    package_post_compression_jobs = package_job_list.package_post_compression_jobs
    presto_filter_jobs = package_job_list.presto_filter_jobs

    clp_package_dir = package_config.path_config.clp_package_dir
    archives_dir = clp_package_dir / CLP_DEFAULT_ARCHIVES_DIRECTORY_PATH

    # Perform initial cleanup in case there are logs currently in archives.
    _remove_directory_children(archives_dir)

    # For each compression job, run it, then its dependent jobs, then cleanup.
    for package_compression_job in package_compression_jobs:
        # Run the compression job.
        logger.info("Running job '%s'...", package_compression_job.job_name)
        run_package_compression_script(package_compression_job, package_instance)

        # Run the dependent package post-compression jobs.
        for package_post_compression_job in package_post_compression_jobs:
            if package_post_compression_job.compression_job is package_compression_job:
                logger.info("Running job '%s'...", package_post_compression_job.job_name)
                run_package_script(package_post_compression_job, package_instance)

        # Run the dependent Presto filter jobs.
        for presto_filter_job in presto_filter_jobs:
            if presto_filter_job.compression_job is package_compression_job:
                logger.info("Running job '%s'...", presto_filter_job.job_name)
                run_presto_filter(presto_filter_job)

        # Cleanup to prevent multiple compression jobs stored in archives.
        _remove_directory_children(archives_dir)

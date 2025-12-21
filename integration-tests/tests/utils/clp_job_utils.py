"""Provides utilities related to the test jobs for the CLP package."""

import logging

from tests.data.jobs.compression.compression_jobs import PACKAGE_COMPRESSION_JOBS
from tests.utils.config import (
    PackageCompressionJob,
    PackageJobList,
)

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

    if not package_compression_jobs:
        return None

    return PackageJobList(package_compression_jobs=package_compression_jobs)

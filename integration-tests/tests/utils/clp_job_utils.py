"""Provides utilities related to the test jobs for the CLP package."""

import logging

from tests.utils.config import (
    PackageCompressJob,
    PackageJobList,
    PackageSearchJob,
)

logger = logging.getLogger(__name__)


PACKAGE_COMPRESS_JOBS: dict[str, PackageCompressJob] = {
    "compress-postgresql": PackageCompressJob(
        job_name="compress-postgresql", mode="clp-json", file_path="/path/to/postgresql.log"
    ),
    "compress-cockroachdb": PackageCompressJob(
        job_name="compress-cockroachdb", mode="clp-json", file_path="/path/to/cockroachdb.log"
    ),
    "compress-hive-24hr": PackageCompressJob(
        job_name="compress-hive-24hr", mode="clp-text", file_path="/path/to/hive-24hr"
    ),
    # TODO: insert more compression jobs as needed...
}

PACKAGE_SEARCH_JOBS: dict[str, PackageSearchJob] = {
    "search-basic-hive": PackageSearchJob(
        job_name="search-basic-hive",
        mode="clp-text",
        package_compress_job=PACKAGE_COMPRESS_JOBS["compress-hive-24hr"],
        query="search query",
    ),
    "search-basic-postgresql": PackageSearchJob(
        job_name="search-basic-postgresql",
        mode="clp-json",
        package_compress_job=PACKAGE_COMPRESS_JOBS["compress-postgresql"],
        query="search query",
    ),
    "search-ignore-case": PackageSearchJob(
        job_name="search-ignore-case",
        mode="clp-json",
        package_compress_job=PACKAGE_COMPRESS_JOBS["compress-cockroachdb"],
        query="sEaRcH qUeRy",
    ),
    # TODO: insert more search jobs as needed...
}

# TODO: insert more job types as needed...


def _matches_keyword(job_name: str, keyword_filter: str) -> bool:
    """Return True if this job should be included given the current -k filter."""
    if not keyword_filter:
        return True
    return keyword_filter.lower() in job_name.lower()


def build_package_job_list(mode_name: str, job_filter: str) -> PackageJobList | None:
    """
    Builds the list of package jobs for this test run.

    :param mode_name:
    :param job_filter:
    :return: PackageJobList if there are jobs for this mode, None if not.
    """
    logger.info("Creating job list for mode %s (job filter: %s)", mode_name, job_filter)

    package_compress_jobs: list[PackageCompressJob] = []
    package_search_jobs: list[PackageSearchJob] = []

    for job_name, package_compress_job in PACKAGE_COMPRESS_JOBS.items():
        if package_compress_job.mode == mode_name and _matches_keyword(job_name, job_filter):
            package_compress_jobs.append(package_compress_job)

    for job_name, package_search_job in PACKAGE_SEARCH_JOBS.items():
        if package_search_job.mode == mode_name and _matches_keyword(job_name, job_filter):
            package_search_jobs.append(package_search_job)
            if package_search_job.package_compress_job not in package_compress_jobs:
                package_compress_jobs.append(package_search_job.package_compress_job)

    if not package_compress_jobs and not package_search_jobs:
        return None
    return PackageJobList(
        package_compress_jobs=package_compress_jobs,
        package_search_jobs=package_search_jobs,
    )


def _run_package_compress_jobs(jobs: list[PackageCompressJob]) -> None:
    job_descriptions = [f"{job.job_name}" for job in jobs]
    logger.info(
        "_run_package_compress_jobs: %d job(s): %s",
        len(jobs),
        job_descriptions,
    )
    # TODO: write this.
    assert True


def _run_package_search_jobs(jobs: list[PackageSearchJob]) -> None:
    job_descriptions = [f"{job.job_name}" for job in jobs]
    logger.info(
        "_run_package_search_jobs: %d job(s): %s",
        len(jobs),
        job_descriptions,
    )
    # TODO: write this.
    assert True


def dispatch_test_jobs(jobs_list: PackageJobList) -> None:
    """
    Dispatches all the package jobs in `job_list` for this package test run.

    :param jobs_list:
    """
    logger.info("dispatch_test_jobs")
    if jobs_list.package_compress_jobs:
        _run_package_compress_jobs(jobs_list.package_compress_jobs)
    if jobs_list.package_search_jobs:
        _run_package_search_jobs(jobs_list.package_search_jobs)

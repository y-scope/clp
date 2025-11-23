"""Provides utilities related to the test jobs for the CLP package."""

import logging
from pathlib import Path

import pytest

from tests.utils.config import (
    PackageCompressJob,
    PackageInstance,
    PackageJobList,
    PackageSearchJob,
)
from tests.utils.package_utils import compress_with_clp_package

logger = logging.getLogger(__name__)


PACKAGE_COMPRESS_JOBS: dict[str, PackageCompressJob] = {
    "compress-postgresql": PackageCompressJob(
        job_name="compress-postgresql",
        log_fixture_name="postgresql",
        mode="clp-json",
        log_format="json",
        unstructured=False,
        dataset_name="postgresql",
        timestamp_key="timestamp",
    ),
    "compress-spark-event-logs": PackageCompressJob(
        job_name="compress-spark-event-logs",
        log_fixture_name="spark_event_logs",
        mode="clp-json",
        log_format="json",
        unstructured=False,
        dataset_name="spark-event-logs",
        timestamp_key="Timestamp",
    ),
    "compress-default-dataset": PackageCompressJob(
        job_name="compress-default-dataset",
        log_fixture_name="postgresql",
        mode="clp-json",
        log_format="json",
        unstructured=False,
        timestamp_key="timestamp",
    ),
    "compress-tagged-data-spark": PackageCompressJob(
        job_name="compress-tagged-data-spark",
        log_fixture_name="spark_event_logs",
        mode="clp-json",
        log_format="json",
        unstructured=False,
        dataset_name="tagged_data",
        timestamp_key="Timestamp",
        subpath=Path("spark-event-logs") / "app-20211007095008-0000",
        tags=[
            "tag1",
        ],
    ),
    "compress-hive-24hr": PackageCompressJob(
        job_name="compress-hive-24hr",
        log_fixture_name="hive_24hr",
        mode="clp-text",
        log_format="text",
        unstructured=True,
    ),
    "compress-tagged-data-hive": PackageCompressJob(
        job_name="compress-tagged-data-hive",
        log_fixture_name="hive_24hr",
        mode="clp-text",
        log_format="text",
        unstructured=True,
        subpath=(
            Path("hive-24hr")
            / "i-0ac90a05"
            / "application_1427088391284_0001"
            / "container_1427088391284_0001_01_000124"
            / "syslog"
        ),
        tags=[
            "tag1",
        ],
    ),
    # Insert more compression jobs here as needed.
}

PACKAGE_SEARCH_JOBS: dict[str, PackageSearchJob] = {
    "search-basic-postgresql": PackageSearchJob(
        job_name="search-basic-postgresql",
        mode="clp-json",
        package_compress_job=PACKAGE_COMPRESS_JOBS["compress-postgresql"],
        query="search query",
    ),
    "search-ignore-case": PackageSearchJob(
        job_name="search-ignore-case",
        mode="clp-json",
        package_compress_job=PACKAGE_COMPRESS_JOBS["compress-postgresql"],
        query="sEaRcH qUeRy",
    ),
    "search-basic-hive": PackageSearchJob(
        job_name="search-basic-hive",
        mode="clp-text",
        package_compress_job=PACKAGE_COMPRESS_JOBS["compress-hive-24hr"],
        query="search query",
    ),
    # Insert more search jobs here as needed.
}


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
    logger.debug("Creating job list for mode %s (job filter: %s)", mode_name, job_filter)

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


def _run_package_compress_jobs(
    request: pytest.FixtureRequest,
    package_instance: PackageInstance,
) -> None:
    """
    Run all the package compress jobs for this test run.

    :param package_instance:
    :param request:
    """
    package_job_list = package_instance.package_config.package_job_list
    if package_job_list is None:
        err_msg = "Package job list is not configured for this package instance."
        raise RuntimeError(err_msg)

    compress_jobs = package_job_list.package_compress_jobs
    for compress_job in compress_jobs:
        compress_with_clp_package(request, compress_job, package_instance)


def _run_package_search_jobs(
    package_instance: PackageInstance,
) -> None:
    """
    Run all the package search jobs for this test run.

    :param package_instance:
    """
    package_job_list = package_instance.package_config.package_job_list
    if package_job_list is None:
        err_msg = "Package job list is not configured for this package instance."
        raise RuntimeError(err_msg)

    assert True


def dispatch_test_jobs(request: pytest.FixtureRequest, package_instance: PackageInstance) -> None:
    """
    Dispatches all the package jobs in `job_list` for this package test run.

    :param jobs_list:
    """
    logger.debug("Dispatching test jobs.")

    jobs_list = package_instance.package_config.package_job_list
    if jobs_list is None:
        return

    if jobs_list.package_compress_jobs:
        _run_package_compress_jobs(request, package_instance)
    if jobs_list.package_search_jobs:
        _run_package_search_jobs(package_instance)

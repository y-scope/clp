"""Provides utilities related to the test jobs for the CLP package."""

import logging
from pathlib import Path

import pytest
from clp_py_utils.clp_config import CLP_DEFAULT_ARCHIVES_DIRECTORY_PATH

from tests.utils.config import (
    PackageCompressJob,
    PackageInstance,
    PackageJobList,
    PackageSearchJob,
    PrestoFilterJob,
)
from tests.utils.package_utils import (
    compress_with_clp_package,
    run_presto_filter,
    search_with_clp_package,
    resolve_path_env_var,
)
from tests.utils.utils import unlink

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
}

PACKAGE_SEARCH_JOBS: dict[str, PackageSearchJob] = {
    "search-basic-postgresql": PackageSearchJob(
        job_name="search-basic-postgresql",
        mode="clp-json",
        package_compress_job=PACKAGE_COMPRESS_JOBS["compress-postgresql"],
        ignore_case=False,
        count=False,
        wildcard_query='message: "next transaction ID: 735; next OID: 16388"',
        desired_result=(
            '{"timestamp":"2023-03-27 00:26:35.873","pid":7813,'
            '"session_id":"64211afb.1e85",'
            '"line_num":4,"session_start":"2023-03-27 00:26:35 EDT","txid":0,'
            '"error_severity":"DEBUG","message":"next transaction ID: 735; '
            'next OID: 16388",'
            '"backend_type":"startup","query_id":0}\n'
        ),
    ),
    "search-by-file-path": PackageSearchJob(
        job_name="search-by-file-path",
        mode="clp-json",
        package_compress_job=PACKAGE_COMPRESS_JOBS["compress-postgresql"],
        ignore_case=False,
        count=False,
        wildcard_query='message: "next transaction ID: 735; next OID: 16388"',
        desired_result=(
            '{"timestamp":"2023-03-27 00:26:35.873","pid":7813,'
            '"session_id":"64211afb.1e85",'
            '"line_num":4,"session_start":"2023-03-27 00:26:35 EDT","txid":0,'
            '"error_severity":"DEBUG","message":"next transaction ID: 735; '
            'next OID: 16388",'
            '"backend_type":"startup","query_id":0}\n'
        ),
        file_subpath=Path("postgresql") / "postgresql.log",
    ),
    "search-ignore-case": PackageSearchJob(
        job_name="search-ignore-case",
        mode="clp-json",
        package_compress_job=PACKAGE_COMPRESS_JOBS["compress-postgresql"],
        ignore_case=True,
        count=False,
        wildcard_query='message: "nExT tRaNsAcTiOn Id: 735; nExT OiD: 16388"',
        desired_result=(
            '{"timestamp":"2023-03-27 00:26:35.873","pid":7813,'
            '"session_id":"64211afb.1e85",'
            '"line_num":4,"session_start":"2023-03-27 00:26:35 EDT","txid":0,'
            '"error_severity":"DEBUG","message":"next transaction ID: 735; '
            'next OID: 16388",'
            '"backend_type":"startup","query_id":0}\n'
        ),
    ),
    "search-count-results": PackageSearchJob(
        job_name="search-count-results",
        mode="clp-json",
        package_compress_job=PACKAGE_COMPRESS_JOBS["compress-postgresql"],
        ignore_case=False,
        count=True,
        wildcard_query='message: "next transaction ID: 735; next OID: 16388"',
        desired_result="tags: [] count: 1\n",
    ),
    "search-count-by-time": PackageSearchJob(
        job_name="search-count-by-time",
        mode="clp-json",
        package_compress_job=PACKAGE_COMPRESS_JOBS["compress-postgresql"],
        ignore_case=False,
        count=False,
        count_by_time=10,
        wildcard_query='message: "next transaction ID: 735; next OID: 16388"',
        desired_result="timestamp: 1679876795870 count: 1\n",
    ),
    "search-tagged-data": PackageSearchJob(
        job_name="search-tagged-data",
        mode="clp-json",
        package_compress_job=PACKAGE_COMPRESS_JOBS["compress-tagged-data-spark"],
        ignore_case=False,
        count=False,
        wildcard_query='"Block Manager ID": {"Port": 38647}',
        desired_result=(
            '{"Event":"SparkListenerBlockManagerAdded","Block Manager ID":{"Executor ID":"1",'
            '"Host":"10.1.0.17","Port":38647},"Maximum Memory":2101975449,'
            '"Timestamp":1633614610933,"Maximum Onheap Memory":2101975449,'
            '"Maximum Offheap Memory":0}\n'
        ),
    ),
    "search-time-range": PackageSearchJob(
        job_name="search-time-range",
        mode="clp-json",
        package_compress_job=PACKAGE_COMPRESS_JOBS["compress-tagged-data-spark"],
        ignore_case=False,
        count=False,
        wildcard_query='"Block Manager ID": {"Port": 38647}',
        desired_result=(
            '{"Event":"SparkListenerBlockManagerAdded","Block Manager ID":{"Executor ID":"1",'
            '"Host":"10.1.0.17","Port":38647},"Maximum Memory":2101975449,'
            '"Timestamp":1633614610933,"Maximum Onheap Memory":2101975449,'
            '"Maximum Offheap Memory":0}\n'
        ),
        begin_time=1633614610931,
        end_time=1633614610950,
    ),
    "search-basic-hive": PackageSearchJob(
        job_name="search-basic-hive",
        mode="clp-text",
        package_compress_job=PACKAGE_COMPRESS_JOBS["compress-hive-24hr"],
        ignore_case=False,
        count=False,
        wildcard_query="Shuffle@79ec394e",
        desired_result=(
            "2015-03-23 05:40:08,988 INFO [main] "
            "org.apache.hadoop.mapred.ReduceTask: "
            "Using ShuffleConsumerPlugin: "
            "org.apache.hadoop.mapreduce.task.reduce.Shuffle@79ec394e\n"
        ),
    ),
    "search-by-file-path-text": PackageSearchJob(
        job_name="search-by-file-path-text",
        mode="clp-text",
        package_compress_job=PACKAGE_COMPRESS_JOBS["compress-hive-24hr"],
        ignore_case=False,
        count=False,
        wildcard_query="Shuffle@79ec394e",
        desired_result=(
            "2015-03-23 05:40:08,988 INFO [main] "
            "org.apache.hadoop.mapred.ReduceTask: "
            "Using ShuffleConsumerPlugin: "
            "org.apache.hadoop.mapreduce.task.reduce.Shuffle@79ec394e\n"
        ),
        file_subpath=(
            Path("hive-24hr")
            / "i-0ac90a05"
            / "application_1427088391284_0001"
            / "container_1427088391284_0001_01_000124"
            / "syslog"
        ),
    ),
    "search-ignore-case-text": PackageSearchJob(
        job_name="search-ignore-case-text",
        mode="clp-text",
        package_compress_job=PACKAGE_COMPRESS_JOBS["compress-hive-24hr"],
        ignore_case=True,
        count=False,
        wildcard_query="Shuffle@79ec394e",
        desired_result=(
            "2015-03-23 05:40:08,988 INFO [main] "
            "org.apache.hadoop.mapred.ReduceTask: "
            "Using ShuffleConsumerPlugin: "
            "org.apache.hadoop.mapreduce.task.reduce.Shuffle@79ec394e\n"
        ),
    ),
    "search-count-results-text": PackageSearchJob(
        job_name="search-count-results-text",
        mode="clp-text",
        package_compress_job=PACKAGE_COMPRESS_JOBS["compress-hive-24hr"],
        ignore_case=False,
        count=True,
        wildcard_query="Shuffle@79ec394e",
        desired_result="tags: [] count: 1\n",
    ),
    "search-count-by-time-text": PackageSearchJob(
        job_name="search-count-by-time-text",
        mode="clp-text",
        package_compress_job=PACKAGE_COMPRESS_JOBS["compress-hive-24hr"],
        ignore_case=False,
        count=False,
        wildcard_query="Shuffle@79ec394e",
        desired_result="timestamp: 1427089208900 count: 1\n",
        count_by_time=100,
    ),
    "search-tagged-data-text": PackageSearchJob(
        job_name="search-tagged-data-text",
        mode="clp-text",
        package_compress_job=PACKAGE_COMPRESS_JOBS["compress-tagged-data-hive"],
        ignore_case=False,
        count=False,
        wildcard_query="Shuffle@79ec394e",
        desired_result=(
            "2015-03-23 05:40:08,988 INFO [main] "
            "org.apache.hadoop.mapred.ReduceTask: "
            "Using ShuffleConsumerPlugin: "
            "org.apache.hadoop.mapreduce.task.reduce.Shuffle@79ec394e\n"
        ),
    ),
    "search-time-range-text": PackageSearchJob(
        job_name="search-tagged-data-text",
        mode="clp-text",
        package_compress_job=PACKAGE_COMPRESS_JOBS["compress-hive-24hr"],
        ignore_case=False,
        count=False,
        wildcard_query="Shuffle@79ec394e",
        desired_result=(
            "2015-03-23 05:40:08,988 INFO [main] "
            "org.apache.hadoop.mapred.ReduceTask: "
            "Using ShuffleConsumerPlugin: "
            "org.apache.hadoop.mapreduce.task.reduce.Shuffle@79ec394e\n"
        ),
        begin_time=0,
        end_time=1761165583429,
    ),
}

PRESTO_FILTER_JOBS: dict[str, PrestoFilterJob] = {
    "filter-describe-postgresql": PrestoFilterJob(
        job_name="filter-describe-postgresql",
        mode="clp-json-presto",
        package_compress_job=PACKAGE_COMPRESS_JOBS["compress-postgresql"],
        filter="DESCRIBE postgresql;",
        ground_truth_file=(
            resolve_path_env_var("TEST_DATA_DIR") /
            "filter-describe-postgresql.jsonl"
        ),
    ),
    "filter-select-distinct-postgresql": PrestoFilterJob(
        job_name="filter-select-distinct-postgresql",
        mode="clp-json-presto",
        package_compress_job=PACKAGE_COMPRESS_JOBS["compress-postgresql"],
        filter="SELECT DISTINCT error_severity FROM postgresql;",
        ground_truth_file=(
            resolve_path_env_var("TEST_DATA_DIR") /
            "filter-select-distinct-postgresql.jsonl"
        ),
    ),
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
    presto_filter_jobs: list[PrestoFilterJob] = []

    for job_name, package_compress_job in PACKAGE_COMPRESS_JOBS.items():
        if package_compress_job.mode == mode_name and _matches_keyword(job_name, job_filter):
            package_compress_jobs.append(package_compress_job)

    for job_name, package_search_job in PACKAGE_SEARCH_JOBS.items():
        if package_search_job.mode == mode_name and _matches_keyword(job_name, job_filter):
            package_search_jobs.append(package_search_job)
            if package_search_job.package_compress_job not in package_compress_jobs:
                package_compress_jobs.append(package_search_job.package_compress_job)

    for job_name, presto_filter_job in PRESTO_FILTER_JOBS.items():
        if presto_filter_job.mode == mode_name and _matches_keyword(job_name, job_filter):
            presto_filter_jobs.append(presto_filter_job)
            if presto_filter_job.package_compress_job not in package_compress_jobs:
                package_compress_jobs.append(presto_filter_job.package_compress_job)

    if not package_compress_jobs and not package_search_jobs and not presto_filter_jobs:
        return None
    return PackageJobList(
        package_compress_jobs=package_compress_jobs,
        package_search_jobs=package_search_jobs,
        presto_filter_jobs=presto_filter_jobs,
    )


def _remove_directory_children(directory: Path) -> None:
    """Remove all children of `directory`."""
    if directory.exists():
        for child in directory.iterdir():
            unlink(child)


def dispatch_test_jobs(request: pytest.FixtureRequest, package_instance: PackageInstance) -> None:
    """
    Dispatches all the package jobs in `job_list` for this package test run. Each compression job is
    followed immediately by the search jobs that depend on it.

    :param request:
    :param package_instance:
    """
    package_job_list = package_instance.package_config.package_job_list
    if package_job_list is None:
        err_msg = "Package job list is not configured for this package instance."
        raise RuntimeError(err_msg)

    package_compress_jobs = package_job_list.package_compress_jobs
    package_search_jobs = package_job_list.package_search_jobs
    presto_filter_jobs = package_job_list.presto_filter_jobs
    clp_package_dir = package_instance.package_config.path_config.clp_package_dir

    # Perform initial cleanup just in case there are any logs currently in archives.
    archives_dir = clp_package_dir / CLP_DEFAULT_ARCHIVES_DIRECTORY_PATH
    _remove_directory_children(archives_dir)

    # For each compression job, run it, then its dependent jobs, then cleanup.
    for package_compress_job in package_compress_jobs:
        # Run the compression job.
        compress_with_clp_package(request, package_compress_job, package_instance)

        # Run the search jobs from the list that depend on the compression job.
        for package_search_job in package_search_jobs:
            if package_search_job.package_compress_job is package_compress_job:
                search_with_clp_package(request, package_search_job, package_instance)

        for presto_filter_job in presto_filter_jobs:
            if presto_filter_job.package_compress_job is package_compress_job:
                run_presto_filter(presto_filter_job)

        # Cleanup to prevent multiple compression jobs stored in archives.
        _remove_directory_children(archives_dir)

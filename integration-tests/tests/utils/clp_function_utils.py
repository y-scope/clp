"""Provides utilities related to the testable functionalities of the clp package."""

from tests.utils.config import (
    CompressJob,
    SearchJob,
)

COMPRESS_JOBS: dict[str, CompressJob] = {
    "compress-postgresql": CompressJob(
        job_name="compress-postgresql", mode="clp-json", file_path="/path/to/postgresql.log"
    ),
    "compress-cockroachdb": CompressJob(
        job_name="compress-cockroachdb", mode="clp-json", file_path="/path/to/cockroachdb.log"
    ),
    "compress-hive-24hr": CompressJob(
        job_name="compress-hive-24hr", mode="clp-text", file_path="/path/to/hive-24hr"
    ),
    # TODO: insert more compression jobs as needed...
}


SEARCH_JOBS: dict[str, SearchJob] = {
    "search-basic-hive": SearchJob(
        job_name="search-basic-hive",
        mode="clp-text",
        compress_job=COMPRESS_JOBS["compress-hive-24hr"],
        query="search query",
    ),
    "search-basic-postgresql": SearchJob(
        job_name="search-basic-postgresql",
        mode="clp-json",
        compress_job=COMPRESS_JOBS["compress-postgresql"],
        query="search query",
    ),
    "search-ignore-case": SearchJob(
        job_name="search-ignore-case",
        mode="clp-json",
        compress_job=COMPRESS_JOBS["compress-cockroachdb"],
        query="sEaRcH qUeRy",
    ),
    # TODO: insert more search jobs as needed...
}

# TODO: insert more job types as needed...

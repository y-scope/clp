"""Provides utilities related to the test jobs for the CLP package."""

from tests.utils.config import (
    PackageCompressJob,
    PackageSearchJob,
)

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

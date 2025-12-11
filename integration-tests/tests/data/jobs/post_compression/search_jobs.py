"""Post-compression search job definitions for CLP package tests."""

from pathlib import Path

from tests.data.jobs.compression.compression_jobs import PACKAGE_COMPRESSION_JOBS
from tests.utils.config import PackagePostCompressionJob

PACKAGE_SEARCH_JOBS: dict[str, PackagePostCompressionJob] = {
    "search-json-multifile-basic": PackagePostCompressionJob(
        job_name="search-json-multifile-basic",
        compression_job=PACKAGE_COMPRESSION_JOBS["compress-json-multifile"],
        script_path=Path("sbin") / "search.sh",
        flags={
            "--dataset": "json_multifile",
            "--raw": None,
        },
        args=[
            "subsystem: GUIDANCE",
        ],
        output_ground_truth_file=Path("search-json-multifile-basic.jsonl"),
    ),
}

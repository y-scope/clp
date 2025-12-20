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
        output_ground_truth_file=Path("search") / "search-json-multifile-basic.jsonl",
    ),
    "search-json-multifile-ignorecase": PackagePostCompressionJob(
        job_name="search-json-multifile-ignorecase",
        compression_job=PACKAGE_COMPRESSION_JOBS["compress-json-multifile"],
        script_path=Path("sbin") / "search.sh",
        flags={
            "--dataset": "json_multifile",
            "--ignore-case": None,
            "--raw": None,
        },
        args=[
            "subsystem: GuIdAnCe",
        ],
        output_ground_truth_file=Path("search") / "search-json-multifile-ignorecase.jsonl",
    ),
    "search-json-multifile-countresults": PackagePostCompressionJob(
        job_name="search-json-multifile-countresults",
        compression_job=PACKAGE_COMPRESSION_JOBS["compress-json-multifile"],
        script_path=Path("sbin") / "search.sh",
        flags={
            "--dataset": "json_multifile",
            "--count": None,
            "--raw": None,
        },
        args=[
            "subsystem: GUIDANCE",
        ],
        output_ground_truth_file=Path("search") / "search-json-multifile-countresults.jsonl",
    ),
    "search-json-multifile-countbytime": PackagePostCompressionJob(
        job_name="search-json-multifile-countbytime",
        compression_job=PACKAGE_COMPRESSION_JOBS["compress-json-multifile"],
        script_path=Path("sbin") / "search.sh",
        flags={
            "--dataset": "json_multifile",
            "--count-by-time": 10,
            "--raw": None,
        },
        args=[
            "subsystem: GUIDANCE",
        ],
        output_ground_truth_file=Path("search") / "search-json-multifile-countbytime.jsonl",
    ),
    "search-json-multifile-timerange": PackagePostCompressionJob(
        job_name="search-json-multifile-timerange",
        compression_job=PACKAGE_COMPRESSION_JOBS["compress-json-multifile"],
        script_path=Path("sbin") / "search.sh",
        flags={
            "--dataset": "json_multifile",
            "--begin-time": 1310138944000,
            "--end-time": 1310140374613,
            "--raw": None,
        },
        args=[
            "subsystem: GUIDANCE",
        ],
        output_ground_truth_file=Path("search") / "search-json-multifile-timerange.jsonl",
    ),
    "search-json-multifile-tagged": PackagePostCompressionJob(
        job_name="search-json-multifile-tagged",
        compression_job=PACKAGE_COMPRESSION_JOBS["compress-json-multifile-tagged"],
        script_path=Path("sbin") / "search.sh",
        flags={
            "--dataset": "json_multifile_tagged",
            "--tags": "tag1",
            "--raw": None,
        },
        args=[
            "subsystem: GUIDANCE",
        ],
        output_ground_truth_file=Path("search") / "search-json-multifile-tagged.jsonl",
    ),
}

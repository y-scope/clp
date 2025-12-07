"""Compression job definitions for CLP package tests."""

from pathlib import Path

from tests.utils.config import PackageCompressionJob

PACKAGE_COMPRESSION_JOBS: dict[str, PackageCompressionJob] = {
    "compress-json-singlefile-1": PackageCompressionJob(
        job_name="compress-json-singlefile-1",
        mode="clp-json",
        script_path=Path("sbin") / "compress.sh",
        log_path=Path("json-singlefile-1") / "logs",
        flags={
            "--timestamp-key": "timestamp",
            "--dataset": "json_singlefile_1",
        },
        args=None,
    ),
    "compress-text-singlefile": PackageCompressionJob(
        job_name="compress-text-singlefile",
        mode="clp-text",
        script_path=Path("sbin") / "compress.sh",
        log_path=Path("text-singlefile") / "logs",
        flags=None,
        args=None,
    ),
    "compress-json-singlefile-2": PackageCompressionJob(
        job_name="compress-json-singlefile-2",
        mode="clp-json-presto",
        script_path=Path("sbin") / "compress.sh",
        log_path=Path("json-singlefile-2") / "logs",
        flags={
            "--timestamp-key": "timestamp",
            "--dataset": "json_singlefile_2",
        },
        args=None,
    ),
}

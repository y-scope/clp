"""Compression job definitions for CLP package tests."""

from pathlib import Path

from tests.utils.config import PackageCompressionJob

PACKAGE_COMPRESSION_JOBS: dict[str, PackageCompressionJob] = {
    "compress-json-multifile": PackageCompressionJob(
        job_name="compress-json-multifile",
        mode="clp-json",
        script_path=Path("sbin") / "compress.sh",
        log_path=Path("json-multifile") / "datasets",
        flags={
            "--timestamp-key": "timestamp",
            "--dataset": "json_multifile",
        },
        args=None,
    ),
}

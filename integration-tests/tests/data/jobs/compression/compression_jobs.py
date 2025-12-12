"""Compression job definitions for CLP package tests."""

from pathlib import Path

from tests.utils.config import PackageCompressionJob

PACKAGE_COMPRESSION_JOBS: dict[str, PackageCompressionJob] = {
    "compress-json-multifile": PackageCompressionJob(
        job_name="compress-json-multifile",
        mode="clp-json",
        script_path=Path("sbin") / "compress.sh",
        log_path=Path("json-multifile") / "logs",
        flags={
            "--timestamp-key": "timestamp",
            "--dataset": "json_multifile",
        },
        args=None,
    ),
    "compress-json-multifile-default-dataset": PackageCompressionJob(
        job_name="compress-json-multifile-default-dataset",
        mode="clp-json",
        script_path=Path("sbin") / "compress.sh",
        log_path=Path("json-multifile") / "logs",
        flags={
            "--timestamp-key": "timestamp",
        },
        args=None,
    ),
    "compress-json-multifile-tagged": PackageCompressionJob(
        job_name="compress-json-multifile-tagged",
        mode="clp-json",
        script_path=Path("sbin") / "compress.sh",
        log_path=Path("json-multifile") / "logs" / "sts-135-2011-07-08.jsonl",
        flags={
            "--timestamp-key": "timestamp",
            "--dataset": "json_multifile_tagged",
            "--tags": "tag1",
        },
        args=None,
    ),
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
    "compress-json-singlefile-2": PackageCompressionJob(
        job_name="compress-json-singlefile-2",
        mode="clp-presto",
        script_path=Path("sbin") / "compress.sh",
        log_path=Path("json-singlefile-2") / "logs",
        flags={
            "--timestamp-key": "timestamp",
            "--dataset": "json_singlefile_2",
        },
        args=None,
    ),
    "compress-text-multifile": PackageCompressionJob(
        job_name="compress-text-multifile",
        mode="clp-text",
        script_path=Path("sbin") / "compress.sh",
        log_path=Path("text-multifile") / "logs",
        flags=None,
        args=None,
    ),
    "compress-text-multifile-tagged": PackageCompressionJob(
        job_name="compress-text-multifile-tagged",
        mode="clp-text",
        script_path=Path("sbin") / "compress.sh",
        log_path=Path("text-multifile") / "logs" / "apollo-17_day01.txt",
        flags={
            "--tags": "tag1",
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
}

import os
import shutil
import subprocess

import pytest
from tests.utils.config import (
    PackageConfig,
    TestLogs,
)


@pytest.fixture(scope="session")
def hive_24hr(request, package_config: PackageConfig) -> TestLogs:
    return _download_and_extract_dataset(
        request=request,
        package_config=package_config,
        name="hive-24hr",
        tarball_url="https://zenodo.org/records/7094921/files/hive-24hr.tar.gz?download=1",
    )


@pytest.fixture(scope="session")
def elasticsearch(request, package_config: PackageConfig) -> TestLogs:
    return _download_and_extract_dataset(
        request=request,
        package_config=package_config,
        name="elasticsearch",
        tarball_url="https://zenodo.org/records/10516227/files/elasticsearch.tar.gz?download=1",
    )


@pytest.fixture(scope="session")
def spark_event_logs(request, package_config: PackageConfig) -> TestLogs:
    return _download_and_extract_dataset(
        request=request,
        package_config=package_config,
        name="spark-event-logs",
        tarball_url="https://zenodo.org/records/10516346/files/spark-event-logs.tar.gz?download=1",
    )


@pytest.fixture(scope="session")
def postgresql(request, package_config: PackageConfig) -> TestLogs:
    return _download_and_extract_dataset(
        request=request,
        package_config=package_config,
        name="postgresql",
        tarball_url="https://zenodo.org/records/10516402/files/postgresql.tar.gz?download=1",
    )


def _download_and_extract_dataset(
    request, package_config: PackageConfig, name: str, tarball_url: str
) -> TestLogs:
    test_logs = TestLogs.create(
        package_config=package_config,
        name=name,
        tarball_url=tarball_url,
    )
    if request.config.cache.get(name, False):
        print(f"Test logs `{name}` is up-to-date. Skipping download.")
        return test_logs

    try:
        # fmt: off
        curl_cmds = [
            "curl",
            "--fail",
            "--location",
            "--output", str(test_logs.tarball_path),
            "--show-error",
            tarball_url,
        ]
        # fmt: on
        subprocess.run(curl_cmds, check=True)
        shutil.unpack_archive(str(test_logs.tarball_path), str(test_logs.extraction_dir))
    except Exception as e:
        raise RuntimeError(f"Failed to download and extract dataset `{name}`: {e}")

    print(f"Downloaded and extracted uncompressed logs for dataset `{name}`.")
    request.config.cache.set(name, True)
    return test_logs

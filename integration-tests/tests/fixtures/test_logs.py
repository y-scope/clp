import os
import shutil
import subprocess

import pytest
from tests.utils.config import (
    TestConfig,
    TestLogs,
)


@pytest.fixture(scope="session")
def hive_24hr(request, test_config: TestConfig) -> TestLogs:
    return _download_and_extract_dataset(
        request=request,
        test_config=test_config,
        name="hive-24hr",
        tarball_url="https://zenodo.org/records/7094921/files/hive-24hr.tar.gz?download=1",
    )


@pytest.fixture(scope="session")
def elasticsearch(request, test_config: TestConfig) -> TestLogs:
    return _download_and_extract_dataset(
        request=request,
        test_config=test_config,
        name="elasticsearch",
        tarball_url="https://zenodo.org/records/10516227/files/elasticsearch.tar.gz?download=1",
    )


@pytest.fixture(scope="session")
def spark_event_logs(request, test_config: TestConfig) -> TestLogs:
    return _download_and_extract_dataset(
        request=request,
        test_config=test_config,
        name="spark-event-logs",
        tarball_url="https://zenodo.org/records/10516346/files/spark-event-logs.tar.gz?download=1",
    )


@pytest.fixture(scope="session")
def postgresql(request, test_config: TestConfig) -> TestLogs:
    return _download_and_extract_dataset(
        request=request,
        test_config=test_config,
        name="postgresql",
        tarball_url="https://zenodo.org/records/10516402/files/postgresql.tar.gz?download=1",
    )


def _download_and_extract_dataset(
    request, test_config: TestConfig, name: str, tarball_url: str
) -> TestLogs:
    test_logs = TestLogs.create(
        name=name,
        tarball_url=tarball_url,
        test_config=test_config,
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
        shutil.unpack_archive(test_logs.tarball_path, test_logs.extraction_dir)
    except Exception as e:
        raise RuntimeError(f"Failed to download and extract dataset `{name}`: {e}")

    print(f"Downloaded and extracted uncompressed logs for dataset `{name}`.")
    request.config.cache.set(name, True)
    return test_logs

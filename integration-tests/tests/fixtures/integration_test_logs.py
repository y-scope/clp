import os
import shutil
import subprocess

import pytest
from tests.utils.config import (
    IntegrationTestConfig,
    IntegrationTestLogs,
)


@pytest.fixture(scope="session")
def hive_24hr(request, integration_test_config: IntegrationTestConfig) -> IntegrationTestLogs:
    return _download_and_extract_dataset(
        request=request,
        integration_test_config=integration_test_config,
        name="hive-24hr",
        tarball_url="https://zenodo.org/records/7094921/files/hive-24hr.tar.gz?download=1",
    )


@pytest.fixture(scope="session")
def elasticsearch(request, integration_test_config: IntegrationTestConfig) -> IntegrationTestLogs:
    return _download_and_extract_dataset(
        request=request,
        integration_test_config=integration_test_config,
        name="elasticsearch",
        tarball_url="https://zenodo.org/records/10516227/files/elasticsearch.tar.gz?download=1",
    )


@pytest.fixture(scope="session")
def spark_event_logs(
    request, integration_test_config: IntegrationTestConfig
) -> IntegrationTestLogs:
    return _download_and_extract_dataset(
        request=request,
        integration_test_config=integration_test_config,
        name="spark-event-logs",
        tarball_url="https://zenodo.org/records/10516346/files/spark-event-logs.tar.gz?download=1",
    )


@pytest.fixture(scope="session")
def postgresql(request, integration_test_config: IntegrationTestConfig) -> IntegrationTestLogs:
    return _download_and_extract_dataset(
        request=request,
        integration_test_config=integration_test_config,
        name="postgresql",
        tarball_url="https://zenodo.org/records/10516402/files/postgresql.tar.gz?download=1",
    )


def _download_and_extract_dataset(
    request, integration_test_config: IntegrationTestConfig, name: str, tarball_url: str
) -> IntegrationTestLogs:
    integration_test_logs = IntegrationTestLogs(
        name=name,
        tarball_url=tarball_url,
        integration_test_config=integration_test_config,
    )
    if request.config.cache.get(name, False):
        print(f"Test logs `{name}` is up-to-date. Skipping download.")
        return integration_test_logs

    try:
        # fmt: off
        curl_cmds = [
            "curl",
            "--fail",
            "--location",
            "--output", str(integration_test_logs.tarball_path),
            "--show-error",
            tarball_url,
        ]
        # fmt: on
        subprocess.run(curl_cmds, check=True)
        shutil.unpack_archive(
            integration_test_logs.tarball_path, integration_test_logs.extraction_dir
        )
    except Exception as e:
        raise RuntimeError(f"Failed to download and extract dataset `{name}`: {e}")

    print(f"Downloaded and extracted uncompressed logs for dataset `{name}`.")
    request.config.cache.set(name, True)
    return integration_test_logs

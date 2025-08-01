import os
import shutil
import subprocess

import pytest
from tests.utils.config import (
    DatasetLogs,
    PackageConfig,
)


@pytest.fixture(scope="session")
def hive_24hr(request, package_config: PackageConfig) -> DatasetLogs:
    return _download_and_extract_dataset(
        request=request,
        package_config=package_config,
        dataset_name="hive-24hr",
        tarball_url="https://zenodo.org/records/7094921/files/hive-24hr.tar.gz?download=1",
    )


@pytest.fixture(scope="session")
def elasticsearch(request, package_config: PackageConfig) -> DatasetLogs:
    return _download_and_extract_dataset(
        request=request,
        package_config=package_config,
        dataset_name="elasticsearch",
        tarball_url="https://zenodo.org/records/10516227/files/elasticsearch.tar.gz?download=1",
    )


@pytest.fixture(scope="session")
def spark_event_logs(request, package_config: PackageConfig) -> DatasetLogs:
    return _download_and_extract_dataset(
        request=request,
        package_config=package_config,
        dataset_name="spark-event-logs",
        tarball_url="https://zenodo.org/records/10516346/files/spark-event-logs.tar.gz?download=1",
    )


@pytest.fixture(scope="session")
def postgresql(request, package_config: PackageConfig) -> DatasetLogs:
    return _download_and_extract_dataset(
        request=request,
        package_config=package_config,
        dataset_name="postgresql",
        tarball_url="https://zenodo.org/records/10516402/files/postgresql.tar.gz?download=1",
    )


def _download_and_extract_dataset(
    request, package_config: PackageConfig, dataset_name: str, tarball_url: str
) -> DatasetLogs:
    dataset_logs = DatasetLogs.create(
        package_config=package_config,
        dataset_name=dataset_name,
        tarball_url=tarball_url,
    )
    if request.config.cache.get(dataset_name, False):
        print(f"Uncompressed logs for dataset `{dataset_name}` is up-to-date. Skipping download.")
        return dataset_logs

    try:
        # fmt: off
        curl_cmds = [
            "curl",
            "--fail",
            "--location",
            "--output", str(dataset_logs.tarball_path),
            "--show-error",
            tarball_url,
        ]
        # fmt: on
        subprocess.run(curl_cmds, check=True)
        shutil.unpack_archive(str(dataset_logs.tarball_path), str(dataset_logs.extraction_dir))
    except Exception as e:
        raise RuntimeError(f"Failed to download and extract dataset `{dataset_name}`: {e}")

    print(f"Downloaded and extracted uncompressed logs for dataset `{dataset_name}`.")
    request.config.cache.set(dataset_name, True)
    return dataset_logs

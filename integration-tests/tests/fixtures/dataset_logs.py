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
        request,
        package_config,
        "hive-24hr",
        "https://zenodo.org/records/7094921/files/hive-24hr.tar.gz?download=1",
    )


@pytest.fixture(scope="session")
def elasticsearch(request, package_config: PackageConfig) -> DatasetLogs:
    return _download_and_extract_dataset(
        request,
        package_config,
        "elasticsearch",
        "https://zenodo.org/records/10516227/files/elasticsearch.tar.gz?download=1",
    )


@pytest.fixture(scope="session")
def spark_event_logs(request, package_config: PackageConfig) -> DatasetLogs:
    return _download_and_extract_dataset(
        request,
        package_config,
        "spark-event-logs",
        "https://zenodo.org/records/10516346/files/spark-event-logs.tar.gz?download=1",
    )


@pytest.fixture(scope="session")
def postgresql(request, package_config: PackageConfig) -> DatasetLogs:
    return _download_and_extract_dataset(
        request,
        package_config,
        "postgresql",
        "https://zenodo.org/records/10516402/files/postgresql.tar.gz?download=1",
    )


def _download_and_extract_dataset(
    request, package_config: PackageConfig, dataset_name: str, dataset_tarball_url: str
) -> DatasetLogs:
    dataset_logs = DatasetLogs(
        name=dataset_name,
        tar_url=dataset_tarball_url,
    )
    if request.config.cache.get(dataset_name, False):
        print(f"Uncompressed logs for dataset `{dataset_name}` is up-to-date. Skipping download.")
        return dataset_logs

    try:
        download_path_str = str(package_config.uncompressed_logs_dir / f"{dataset_name}.tar.gz")
        # fmt: off
        curl_cmds = [
            "curl",
            "--fail",
            "--location",
            "--output", download_path_str,
            "--show-error",
            dataset_tarball_url,
        ]
        # fmt: on
        subprocess.run(curl_cmds, check=True)

        extract_path_str = str(package_config.uncompressed_logs_dir / dataset_name)
        shutil.unpack_archive(download_path_str, extract_path_str)
    except Exception as e:
        raise RuntimeError(f"Failed to download and extract dataset `{dataset_name}`: {e}")

    print(f"Downloaded and extracted uncompressed logs for dataset `{dataset_name}`.")
    request.config.cache.set(dataset_name, True)
    return dataset_logs

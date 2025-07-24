import os
import shutil
import subprocess

import pytest
from tests.utils.config import (
    PackageConfig,
    DatasetLogs,
)


@pytest.fixture(scope="session")
def hive_24hr() -> DatasetLogs:
    return DatasetLogs(
        name="hive-24hr",
        tar_url="https://zenodo.org/records/7094921/files/hive-24hr.tar.gz?download=1",
    )


@pytest.fixture(scope="session")
def elasticsearch() -> DatasetLogs:
    return DatasetLogs(
        name="elasticsearch",
        tar_url="https://zenodo.org/records/10516227/files/elasticsearch.tar.gz?download=1",
    )


@pytest.fixture(scope="session")
def spark_event_logs() -> DatasetLogs:
    return DatasetLogs(
        name="spark-event-logs",
        tar_url="https://zenodo.org/records/10516346/files/spark-event-logs.tar.gz?download=1",
    )


@pytest.fixture(scope="session")
def postgresql() -> DatasetLogs:
    return DatasetLogs(
        name="postgresql",
        tar_url="https://zenodo.org/records/10516402/files/postgresql.tar.gz?download=1",
    )


@pytest.fixture
def download_and_extract_dataset(request, package_config: PackageConfig) -> DatasetLogs:
    dataset_config = request.getfixturevalue(request.param)
    dataset_name = dataset_config.name
    if request.config.cache.get(dataset_name, False):
        print(f"Uncompressed logs for dataset `{dataset_name}` is up-to-date. Skipping download.")
        return dataset_config

    try:
        download_path_str = str(package_config.uncompressed_logs_dir / f"{dataset_name}.tar.gz")
        # fmt: off
        curl_cmds = [
            "curl",
            "--fail",
            "--location",
            "--output", download_path_str,
            "--show-error",
            dataset_config.tar_url,
        ]
        # fmt: on
        subprocess.run(curl_cmds, check=True)

        extract_path_str = str(package_config.uncompressed_logs_dir / dataset_name)
        shutil.unpack_archive(download_path_str, extract_path_str)
        os.remove(download_path_str)
    except Exception as e:
        raise RuntimeError(f"Failed to download and extract dataset `{dataset_name}`: {e}")

    print(f"Downloaded and extracted uncompressed logs for dataset `{dataset_name}`.")
    request.config.cache.set(dataset_name, True)
    return dataset_config

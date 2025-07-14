import shutil
import pytest

from tests.utils.config import (
    BaseConfig,
    DatasetLogs,
)
from tests.utils.utils import run_and_assert

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


@pytest.fixture(autouse=True)
def download_and_extract_dataset(request, base_config: BaseConfig) -> DatasetLogs:
    dataset_config = request.getfixturevalue(request.param)
    dataset_name = dataset_config.name
    if request.config.cache.get(dataset_name, False):
        print(f"Uncompressed logs for dataset `{dataset_name}` is up-to-date.")
        return dataset_config

    download_path = str(base_config.uncompressed_logs_dir / f"{dataset_name}.tar.gz")
    extract_path = str(base_config.uncompressed_logs_dir / dataset_name)
    # fmt: off
    cmds = [
        "curl",
        "--fail",
        "--location",
        "--output", str(download_path),
        "--show-error",
        dataset_config.tar_url,
    ]
    # fmt: on
    run_and_assert(cmds)

    try:
        shutil.unpack_archive(download_path, extract_path)
    except:
        assert False, f"Tar extraction failed for downloaded dataset `{dataset_name}`."

    request.config.cache.set(dataset_name, True)
    return dataset_config

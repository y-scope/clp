import pytest

from .fixture_types import DatasetParams


@pytest.fixture(scope="session")
def hive_24hr() -> DatasetParams:
    return DatasetParams(
        dataset_name="hive-24hr",
        tar_url="https://zenodo.org/records/7094921/files/hive-24hr.tar.gz?download=1",
    )


@pytest.fixture(scope="session")
def elasticsearch() -> DatasetParams:
    return DatasetParams(
        dataset_name="elasticsearch",
        tar_url="https://zenodo.org/records/10516227/files/elasticsearch.tar.gz?download=1",
    )


@pytest.fixture(scope="session")
def spark_event_logs() -> DatasetParams:
    return DatasetParams(
        dataset_name="spark-event-logs",
        tar_url="https://zenodo.org/records/10516346/files/spark-event-logs.tar.gz?download=1",
    )


@pytest.fixture(scope="session")
def postgresql() -> DatasetParams:
    return DatasetParams(
        dataset_name="postgresql",
        tar_url="https://zenodo.org/records/10516402/files/postgresql.tar.gz?download=1",
    )

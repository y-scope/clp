import pytest
from fixtures.fixture_types import DatasetParams

pytestmark = pytest.mark.binaries

text_datasets = pytest.mark.parametrize(
    "download_and_extract_dataset",
    [
        # "hive_24hr",
    ],
    indirect=True,
)

json_datasets = pytest.mark.parametrize(
    "download_and_extract_dataset",
    [
        # "spark_event_logs",
        "postgresql",
    ],
    indirect=["download_and_extract_dataset"],
)


@pytest.mark.clp
@text_datasets
def test_clp_identity_transform(download_and_extract_dataset, request) -> None:
    # print(request.getfixturevalue(dataset))
    assert True


@pytest.mark.clp_s
@json_datasets
def test_clp_s_identity_transform(download_and_extract_dataset: DatasetParams, request) -> None:
    # print(request.getfixturevalue(download_and_extract_dataset))
    print(download_and_extract_dataset)
    assert True

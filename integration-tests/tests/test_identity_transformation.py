import pytest
from fixtures.fixture_types import DatasetParams, PackageTestConfig
from fixtures.utils import run_and_assert

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
def test_clp_s_identity_transform(
    request, config: PackageTestConfig, download_and_extract_dataset: DatasetParams
) -> None:
    dataset_name = download_and_extract_dataset.dataset_name
    download_dir = config.uncompressed_logs_dir / dataset_name
    archives_dir = config.test_output_dir / f"{dataset_name}-archives"
    extract_dir = config.test_output_dir / f"{dataset_name}-logs"

    compression_cmd = [config.clp_bin_dir / "clp-s", "c", archives_dir, download_dir]
    print(compression_cmd)
    run_and_assert(compression_cmd)

    decompression_cmd = [config.clp_bin_dir / "clp-s", "x", archives_dir, extract_dir]
    run_and_assert(decompression_cmd)

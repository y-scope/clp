from pathlib import Path

import pytest
from tests.utils.config import (
    DatasetLogs,
    PackageConfig,
)
from tests.utils.utils import (
    is_dir_tree_content_equal,
    is_json_file_structurally_equal,
    run_and_assert,
)

pytestmark = pytest.mark.binaries

text_datasets = pytest.mark.parametrize(
    "dataset_logs_fixture",
    [
        # "hive_24hr",
    ],
)

json_datasets = pytest.mark.parametrize(
    "dataset_logs_fixture",
    [
        "spark_event_logs",
        "postgresql",
    ],
)


@pytest.mark.clp
@text_datasets
def test_clp_identity_transform(
    request,
    package_config: PackageConfig,
    dataset_logs_fixture: str,
) -> None:
    dataset_logs: DatasetLogs = request.getfixturevalue(dataset_logs_fixture)
    dataset_name = dataset_logs.name
    dataset_logs.clear_test_outputs()

    binary_path_str = str(package_config.clp_bin_dir / "clp")
    # fmt: off
    compression_cmd = [
        binary_path_str,
        "c",
        "--progress",
        "--remove-path-prefix", str(dataset_logs.extraction_dir),
        str(dataset_logs.compression_dir),
        str(dataset_logs.extraction_dir),
    ]
    # fmt: on
    run_and_assert(compression_cmd)

    extraction_cmd = [
        binary_path_str,
        "x",
        str(dataset_logs.compression_dir),
        str(dataset_logs.decompression_dir),
    ]
    run_and_assert(extraction_cmd)

    assert is_dir_tree_content_equal(
        dataset_logs.extraction_dir, dataset_logs.decompression_dir
    ), "Mismatch between clp compression input and decompression output."

    dataset_logs.clear_test_outputs()


@pytest.mark.clp_s
@json_datasets
def test_clp_s_identity_transform(
    request,
    package_config: PackageConfig,
    dataset_logs_fixture: str,
) -> None:
    dataset_logs: DaatsetLogs = request.getfixturevalue(dataset_logs_fixture)
    dataset_name = dataset_logs.dataset_name
    _clp_s_compress_and_decompress(package_config, dataset_logs)

    # Recompress the decompressed output that's consolidated into a single json file, and decompress
    # it again to verify consistency. The compression input of the second iteration points to the
    # decompression output of the first.
    # TODO: Remove this check once we can directly compare decompressed logs (which would preserve
    #       the directory structure and row/key order) with the original downloaded logs.
    # See also: https://docs.yscope.com/clp/main/user-guide/core-clp-s.html#current-limitations
    consolidated_json_logs = DatasetLogs.create(
        package_config=package_config,
        dataset_name=f"{dataset_name}-consolidated-json",
        extraction_dir=dataset_logs.decompression_dir,
    )
    _clp_s_compress_and_decompress(package_config, consolidated_json_logs)

    assert is_json_file_structurally_equal(
        dataset_logs.decompression_dir / "original",
        consolidated_json_logs.decompression_dir / "original",
    ), "Mismatch between clp-s compression input and decompression output."

    # Remove test outputs to save disk space
    dataset_logs.clear_test_outputs()
    consolidated_json_logs.clear_test_outputs()


def _clp_s_compress_and_decompress(
    package_config: PackageConfig, dataset_logs: DatasetLogs
) -> None:
    dataset_logs.clear_test_outputs()
    bin_path = str(package_config.clp_bin_dir / "clp-s")
    ext_path = str(dataset_logs.extraction_dir)
    comp_path = str(dataset_logs.compression_dir)
    decomp_path = str(dataset_logs.decompression_dir)
    run_and_assert([bin_path, "c", comp_path, ext_path])
    run_and_assert([bin_path, "x", comp_path, decomp_path])

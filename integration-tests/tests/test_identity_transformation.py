import shutil
import tempfile

import pytest
from tests.utils.config import (
    BaseConfig,
    DatasetLogs,
)
from tests.utils.utils import (
    diff_equal,
    run_and_assert,
)

pytestmark = pytest.mark.binaries

text_datasets = pytest.mark.parametrize(
    "download_and_extract_dataset",
    [
        "hive_24hr",
    ],
    indirect=["download_and_extract_dataset"],
)

json_datasets = pytest.mark.parametrize(
    "download_and_extract_dataset",
    [
        "spark_event_logs",
        # "postgresql",
    ],
    indirect=["download_and_extract_dataset"],
)


@pytest.mark.clp
@text_datasets
def test_clp_identity_transform(
    request, base_config: BaseConfig, download_and_extract_dataset: DatasetLogs
) -> None:
    binary_path_str = base_config.clp_bin_dir / "clp"
    dataset_name = download_and_extract_dataset.name
    download_dir = base_config.uncompressed_logs_dir / dataset_name
    archives_dir = base_config.test_output_dir / f"{dataset_name}-archives"
    shutil.rmtree(archives_dir, ignore_errors=True)
    extract_dir = base_config.test_output_dir / f"{dataset_name}-logs"
    shutil.rmtree(extract_dir, ignore_errors=True)

    # fmt: off
    compression_cmd = [
        binary_path_str,
        "c",
        "--progress",
        "--remove-path-prefix", str(download_dir),
        str(archives_dir),
        str(download_dir),
    ]
    # fmt: on
    run_and_assert(compression_cmd)
    run_and_assert([binary_path_str, "x", str(archives_dir), str(extract_dir)])

    diff_equal(download_dir, extract_dir)


@pytest.mark.clp_s
@json_datasets
def test_clp_s_identity_transform(
    request, base_config: BaseConfig, download_and_extract_dataset: DatasetLogs
) -> None:
    binary_path_str = base_config.clp_bin_dir / "clp-s"
    dataset_name = download_and_extract_dataset.name
    download_dir = base_config.uncompressed_logs_dir / dataset_name
    archives_dir = base_config.test_output_dir / f"{dataset_name}-archives"
    extract_dir = base_config.test_output_dir / f"{dataset_name}-logs"

    # shutil.rmtree(archives_dir, ignore_errors=True)
    # shutil.rmtree(extract_dir, ignore_errors=True)

    # run_and_assert([binary_path_str, "c", str(archives_dir), str(download_dir)])
    # run_and_assert([binary_path_str, "x", str(archives_dir), str(extract_dir)])

    # Recompress the decompressed single-file output and decompress it again to verify consistency.
    # TODO: Remove this check once we can directly compare decompressed logs (which would preserve
    #       the directory structure) with the original downloaded logs.
    # See also: https://docs.yscope.com/clp/main/user-guide/core-clp-s.html#current-limitations
    single_file_archives_dir = base_config.test_output_dir / f"{dataset_name}-single-file-archives"
    single_file_extract_dir = base_config.test_output_dir / f"{dataset_name}-single-file-logs"

    # shutil.rmtree(single_file_archives_dir, ignore_errors=True)
    # shutil.rmtree(single_file_extract_dir, ignore_errors=True)

    # run_and_assert([binary_path_str, "c", single_file_archives_dir, extract_dir])
    # run_and_assert([binary_path_str, "x", single_file_archives_dir, single_file_extract_dir])

    # Key order within a single entry is not preserved.
    with tempfile.NamedTemporaryFile(
        suffix=".json", mode="w+", delete=True
    ) as t1, tempfile.NamedTemporaryFile(suffix=".json", mode="w+", delete=True) as t2:
        run_and_assert(["jq", "--sort-keys", ".", str(extract_dir / "original")], stdout=t1)
        run_and_assert(
            ["jq", "--sort-keys", ".", str(single_file_extract_dir / "original")], stdout=t2
        )
        t1.flush()
        t2.flush()
        diff_equal(t1.name, t2.name)

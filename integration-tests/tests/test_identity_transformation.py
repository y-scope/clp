import shutil
import subprocess
from contextlib import contextmanager
from pathlib import Path
from tempfile import NamedTemporaryFile
from typing import IO

import pytest
from tests.utils.config import (
    DatasetLogs,
    PackageConfig,
)
from tests.utils.utils import (
    is_dir_tree_content_equal,
    run_and_assert,
)

pytestmark = pytest.mark.binaries

text_datasets = pytest.mark.parametrize(
    "download_and_extract_dataset",
    [
        # "hive_24hr",
    ],
    indirect=["download_and_extract_dataset"],
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
def test_clp_identity_transform(
    request, package_config: PackageConfig, download_and_extract_dataset: DatasetLogs
) -> None:
    binary_path_str = str(package_config.clp_bin_dir / "clp")
    dataset_name = download_and_extract_dataset.name
    download_dir = package_config.uncompressed_logs_dir / dataset_name
    archives_dir = package_config.test_output_dir / f"{dataset_name}-archives"
    extract_dir = package_config.test_output_dir / f"{dataset_name}-logs"

    shutil.rmtree(archives_dir, ignore_errors=True)
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

    extraction_cmd = [binary_path_str, "x", str(archives_dir), str(extract_dir)]
    run_and_assert(extraction_cmd)

    assert is_dir_tree_content_equal(
        download_dir, extract_dir
    ), "Mismatch between clp compression input and decompression output."

    shutil.rmtree(archives_dir, ignore_errors=True)
    shutil.rmtree(extract_dir, ignore_errors=True)


@pytest.mark.clp_s
@json_datasets
def test_clp_s_identity_transform(
    request, package_config: PackageConfig, download_and_extract_dataset: DatasetLogs
) -> None:
    binary_path_str = str(package_config.clp_bin_dir / "clp-s")
    dataset_name = download_and_extract_dataset.name
    download_dir = package_config.uncompressed_logs_dir / dataset_name
    archives_dir = package_config.test_output_dir / f"{dataset_name}-archives"
    extract_dir = package_config.test_output_dir / f"{dataset_name}-logs"

    shutil.rmtree(archives_dir, ignore_errors=True)
    shutil.rmtree(extract_dir, ignore_errors=True)

    run_and_assert([binary_path_str, "c", str(archives_dir), str(download_dir)])
    run_and_assert([binary_path_str, "x", str(archives_dir), str(extract_dir)])

    # Recompress the decompressed single-file output and decompress it again to verify consistency.
    # TODO: Remove this check once we can directly compare decompressed logs (which would preserve
    #       the directory structure and row/key order) with the original downloaded logs.
    # See also: https://docs.yscope.com/clp/main/user-guide/core-clp-s.html#current-limitations
    single_file_archives_dir = (
        package_config.test_output_dir / f"{dataset_name}-single-file-archives"
    )
    single_file_extract_dir = package_config.test_output_dir / f"{dataset_name}-single-file-logs"

    shutil.rmtree(single_file_archives_dir, ignore_errors=True)
    shutil.rmtree(single_file_extract_dir, ignore_errors=True)

    run_and_assert([binary_path_str, "c", single_file_archives_dir, extract_dir])
    run_and_assert([binary_path_str, "x", single_file_archives_dir, single_file_extract_dir])

    # Key and row orders are not preserved during `clp-s` operations, so sort before diffing.
    with _sort_json_keys_and_rows(extract_dir / "original") as s1, _sort_json_keys_and_rows(
        single_file_extract_dir / "original"
    ) as s2:
        assert is_dir_tree_content_equal(
            s1.name, s2.name
        ), "Mismatch between clp-s compression input and decompression output."

    shutil.rmtree(archives_dir, ignore_errors=True)
    shutil.rmtree(extract_dir, ignore_errors=True)
    shutil.rmtree(single_file_archives_dir, ignore_errors=True)
    shutil.rmtree(single_file_extract_dir, ignore_errors=True)


@contextmanager
def _sort_json_keys_and_rows(json_fp: Path) -> IO[str]:
    with NamedTemporaryFile(mode="w+", delete=True) as keys_sorted, NamedTemporaryFile(
        mode="w+", delete=True
    ) as flattened, NamedTemporaryFile(mode="w+", delete=True) as keys_and_rows_sorted:
        keys_and_rows_sorted = NamedTemporaryFile(mode="w+", delete=True)
        subprocess.run(["jq", "--sort-keys", ".", str(json_fp)], check=True, stdout=keys_sorted)
        keys_sorted.flush()
        subprocess.run(["jq", ".", keys_sorted.name], check=True, stdout=flattened)
        flattened.flush()
        subprocess.run(["sort", flattened.name], check=True, stdout=keys_and_rows_sorted)
        keys_and_rows_sorted.flush()
        yield keys_and_rows_sorted

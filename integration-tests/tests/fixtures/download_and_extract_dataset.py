import shutil
from pathlib import Path

import pytest

from .fixture_types import DatasetParams
from .package_test_config import PackageTestConfig
from .utils import run_and_assert


@pytest.fixture(autouse=True)
def download_and_extract_dataset(request, config: PackageTestConfig) -> DatasetParams:
    download_params = request.getfixturevalue(request.param)
    dataset_name = download_params.dataset_name
    if request.config.cache.get(dataset_name, False):
        print(f"Uncompressed logs for dataset `{dataset_name}` is up-to-date.")
        return download_params

    tar_path = str(config.uncompressed_logs_dir / f"{dataset_name}.tar.gz")
    extract_path = str(config.uncompressed_logs_dir / dataset_name)
    cmds = [
        "curl",
        "--fail",
        "--location",
        "--output",
        str(tar_path),
        "--show-error",
        download_params.tar_url,
    ]
    run_and_assert(cmds)

    try:
        shutil.unpack_archive(tar_path, extract_path)
        request.config.cache.set(dataset_name, True)
    except:
        assert False, f"Tar extraction failed for dataset `{dataset_name}`."

    return download_params

"""Session-scoped fixtures for test datasets downloaded on-demand from external URLs."""

import logging
from dataclasses import dataclass, field, InitVar
from pathlib import Path

import pytest

from tests.utils.classes import ExternalAction, IntegrationTestPathConfig
from tests.utils.utils import (
    get_binary_path,
    remove_path,
    validate_dir_exists,
)

logger = logging.getLogger(__name__)


@dataclass(frozen=True)
class DownloadedDataset:
    """Metadata for a dataset downloaded on-demand from an external URL."""

    #:
    dataset_name: str
    #:
    tarball_url: str
    integration_test_path_config: InitVar[IntegrationTestPathConfig]
    #:
    tarball_path: Path = field(init=False, repr=True)
    #:
    extraction_dir: Path = field(init=False, repr=True)
    #: Optional number of log events in the downloaded logs.
    num_log_events: int | None = None

    def __post_init__(self, integration_test_path_config: IntegrationTestPathConfig) -> None:
        """Initialize and set tarball and extraction paths for integration test logs."""
        dataset_name = self.dataset_name.strip()
        if 0 == len(dataset_name):
            err_msg = "`dataset_name` cannot be empty."
            raise ValueError(err_msg)
        downloaded_logs_dir = integration_test_path_config.downloaded_logs_dir
        validate_dir_exists(downloaded_logs_dir)

        object.__setattr__(self, "dataset_name", dataset_name)
        object.__setattr__(self, "tarball_path", downloaded_logs_dir / f"{dataset_name}.tar.gz")
        object.__setattr__(self, "extraction_dir", downloaded_logs_dir / dataset_name)


@pytest.fixture(scope="session")
def hive_24hr(
    request: pytest.FixtureRequest,
    integration_test_path_config: IntegrationTestPathConfig,
) -> DownloadedDataset:
    """Provides shared `hive_24hr` test logs."""
    return _download_and_extract_gzip_dataset(
        request=request,
        integration_test_path_config=integration_test_path_config,
        dataset_name="hive-24hr",
        tarball_url="https://zenodo.org/records/7094921/files/hive-24hr.tar.gz?download=1",
    )


@pytest.fixture(scope="session")
def postgresql(
    request: pytest.FixtureRequest,
    integration_test_path_config: IntegrationTestPathConfig,
) -> DownloadedDataset:
    """Provides shared `postgresql` test logs."""
    return _download_and_extract_gzip_dataset(
        request=request,
        integration_test_path_config=integration_test_path_config,
        dataset_name="postgresql",
        tarball_url="https://zenodo.org/records/10516402/files/postgresql.tar.gz?download=1",
    )


def _download_and_extract_gzip_dataset(
    request: pytest.FixtureRequest,
    integration_test_path_config: IntegrationTestPathConfig,
    dataset_name: str,
    tarball_url: str,
    keep_leading_dir: bool = False,
) -> DownloadedDataset:
    """
    Download and extract a gzip-compressed dataset tarball for setting up the `DownloadedDataset`
    fixture. Adjust its file permissions for test use.

    :param request: Provides access to the pytest cache.
    :param integration_test_path_config: See `IntegrationTestPathConfig`.
    :param dataset_name: Dataset name.
    :param tarball_url: Dataset tarball URL.
    :param keep_leading_dir: Whether to preserve the top-level directory during tarball extraction.
        Defaults to False to avoid an unnecessary extra directory level.
    :return: A DownloadedDataset instance providing metadata for the downloaded logs.
    :raise pytest.fail: If `curl`, `tar`, or `chmod` returns a non-zero exit code.
    """
    downloaded_dataset = DownloadedDataset(
        dataset_name=dataset_name,
        tarball_url=tarball_url,
        integration_test_path_config=integration_test_path_config,
    )
    if request.config.cache.get(dataset_name, False):
        logger.info("Test logs `%s` are up-to-date. Skipping download.", dataset_name)
        return downloaded_dataset

    remove_path(downloaded_dataset.tarball_path)
    remove_path(downloaded_dataset.extraction_dir)
    downloaded_dataset.extraction_dir.mkdir(parents=True, exist_ok=False)

    tarball_path_str = str(downloaded_dataset.tarball_path)
    extract_path_str = str(downloaded_dataset.extraction_dir)

    # fmt: off
    curl_cmd = [
        get_binary_path("curl"),
        "--fail",
        "--location",
        "--output", tarball_path_str,
        "--show-error",
        tarball_url,
    ]
    # fmt: on
    curl_action = ExternalAction.from_cmd(curl_cmd)
    curl_action.assert_returncode(f"`curl` failed when downloading `{tarball_url}`.")

    # fmt: off
    extract_cmd = [
        get_binary_path("tar"),
        "--extract",
        "--gzip",
        "--file", tarball_path_str,
        "--directory", extract_path_str,
    ]
    # fmt: on
    if not keep_leading_dir:
        extract_cmd.extend(["--strip-components", "1"])
    extract_action = ExternalAction.from_cmd(extract_cmd)
    extract_action.assert_returncode(f"`tar` failed when extracting `{tarball_path_str}`.")

    # Allow the downloaded and extracted contents to be deletable or overwritable by adding write
    # permissions for both the user and the group.
    chmod_bin = get_binary_path("chmod")
    chmod_tarball_action = ExternalAction.from_cmd([chmod_bin, "gu+w", tarball_path_str])
    chmod_tarball_action.assert_returncode(f"`chmod` failed for `{tarball_path_str}`.")
    chmod_extract_action = ExternalAction.from_cmd([chmod_bin, "-R", "gu+w", extract_path_str])
    chmod_extract_action.assert_returncode(f"`chmod` failed for `{extract_path_str}`.")

    logger.info("Downloaded and extracted uncompressed logs for dataset `%s`.", dataset_name)
    request.config.cache.set(dataset_name, True)
    return downloaded_dataset

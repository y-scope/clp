"""Session-scoped fixtures for test datasets downloaded on-demand from external URLs."""

import logging
import subprocess
from dataclasses import dataclass, field, InitVar
from pathlib import Path

import pytest

from tests.utils.classes import IntegrationTestPathConfig
from tests.utils.utils import (
    get_binary_path,
    remove_path,
    validate_dir_exists,
)

logger = logging.getLogger(__name__)


@dataclass(frozen=True)
class DownloadedDataset:
    """Metadata for the downloaded logs used for integration tests."""

    #:
    name: str
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
        name = self.name.strip()
        if 0 == len(name):
            err_msg = "`name` cannot be empty."
            raise ValueError(err_msg)
        downloaded_logs_dir = integration_test_path_config.downloaded_logs_dir
        validate_dir_exists(downloaded_logs_dir)

        object.__setattr__(self, "name", name)
        object.__setattr__(self, "tarball_path", downloaded_logs_dir / f"{name}.tar.gz")
        object.__setattr__(self, "extraction_dir", downloaded_logs_dir / name)


@pytest.fixture(scope="session")
def hive_24hr(
    request: pytest.FixtureRequest,
    integration_test_path_config: IntegrationTestPathConfig,
) -> DownloadedDataset:
    """Provides shared `hive_24hr` test logs."""
    return _download_and_extract_gzip_dataset(
        request=request,
        integration_test_path_config=integration_test_path_config,
        name="hive-24hr",
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
        name="postgresql",
        tarball_url="https://zenodo.org/records/10516402/files/postgresql.tar.gz?download=1",
    )


def _download_and_extract_gzip_dataset(
    request: pytest.FixtureRequest,
    integration_test_path_config: IntegrationTestPathConfig,
    name: str,
    tarball_url: str,
    keep_leading_dir: bool = False,
) -> DownloadedDataset:
    """
    Download and extract a gzip-compressed dataset tarball for setting up the `DownloadedDataset`
    fixture. Adjust its file permissions for test use.

    :param request: Provides access to the pytest cache.
    :param integration_test_path_config: See `IntegrationTestPathConfig`.
    :param name: Dataset name.
    :param tarball_url: Dataset tarball URL.
    :param keep_leading_dir: Whether to preserve the top-level directory during tarball extraction.
        Defaults to False to avoid an unnecessary extra directory level.
    :return: A DownloadedDataset instance providing metadata for the downloaded logs.
    :raises subprocess.CalledProcessError: If `curl`, `tar`, or `chmod` fails.
    """
    downloaded_dataset = DownloadedDataset(
        name=name,
        tarball_url=tarball_url,
        integration_test_path_config=integration_test_path_config,
    )
    if request.config.cache.get(name, False):
        logger.info("Test logs `%s` are up-to-date. Skipping download.", name)
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
    subprocess.run(curl_cmd, check=True)

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
    subprocess.run(extract_cmd, check=True)

    # Allow the downloaded and extracted contents to be deletable or overwritable by adding write
    # permissions for both the user and the group.
    chmod_bin = get_binary_path("chmod")
    subprocess.run([chmod_bin, "gu+w", tarball_path_str], check=True)
    subprocess.run([chmod_bin, "-R", "gu+w", extract_path_str], check=True)

    logger.info("Downloaded and extracted uncompressed logs for dataset `%s`.", name)
    request.config.cache.set(name, True)
    return downloaded_dataset

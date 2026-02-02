"""Session-scoped test log fixtures shared across integration tests."""

import logging
import shutil
import subprocess

import pytest

from tests.utils.config import (
    IntegrationTestLogs,
    IntegrationTestPathConfig,
)
from tests.utils.utils import remove_path

logger = logging.getLogger(__name__)


@pytest.fixture(scope="session")
def hive_24hr(
    request: pytest.FixtureRequest,
    integration_test_path_config: IntegrationTestPathConfig,
) -> IntegrationTestLogs:
    """Provides shared `hive_24hr` test logs."""
    return _download_and_extract_dataset(
        request=request,
        integration_test_path_config=integration_test_path_config,
        name="hive-24hr",
        tarball_url="https://zenodo.org/records/7094921/files/hive-24hr.tar.gz?download=1",
    )


@pytest.fixture(scope="session")
def postgresql(
    request: pytest.FixtureRequest,
    integration_test_path_config: IntegrationTestPathConfig,
) -> IntegrationTestLogs:
    """Provides shared `postgresql` test logs."""
    return _download_and_extract_dataset(
        request=request,
        integration_test_path_config=integration_test_path_config,
        name="postgresql",
        tarball_url="https://zenodo.org/records/10516402/files/postgresql.tar.gz?download=1",
    )


def _download_and_extract_dataset(
    request: pytest.FixtureRequest,
    integration_test_path_config: IntegrationTestPathConfig,
    name: str,
    tarball_url: str,
    strip_leading_dir: bool = True,
) -> IntegrationTestLogs:
    """
    :param request: Provides access to the pytest cache.
    :param integration_test_path_config: See `IntegrationTestPathConfig`.
    :param name: Dataset name.
    :param tarball_url: Dataset tarball URL.
    :param strip_leading_dir: Whether to strip a single top-level directory from the tarball
        contents. Defaults to True.
    :return: An IntegrationTestLogs instance with the dataset downloaded, extracted, and file
        permissions adjusted for test use.
    """
    integration_test_logs = IntegrationTestLogs(
        name=name,
        tarball_url=tarball_url,
        integration_test_path_config=integration_test_path_config,
    )
    if request.config.cache.get(name, False):
        logger.info("Test logs `%s` are up-to-date. Skipping download.", name)
        return integration_test_logs

    remove_path(integration_test_logs.tarball_path)
    remove_path(integration_test_logs.extraction_dir)
    integration_test_logs.extraction_dir.mkdir(parents=True, exist_ok=False)

    tarball_path_str = str(integration_test_logs.tarball_path)
    extract_path_str = str(integration_test_logs.extraction_dir)

    curl_bin = shutil.which("curl")
    if curl_bin is None:
        err_msg = "curl executable not found"
        raise RuntimeError(err_msg)

    # fmt: off
    curl_cmd = [
        curl_bin,
        "--fail",
        "--location",
        "--output", tarball_path_str,
        "--show-error",
        tarball_url,
    ]
    # fmt: on
    subprocess.run(curl_cmd, check=True)

    tar_bin = shutil.which("tar")
    if tar_bin is None:
        err_msg = "tar executable not found"
        raise RuntimeError(err_msg)

    # fmt: off
    extract_cmd = [
        tar_bin,
        "--extract",
        "--gzip",
        "--file", tarball_path_str,
        "-C", extract_path_str,
    ]
    # fmt: on
    if has_leading_directory_component:
        extract_cmd.extend(["--strip-components", "1"])
    subprocess.run(extract_cmd, check=True)

    chmod_bin = shutil.which("chmod")
    if chmod_bin is None:
        err_msg = "chmod executable not found"
        raise RuntimeError(err_msg)

    # Allow the downloaded and extracted contents to be deletable or overwritable
    subprocess.run([chmod_bin, "-R", "gu+w", tarball_path_str], check=True)
    subprocess.run([chmod_bin, "-R", "gu+w", extract_path_str], check=True)

    logger.info("Downloaded and extracted uncompressed logs for dataset `%s`.", name)
    request.config.cache.set(name, True)
    return integration_test_logs

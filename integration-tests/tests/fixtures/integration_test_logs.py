"""Session-scoped test log fixtures shared across integration tests."""

import logging
import shutil
import subprocess

import pytest

from tests.utils.config import (
    IntegrationTestLogs,
    IntegrationTestPathConfig,
)
from tests.utils.utils import unlink

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
) -> IntegrationTestLogs:
    integration_test_logs = IntegrationTestLogs(
        name=name,
        tarball_url=tarball_url,
        integration_test_path_config=integration_test_path_config,
    )
    if request.config.cache.get(name, False):
        logger.info("Test logs `%s` are up-to-date. Skipping download.", name)
        return integration_test_logs

    curl_bin = shutil.which("curl")
    if curl_bin is None:
        err_msg = "curl executable not found"
        raise RuntimeError(err_msg)

    try:
        # fmt: off
        curl_cmds = [
            curl_bin,
            "--fail",
            "--location",
            "--output", str(integration_test_logs.tarball_path),
            "--show-error",
            tarball_url,
        ]
        # fmt: on
        subprocess.run(curl_cmds, check=True)

        unlink(integration_test_logs.extraction_dir)
        shutil.unpack_archive(
            integration_test_logs.tarball_path, integration_test_logs.extraction_dir
        )
    except Exception as e:
        err_msg = f"Failed to download and extract dataset `{name}`."
        raise RuntimeError(err_msg) from e

    # Allow the extracted content to be deletable or overwritable
    chmod_bin = shutil.which("chmod")
    if chmod_bin is None:
        err_msg = "chmod executable not found"
        raise RuntimeError(err_msg)
    subprocess.run([chmod_bin, "-R", "gu+w", integration_test_logs.extraction_dir], check=True)

    logger.info("Downloaded and extracted uncompressed logs for dataset `%s`.", name)
    request.config.cache.set(name, True)
    return integration_test_logs

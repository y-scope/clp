"""Define test logs fixtures."""

import logging
import shutil
import subprocess

import pytest

from tests.utils.config import (
    IntegrationTestConfig,
    IntegrationTestLogs,
)
from tests.utils.utils import unlink

logger = logging.getLogger(__name__)


@pytest.fixture(scope="session")
def hive_24hr(
    request: pytest.FixtureRequest,
    integration_test_config: IntegrationTestConfig,
) -> IntegrationTestLogs:
    """Fixture that provides `hive_24hr` test logs shared across tests."""
    return _download_and_extract_dataset(
        request=request,
        integration_test_config=integration_test_config,
        name="hive-24hr",
        tarball_url="https://zenodo.org/records/7094921/files/hive-24hr.tar.gz?download=1",
    )


@pytest.fixture(scope="session")
def postgresql(
    request: pytest.FixtureRequest,
    integration_test_config: IntegrationTestConfig,
) -> IntegrationTestLogs:
    """Fixture that provides `postgresql` test logs shared across tests."""
    return _download_and_extract_dataset(
        request=request,
        integration_test_config=integration_test_config,
        name="postgresql",
        tarball_url="https://zenodo.org/records/10516402/files/postgresql.tar.gz?download=1",
    )


def _download_and_extract_dataset(
    request: pytest.FixtureRequest,
    integration_test_config: IntegrationTestConfig,
    name: str,
    tarball_url: str,
) -> IntegrationTestLogs:
    integration_test_logs = IntegrationTestLogs(
        name=name,
        tarball_url=tarball_url,
        integration_test_config=integration_test_config,
    )
    if request.config.cache.get(name, False):
        logger.info("Test logs `%s` are up-to-date. Skipping download.", name)
        return integration_test_logs

    try:
        # fmt: off
        curl_cmds = [
            "curl",
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
        subprocess.run(["chmod","-R", "gu+w", integration_test_logs.extraction_dir], check=True)
    except Exception as e:
        err_msg = f"Failed to download and extract dataset `{name}`."
        raise RuntimeError(err_msg) from e

    logger.info("Downloaded and extracted uncompressed logs for dataset `%s`.", name)
    request.config.cache.set(name, True)
    return integration_test_logs

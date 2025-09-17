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
            "--output", str(integration_test_logs.tar_gz_path),
            "--show-error",
            tarball_url,
        ]
        # fmt: on
        subprocess.run(curl_cmds, check=True)

        unlink(integration_test_logs.extraction_dir)
        shutil.unpack_archive(
            integration_test_logs.tar_gz_path, integration_test_logs.extraction_dir
        )
    except Exception as e:
        err_msg = f"Failed to download and extract dataset `{name}`."
        raise RuntimeError(err_msg) from e

    extraction_path = str(integration_test_logs.extraction_dir)

    # Allow the extracted content to be deletable or overwritable
    chmod_bin = shutil.which("chmod")
    if chmod_bin is None:
        err_msg = "chmod executable not found"
        raise RuntimeError(err_msg)
    subprocess.run([chmod_bin, "-R", "gu+w", extraction_path], check=True)

    # Create tar of the extracted content for different compression formats
    tar_bin = shutil.which("tar")
    if tar_bin is None:
        err_msg = "tar executable not found"
        raise RuntimeError(err_msg)
    subprocess.run([tar_bin, "--create", f"--file={integration_test_logs.base_tar_path}", f"--directory={integration_test_logs.extraction_dir}", extraction_path], check=True)

    # Create LibLZMA xz tar
    xz_bin = str(integration_test_config.deps_config.xz_binary_path)
    xz_cmds = [xz_bin, "--keep", "--compress", "--stdout", extraction_path]
    with open(integration_test_logs.tar_xz_path, "wb") as fout:
        subprocess.run(xz_cmds, check=True, stdout=fout, stdin=subprocess.DEVNULL)

    logger.info("Downloaded and extracted uncompressed logs for dataset `%s`.", name)
    request.config.cache.set(name, True)
    return integration_test_logs

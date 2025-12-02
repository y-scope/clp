"""Provides utility functions for interacting with the CLP package."""

import logging
import subprocess

import pytest

from tests.utils.asserting_utils import run_and_assert
from tests.utils.config import (
    IntegrationTestLogs,
    PackageCompressJob,
    PackageConfig,
    PackageInstance,
)

logger = logging.getLogger(__name__)


def start_clp_package(package_config: PackageConfig) -> None:
    """
    Starts an instance of the CLP package.

    :param package_config:
    :raise RuntimeError: If the package fails to start.
    """
    path_config = package_config.path_config
    start_script_path = path_config.start_script_path
    temp_config_file_path = package_config.temp_config_file_path
    try:
        # fmt: off
        start_cmd = [
            str(start_script_path),
            "--config", str(temp_config_file_path),
        ]
        # fmt: on
        subprocess.run(start_cmd, check=True)
    except Exception as err:
        err_msg = f"Failed to start an instance of the {package_config.mode_name} package."
        raise RuntimeError(err_msg) from err


def stop_clp_package(instance: PackageInstance) -> None:
    """
    Stops an instance of the CLP package.

    :param instance:
    :raise RuntimeError: If the package fails to stop.
    """
    package_config = instance.package_config
    path_config = package_config.path_config
    stop_script_path = path_config.stop_script_path
    try:
        # fmt: off
        stop_cmd = [
            str(stop_script_path)
        ]
        # fmt: on
        subprocess.run(stop_cmd, check=True)
    except Exception as err:
        err_msg = f"Failed to stop an instance of the {package_config.mode_name} package."
        raise RuntimeError(err_msg) from err


def compress_with_clp_package(
    request: pytest.FixtureRequest,
    compress_job: PackageCompressJob,
    package_instance: PackageInstance,
) -> None:
    """
    Construct and run a compression command for the CLP package.

    :param request:
    :param compress_job:
    :param package_instance:
    """
    package_config = package_instance.package_config
    compress_script_path = package_config.path_config.compress_script_path
    temp_config_file_path = package_config.temp_config_file_path
    # Get the correct logs fixture for this job and set up path config objects.
    integration_test_logs: IntegrationTestLogs = request.getfixturevalue(
        compress_job.log_fixture_name
    )

    # Construct the compression command for this job.
    compress_cmd = [
        str(compress_script_path),
        "--config",
        str(temp_config_file_path),
    ]
    if compress_job.dataset_name is not None:
        compress_cmd.extend(
            [
                "--dataset",
                compress_job.dataset_name,
            ]
        )
    if compress_job.timestamp_key is not None:
        compress_cmd.extend(
            [
                "--timestamp-key",
                compress_job.timestamp_key,
            ]
        )
    if compress_job.unstructured:
        compress_cmd.append("--unstructured")
    if compress_job.tags is not None:
        compress_cmd.extend(
            [
                "-t",
                ",".join(compress_job.tags),
            ]
        )
    if compress_job.subpath is not None:
        compress_cmd.append(str(integration_test_logs.extraction_dir / compress_job.subpath))
    else:
        compress_cmd.append(str(integration_test_logs.extraction_dir))

    # Run compression command for this job.
    run_and_assert(compress_cmd)

    # TODO: Assert that the compression job was successful with package decompression.

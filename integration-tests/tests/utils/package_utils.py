"""Provides utility functions related to the CLP package used across `integration-tests`."""

import pytest

from tests.utils.classes import ClpAction
from tests.utils.config import (
    PackageCompressionJob,
    PackageTestConfig,
)


def start_clp_package(package_test_config: PackageTestConfig) -> None:
    """
    Starts an instance of the CLP package.

    :param package_test_config:
    :raise pytest.fail: if the start script returns a non-zero exit code.
    """
    path_config = package_test_config.path_config
    start_script_path = path_config.start_script_path
    temp_config_file_path = package_test_config.temp_config_file_path

    # fmt: off
    start_cmd = [
        str(start_script_path),
        "--config", str(temp_config_file_path),
    ]
    # fmt: on
    start_action = ClpAction.from_cmd(start_cmd)
    result = start_action.verify_returncode()
    if not result:
        pytest.fail(result.failure_message)


def stop_clp_package(package_test_config: PackageTestConfig) -> None:
    """
    Stops the running instance of the CLP package.

    :param package_test_config:
    :raise pytest.fail: if the stop script returns a non-zero exit code.
    """
    path_config = package_test_config.path_config
    stop_script_path = path_config.stop_script_path
    temp_config_file_path = package_test_config.temp_config_file_path

    # fmt: off
    stop_cmd = [
        str(stop_script_path),
        "--config", str(temp_config_file_path),
    ]
    # fmt: on
    stop_action = ClpAction.from_cmd(stop_cmd)
    result = stop_action.verify_returncode()
    if not result:
        pytest.fail(result.failure_message)


def run_package_compression_script(
    compression_job: PackageCompressionJob,
    package_test_config: PackageTestConfig,
) -> None:
    """
    Constructs and runs a compression command on the CLP package.

    :param compression_job:
    :param package_test_config:
    :raise pytest.fail: if the compression script returns a non-zero exit code.
    """
    path_config = package_test_config.path_config
    compress_script_path = path_config.compress_script_path
    temp_config_file_path = package_test_config.temp_config_file_path

    compress_cmd = [
        str(compress_script_path),
        "--config",
        str(temp_config_file_path),
    ]

    if compression_job.options is not None:
        compress_cmd.extend(compression_job.options)

    if compression_job.positional_args is not None:
        compress_cmd.extend(compression_job.positional_args)

    compress_cmd.append(str(compression_job.path_to_original_dataset))

    # Run compression command for this job and fail the test if it returned a bad return code.
    compress_action = ClpAction.from_cmd(compress_cmd)
    result = compress_action.verify_returncode()
    if not result:
        pytest.fail(result.failure_message)

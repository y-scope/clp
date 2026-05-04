"""Provides utility functions related to the CLP package used across `integration-tests`."""

import pytest

from tests.utils.classes import ExternalAction
from tests.utils.config import (
    PackageCompressionJob,
    PackageTestConfig,
)
from tests.utils.logging_utils import format_action_failure_msg


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
    start_action = ExternalAction(cmd=start_cmd)
    if start_action.completed_proc.returncode != 0:
        pytest.fail(
            format_action_failure_msg(
                f"Failed to start CLP package using `{start_script_path.name}`.",
                start_action,
            )
        )


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
    stop_action = ExternalAction(cmd=stop_cmd)
    if stop_action.completed_proc.returncode != 0:
        pytest.fail(
            format_action_failure_msg(
                f"Failed to stop CLP package using `{stop_script_path.name}`.",
                stop_action,
            )
        )


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

    # Run compression command for this job and assert that it succeeds.
    compress_action = ExternalAction(cmd=compress_cmd)
    if compress_action.completed_proc.returncode != 0:
        pytest.fail(
            format_action_failure_msg(
                f"Compression script `{compress_script_path.name}` failed.",
                compress_action,
            )
        )

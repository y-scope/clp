"""Provides utility functions related to the CLP package used across `integration-tests`."""

from tests.utils.config import (
    PackageCompressionJob,
    PackageTestConfig,
)
from tests.utils.subprocess_utils import run_and_log_subprocess


def start_clp_package(package_test_config: PackageTestConfig) -> None:
    """
    Starts an instance of the CLP package.

    :param package_test_config:
    :raise: Propagates `run_and_log_subprocess`'s errors.
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
    run_and_log_subprocess(start_cmd)


def stop_clp_package(package_test_config: PackageTestConfig) -> None:
    """
    Stops the running instance of the CLP package.

    :param package_test_config:
    :raise: Propagates `run_and_log_subprocess`'s errors.
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
    run_and_log_subprocess(stop_cmd)


def run_package_compression_script(
    compression_job: PackageCompressionJob,
    package_test_config: PackageTestConfig,
) -> None:
    """
    Constructs and runs a compression command on the CLP package.

    :param compression_job:
    :param package_test_config:
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
    run_and_log_subprocess(compress_cmd)

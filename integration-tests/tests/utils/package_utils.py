"""Provides utility functions related to the CLP package used across `integration-tests`."""

from tests.package_tests.classes import ClpPackage
from tests.utils.config import PackageCompressionJob
from tests.utils.subprocess_utils import run_and_log_subprocess


def run_package_compression_script(
    compression_job: PackageCompressionJob,
    clp_package: ClpPackage,
) -> None:
    """
    Constructs and runs a compression command on the CLP package.

    :param compression_job:
    :param clp_package:
    """
    path_config = clp_package.path_config
    compress_script_path = path_config.compress_path
    temp_config_file_path = clp_package.temp_config_file_path

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

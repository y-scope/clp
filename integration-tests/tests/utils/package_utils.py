"""Provides utility functions related to the CLP package used across `integration-tests`."""

from tests.package_tests.classes import ClpPackage
from tests.utils.classes import ClpAction
from tests.utils.config import PackageCompressionJob


def run_package_compression_script(
    compression_job: PackageCompressionJob,
    clp_package: ClpPackage,
) -> None:
    """
    Constructs and runs a compression command on the CLP package.

    :param compression_job:
    :param clp_package:
    :raise AssertionError: if the compression script returns a non-zero exit code.
    """
    path_config = clp_package.path_config
    compress_cmd = [
        str(path_config.compress_path),
        "--config",
        str(clp_package.temp_config_file_path),
    ]

    if compression_job.options is not None:
        compress_cmd.extend(compression_job.options)

    if compression_job.positional_args is not None:
        compress_cmd.extend(compression_job.positional_args)

    compress_cmd.append(str(compression_job.path_to_original_dataset))

    # Run compression command for this job and fail the test if it returned a bad return code.
    compress_action = ClpAction.from_cmd(compress_cmd)
    result = compress_action.verify_returncode()
    assert result, result.failure_message

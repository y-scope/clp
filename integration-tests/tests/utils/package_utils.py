"""Provides utility functions for interacting with the CLP package."""

import logging
import subprocess

import pytest

from tests.utils.asserting_utils import run_and_assert
from tests.utils.config import (
    IntegrationTestLogs,
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


def run_package_compress_jobs(
    request: pytest.FixtureRequest,
    package_instance: PackageInstance,
) -> None:
    """
    Run all the package compress jobs for this test run.

    :param package_instance:
    :param request:
    """
    package_job_list = package_instance.package_config.package_job_list
    if package_job_list is None:
        err_msg = "Package job list is not configured for this package instance."
        raise RuntimeError(err_msg)

    compress_jobs = package_job_list.package_compress_jobs
    job_descriptions = [f"{compress_job.job_name}" for compress_job in compress_jobs]
    logger.info(
        "run_package_compress_jobs: %d job(s): %s",
        len(compress_jobs),
        job_descriptions,
    )

    package_config = package_instance.package_config
    compress_script_path = package_config.path_config.compress_script_path
    temp_config_file_path = package_config.temp_config_file_path

    for compress_job in compress_jobs:
        # Get the correct logs fixture for this job and set up path config objects.
        integration_test_logs: IntegrationTestLogs = request.getfixturevalue(
            compress_job.fixture_name
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

        compress_cmd.append(str(integration_test_logs.extraction_dir))

        # Run compression command for this job and assert that it succeeds.
        run_and_assert(compress_cmd)


def run_package_search_jobs(
    package_instance: PackageInstance,
) -> None:
    """
    Run all the package search jobs for this test run.

    :param package_instance:
    """
    package_job_list = package_instance.package_config.package_job_list
    if package_job_list is None:
        err_msg = "Package job list is not configured for this package instance."
        raise RuntimeError(err_msg)

    search_jobs = package_job_list.package_search_jobs
    job_descriptions = [f"{search_job.job_name}" for search_job in search_jobs]
    logger.info(
        "run_package_search_jobs: %d job(s): %s",
        len(search_jobs),
        job_descriptions,
    )
    # TODO: write this.
    assert True

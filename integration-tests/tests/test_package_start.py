"""Integration tests verifying that the CLP package can be started and stopped."""

import logging

import pytest

from tests.utils.asserting_utils import (
    validate_package_running,
    validate_running_mode_correct,
)
from tests.utils.clp_mode_utils import CLP_MODE_CONFIGS
from tests.utils.config import PackageInstance

TEST_MODES = CLP_MODE_CONFIGS.keys()

logger = logging.getLogger(__name__)


@pytest.mark.package
@pytest.mark.parametrize("fixt_package_config", TEST_MODES, indirect=True)
def test_clp_package(fixt_package_instance: PackageInstance) -> None:
    """
    Validate that the CLP package starts up successfully for the selected mode(s) of operation.

    :param fixt_package_instance:
    """
    # Ensure that all package components are running.
    validate_package_running(fixt_package_instance)

    # Ensure that the package is running in the correct mode.
    validate_running_mode_correct(fixt_package_instance)

    # TODO: write function `dispatch_test_jobs` and call it here; remove the logging statement.
    package_job_list = fixt_package_instance.package_config.package_job_list
    if package_job_list is not None:
        mode_name = fixt_package_instance.package_config.mode_name
        compression_job_list = package_job_list.package_compression_jobs

        compression_job_names = [job.job_name for job in compression_job_list]
        compression_job_names_text = ", ".join(compression_job_names)

        log_msg = (
            f"Compression jobs listed to run for the '{mode_name}' package test:"
            f" {compression_job_names_text}"
        )
        logger.info(log_msg)

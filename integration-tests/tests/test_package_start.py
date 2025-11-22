"""Integration tests verifying that the CLP package can be started and stopped."""

import logging

import pytest

from tests.utils.asserting_utils import (
    validate_package_running,
    validate_running_mode_correct,
)
from tests.utils.clp_job_utils import (
    dispatch_test_jobs,
)
from tests.utils.clp_mode_utils import CLP_MODE_CONFIGS
from tests.utils.config import (
    PackageInstance,
)

TEST_MODES = CLP_MODE_CONFIGS.keys()

logger = logging.getLogger(__name__)


@pytest.mark.package
@pytest.mark.parametrize("fixt_package_config", TEST_MODES, indirect=True)
def test_clp_package(fixt_package_instance: PackageInstance) -> None:
    """
    Validate that all of the components of the CLP package start up successfully for the selected
    mode of operation.

    :param fixt_package_instance:
    """
    mode_name = fixt_package_instance.package_config.mode_name
    instance_id = fixt_package_instance.clp_instance_id

    # Ensure that all package components are running.
    logger.debug(
        "Checking if all components of %s package with instance ID '%s' are running properly.",
        mode_name,
        instance_id,
    )

    validate_package_running(fixt_package_instance)

    # Ensure that the package is running in the correct mode.
    logger.debug(
        "Checking that the %s package with instance ID '%s' is running in the correct mode.",
        mode_name,
        instance_id,
    )

    validate_running_mode_correct(fixt_package_instance)

    # Run all jobs.
    package_job_list = fixt_package_instance.package_config.package_job_list
    if package_job_list is not None:
        dispatch_test_jobs(package_job_list)

"""Integration tests verifying that the CLP package can be started and stopped."""

import pytest

from tests.utils.asserting_utils import (
    validate_package_running,
    validate_presto_running,
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


@pytest.mark.package
@pytest.mark.parametrize("fixt_package_config", TEST_MODES, indirect=True)
def test_clp_package(fixt_package_instance: PackageInstance) -> None:
    """
    Validate that all of the components of the CLP package start up successfully for the selected
    mode of operation.

    :param fixt_package_instance:
    """
    # Ensure that all package components are running.
    validate_package_running(fixt_package_instance)

    # Ensure that the package is running in the correct mode.
    validate_running_mode_correct(fixt_package_instance)

    # If running a Presto cluster as part of the test, validate that it is running properly.
    mode_name = fixt_package_instance.package_config.mode_name
    if mode_name == "clp-presto":
        validate_presto_running()

    # Run all jobs.
    package_job_list = fixt_package_instance.package_config.package_job_list
    if package_job_list is not None:
        dispatch_test_jobs(fixt_package_instance)

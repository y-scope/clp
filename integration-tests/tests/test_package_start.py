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
def test_clp_package(
    request: pytest.FixtureRequest, fixt_package_instance: PackageInstance
) -> None:
    """
    Validate that the CLP package starts up successfully for the selected mode(s) of operation.

    :param fixt_package_instance:
    """
    # Ensure that all package components are running.
    validate_package_running(fixt_package_instance)

    # Ensure that the package is running in the correct mode.
    validate_running_mode_correct(fixt_package_instance)

    # Run all jobs.
    package_job_list = fixt_package_instance.package_config.package_job_list
    if package_job_list is not None:
        dispatch_test_jobs(request, fixt_package_instance)

"""Integration tests verifying that the CLP package can be started and stopped."""

import logging

import pytest

from tests.utils.clp_mode_utils import CLP_MODE_CONFIGS
from tests.utils.config import PackageInstance
from tests.utils.package_utils import (
    validate_package_running,
    validate_running_mode_correct,
)

TEST_MODES = CLP_MODE_CONFIGS.keys()

logger = logging.getLogger(__name__)


@pytest.mark.package
@pytest.mark.parametrize("clp_config", TEST_MODES, indirect=True)
def test_clp_package(clp_package: PackageInstance) -> None:
    """
    Validate that all of the components of the CLP package start up successfully for the selected
    mode of operation.

    :param clp_package:
    """
    mode_name = clp_package.package_config.mode_name
    instance_id = clp_package.clp_instance_id

    # Ensure that all package components are running.
    logger.info(
        "Checking if all components of %s package with instance ID '%s' are running properly...",
        mode_name,
        instance_id,
    )

    running, fail_msg = validate_package_running(clp_package)
    if not running:
        assert fail_msg is not None
        pytest.fail(fail_msg)

    logger.info(
        "All components of the %s package with instance ID '%s' are running properly.",
        mode_name,
        instance_id,
    )

    # Ensure that the package is running in the correct mode.
    logger.info(
        "Checking that the %s package with instance ID '%s' is running in the correct mode...",
        mode_name,
        instance_id,
    )

    correct, fail_msg = validate_running_mode_correct(clp_package)
    if not correct:
        assert fail_msg is not None
        pytest.fail(fail_msg)

    logger.info(
        "The %s package with instance ID '%s' is running in the correct mode.",
        mode_name,
        instance_id,
    )

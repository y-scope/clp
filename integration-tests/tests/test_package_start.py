"""Integration tests verifying that the CLP package can be started and stopped."""

import logging

import pytest

from tests.utils.package_utils import (
    is_package_running,
    is_running_mode_correct,
)

package_configurations = pytest.mark.parametrize(
    "test_package_fixture",
    [
        "clp_text_package",
        "clp_json_package",
    ],
)

logger = logging.getLogger(__name__)


@pytest.mark.package
@package_configurations
def test_clp_package(
    request: pytest.FixtureRequest,
    test_package_fixture: str,
) -> None:
    """
    Validate that all of the components of the clp package start up successfully. The package is
    tested in each of the configurations described in `test_package_fixture`.

    :param request:
    :param test_package_fixture:
    """
    package_instance = request.getfixturevalue(test_package_fixture)
    mode_name = package_instance.package_instance_config.mode_config.name
    instance_id = package_instance.clp_instance_id

    # Ensure that all package components are running.
    logger.info(
        "Checking if all components of %s package with instance ID '%s' are running properly...",
        mode_name,
        instance_id,
    )

    running, fail_msg = is_package_running(package_instance)
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

    correct, fail_msg = is_running_mode_correct(package_instance)
    if not correct:
        assert fail_msg is not None
        pytest.fail(fail_msg)

    logger.info(
        "The %s package with instance ID '%s' is running in the correct mode.",
        mode_name,
        instance_id,
    )

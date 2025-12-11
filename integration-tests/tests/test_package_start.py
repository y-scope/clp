"""Integration tests verifying that the CLP package can be started and stopped."""

import pytest

from tests.utils.asserting_utils import (
    validate_package_running,
)
from tests.utils.clp_mode_utils import CLP_MODE_CONFIGS
from tests.utils.config import PackageInstance

TEST_MODES = CLP_MODE_CONFIGS.keys()


@pytest.mark.package
@pytest.mark.parametrize("fixt_package_config", TEST_MODES, indirect=True)
def test_clp_package(fixt_package_instance: PackageInstance) -> None:
    """
    Validate that the CLP package starts up successfully for the selected mode(s) of operation.

    :param fixt_package_instance:
    """
    # Ensure that all package components are running.
    validate_package_running(fixt_package_instance)

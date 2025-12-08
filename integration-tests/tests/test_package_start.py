"""Integration tests verifying that the CLP package can be started and stopped."""

import logging

import pytest

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
    # TODO: write code that properly validates that the package is running. This is a placeholder.
    mode_name = fixt_package_instance.package_config.mode_name
    logger.info("The '%s' package has been spun up successfully.", mode_name)

    component_list = fixt_package_instance.package_config.component_list
    logger.info(
        "The '%s' package runs with the following required components :\n%s",
        mode_name,
        "\n".join(component_list),
    )

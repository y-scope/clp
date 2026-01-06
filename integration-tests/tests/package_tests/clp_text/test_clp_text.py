"""Tests for the clp-text package."""

import logging

import pytest
from clp_py_utils.clp_config import (
    ClpConfig,
    Package,
    QueryEngine,
    StorageEngine,
)

from tests.utils.asserting_utils import (
    validate_package_instance,
)
from tests.utils.clp_mode_utils import CLP_BASE_COMPONENTS
from tests.utils.config import PackageInstance, PackageModeConfig

logger = logging.getLogger(__name__)


# Mode description for this module.
CLP_TEXT_MODE = PackageModeConfig(
    mode_name="clp-text",
    clp_config=ClpConfig(
        package=Package(
            storage_engine=StorageEngine.CLP,
            query_engine=QueryEngine.CLP,
        ),
        api_server=None,
        log_ingestor=None,
    ),
    component_list=[
        *CLP_BASE_COMPONENTS,
    ],
)


# Pytest markers for this module.
pytestmark = [
    pytest.mark.package,
    pytest.mark.clp_text,
    pytest.mark.parametrize("fixt_package_test_config", [CLP_TEXT_MODE], indirect=True),
]


@pytest.mark.startup
def test_clp_text_startup(fixt_package_instance: PackageInstance) -> None:
    """Tests package startup."""
    validate_package_instance(fixt_package_instance)

    log_msg = "test_clp_text_startup is running successfully."
    logger.info(log_msg)

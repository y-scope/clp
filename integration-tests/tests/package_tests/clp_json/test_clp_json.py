"""Tests for the clp-json package."""

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
from tests.utils.clp_mode_utils import CLP_API_SERVER_COMPONENT, CLP_BASE_COMPONENTS
from tests.utils.config import PackageInstance, PackageModeConfig

logger = logging.getLogger(__name__)


# Mode description for this module.
CLP_JSON_MODE = PackageModeConfig(
    mode_name="clp-json",
    clp_config=ClpConfig(
        package=Package(
            storage_engine=StorageEngine.CLP_S,
            query_engine=QueryEngine.CLP_S,
        ),
    ),
    component_list=[*CLP_BASE_COMPONENTS, CLP_API_SERVER_COMPONENT],
)


# Pytest markers for this module.
pytestmark = [
    pytest.mark.package,
    pytest.mark.clp_json,
    pytest.mark.parametrize("fixt_package_test_config", [CLP_JSON_MODE], indirect=True),
]


@pytest.mark.startup
def test_clp_json_startup(fixt_package_instance: PackageInstance) -> None:
    """
    Validate that the `clp-json` package starts up successfully.

    :param fixt_package_instance:
    """
    validate_package_instance(fixt_package_instance)

    log_msg = "test_clp_json_startup was successful."
    logger.info(log_msg)


@pytest.mark.compression
def test_clp_json_compression_multifile(fixt_package_instance: PackageInstance) -> None:
    """
    Validate that the `clp-json` package successfully compresses the `json-multifile` dataset.

    :param fixt_package_instance:
    """
    validate_package_instance(fixt_package_instance)

    # TODO: compress the json-multifile dataset and check the correctness of compression.
    assert True

    log_msg = "test_clp_json_compression_multifile was successful."
    logger.info(log_msg)

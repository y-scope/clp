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
    verify_package_compression,
)
from tests.utils.clp_mode_utils import CLP_API_SERVER_COMPONENT, CLP_BASE_COMPONENTS
from tests.utils.config import PackageCompressionJob, PackageInstance, PackageModeConfig
from tests.utils.package_utils import run_package_compression_script
from tests.utils.utils import resolve_path_env_var

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
    component_list=(*CLP_BASE_COMPONENTS, CLP_API_SERVER_COMPONENT),
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
def test_clp_json_compression_json_multifile(fixt_package_instance: PackageInstance) -> None:
    """
    Validate that the `clp-json` package successfully compresses the `json-multifile` dataset.

    :param fixt_package_instance:
    """
    validate_package_instance(fixt_package_instance)

    # Clear archives before compressing.
    package_test_config = fixt_package_instance.package_test_config
    package_path_config = package_test_config.path_config
    package_path_config.clear_package_archives()

    # Compress a dataset.
    compression_job = PackageCompressionJob(
        path_to_dataset=(
            resolve_path_env_var("INTEGRATION_TESTS_PROJECT_ROOT")
            / "tests"
            / "package_tests"
            / "clp_json"
            / "data"
            / "json-multifile"
            / "logs"
        ),
        options={
            "--timestamp-key": "timestamp",
            "--dataset": "json_multifile",
        },
        positional_args=None,
    )
    run_package_compression_script(compression_job, package_test_config)

    # Check the correctness of compression.
    verify_package_compression(compression_job, package_test_config)

    log_msg = "test_clp_json_compression_json_multifile was successful."
    logger.info(log_msg)

    # Clear archives.
    package_path_config.clear_package_archives()


@pytest.mark.search
def test_clp_json_search(fixt_package_instance: PackageInstance) -> None:
    """
    Validate that the `clp-json` package successfully searches some dataset.

    :param fixt_package_instance:
    """
    validate_package_instance(fixt_package_instance)

    # TODO: compress a dataset

    # TODO: check the correctness of the compression

    # TODO: search through that dataset and check the correctness of the search results.

    assert True

    log_msg = "test_clp_json_search was successful."
    logger.info(log_msg)

    # TODO: clean up clp-package/var/data, clp-package/var/log, and clp-package/var/tmp

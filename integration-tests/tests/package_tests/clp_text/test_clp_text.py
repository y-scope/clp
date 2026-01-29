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
    verify_compression,
)
from tests.utils.clp_mode_utils import CLP_BASE_COMPONENTS
from tests.utils.config import PackageCompressionJob, PackageInstance, PackageModeConfig
from tests.utils.package_utils import run_package_compression_script
from tests.utils.utils import resolve_path_env_var

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
    component_list=(*CLP_BASE_COMPONENTS,),
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

    log_msg = "test_clp_text_startup was successful."
    logger.info(log_msg)


@pytest.mark.compression
def test_clp_text_compression_text_multifile(fixt_package_instance: PackageInstance) -> None:
    """
    Validate that the `clp-text` package successfully compresses the `text-multifile` dataset.

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
            resolve_path_env_var("INTEGRATION_TESTS_DIR")
            / "tests"
            / "package_tests"
            / "clp_text"
            / "sample_datasets"
            / "text-multifile"
            / "logs"
        ),
        options=None,
        positional_args=None,
    )
    run_package_compression_script(compression_job, package_test_config)

    # Check the correctness of compression.
    verify_compression(compression_job, package_test_config)

    log_msg = "test_clp_text_compression_text_multifile was successful."
    logger.info(log_msg)

    # Clear archives.
    package_path_config.clear_package_archives()

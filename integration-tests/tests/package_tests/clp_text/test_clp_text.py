"""Tests for the clp-text package."""

import logging

import pytest

from tests.package_tests.clp_text.utils.mode import CLP_TEXT_MODE
from tests.utils.asserting_utils import (
    validate_package_running,
    verify_package_compression,
)
from tests.utils.config import PackageCompressionJob, PackageInstance
from tests.utils.package_utils import run_package_compression_script

logger = logging.getLogger(__name__)


# Pytest markers for this module.
pytestmark = [
    pytest.mark.package,
    pytest.mark.clp_text,
    pytest.mark.parametrize("fixt_package_test_config", [CLP_TEXT_MODE], indirect=True),
]


@pytest.mark.startup
def test_clp_text_startup(fixt_package_instance: PackageInstance) -> None:
    """Tests package startup."""
    logger.info("Starting test: 'test_clp_text_startup'")

    validate_package_running(fixt_package_instance)

    logger.info("Test complete: 'test_clp_text_startup'")


@pytest.mark.compression
def test_clp_text_compression_text_multifile(fixt_package_instance: PackageInstance) -> None:
    """
    Validate that the `clp-text` package successfully compresses the `text-multifile` dataset.

    :param fixt_package_instance:
    """
    logger.info("Starting test: 'test_clp_text_compression_text_multifile'")

    validate_package_running(fixt_package_instance)

    # Clear archives before compressing.
    package_test_config = fixt_package_instance.package_test_config
    package_path_config = package_test_config.path_config
    package_path_config.clear_package_archives()

    # Compress a dataset.
    compression_job = PackageCompressionJob(
        path_to_original_dataset=(
            package_path_config.clp_text_test_data_path / "text-multifile" / "logs"
        ),
        options=None,
        positional_args=None,
    )
    run_package_compression_script(compression_job, package_test_config)

    # Check the correctness of compression.
    verify_package_compression(compression_job.path_to_original_dataset, package_test_config)

    # Clear archives.
    package_path_config.clear_package_archives()

    logger.info("Test complete: 'test_clp_text_compression_text_multifile'")

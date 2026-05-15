"""Tests for the clp-text package."""

import logging

import pytest

from tests.package_tests.classes import ClpPackage
from tests.package_tests.clp_text.utils.mode import CLP_TEXT_MODE
from tests.utils.asserting_utils import (
    validate_package_running,
    verify_package_compression,
)
from tests.utils.classes import SampleDataset
from tests.utils.config import PackageCompressionJob
from tests.utils.package_utils import run_package_compression_script

logger = logging.getLogger(__name__)
# Pytest markers for this module.
pytestmark = [
    pytest.mark.package,
    pytest.mark.clp_text,
    pytest.mark.parametrize("clp_package", [CLP_TEXT_MODE], indirect=True),
]


@pytest.mark.startup
def test_clp_text_startup(clp_package: ClpPackage) -> None:
    """Tests package startup."""
    logger.info("Starting test: 'test_clp_text_startup'")

    validate_package_running(clp_package)

    logger.info("Test complete: 'test_clp_text_startup'")


@pytest.mark.compression
def test_clp_text_compression_text_multifile(
    clp_package: ClpPackage,
    text_multifile: SampleDataset,
) -> None:
    """
    Validate that the `clp-text` package successfully compresses the `text-multifile` dataset.

    :param clp_package:
    :param text_multifile:
    """
    logger.info("Starting test: 'test_clp_text_compression_text_multifile'")

    validate_package_running(clp_package)

    # Clear archives before compressing.
    package_path_config = clp_package.path_config
    package_path_config.clear_package_archives()

    # Compress a dataset.
    compression_job = PackageCompressionJob(
        path_to_original_dataset=text_multifile.logs_path,
        options=None,
        positional_args=None,
    )
    run_package_compression_script(compression_job, clp_package)

    # Check the correctness of compression.
    verify_package_compression(text_multifile.logs_path, clp_package)

    # Clear archives.
    package_path_config.clear_package_archives()

    logger.info("Test complete: 'test_clp_text_compression_text_multifile'")

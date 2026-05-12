"""Tests for the clp-json package."""

import logging
from pathlib import Path

import pytest

from tests.package_tests.classes import ClpPackage
from tests.package_tests.clp_json.utils.mode import CLP_JSON_MODE
from tests.utils.asserting_utils import (
    validate_package_running,
    verify_package_compression,
)
from tests.utils.config import PackageCompressionJob
from tests.utils.package_utils import run_package_compression_script

logger = logging.getLogger(__name__)

CLP_JSON_TEST_DATA_DIR = Path(__file__).parent / "data"


# Pytest markers for this module.
pytestmark = [
    pytest.mark.package,
    pytest.mark.clp_json,
    pytest.mark.parametrize("clp_package", [CLP_JSON_MODE], indirect=True),
]


@pytest.mark.startup
def test_clp_json_startup(clp_package: ClpPackage) -> None:
    """
    Validate that the `clp-json` package starts up successfully.

    :param clp_package:
    """
    logger.info("Starting test: 'test_clp_json_startup'")

    assert clp_package

    logger.info("Test complete: 'test_clp_json_startup'")


@pytest.mark.compression
def test_clp_json_compression_json_multifile(clp_package: ClpPackage) -> None:
    """
    Validate that the `clp-json` package successfully compresses the `json-multifile` dataset.

    :param clp_package:
    """
    logger.info("Starting test: 'test_clp_json_compression_json_multifile'")

    # Clear archives before compressing.
    package_path_config = clp_package.path_config
    package_path_config.clear_package_archives()

    # Compress a dataset.
    compression_job = PackageCompressionJob(
        path_to_original_dataset=(CLP_JSON_TEST_DATA_DIR / "json-multifile" / "logs"),
        options=[
            "--timestamp-key",
            "timestamp",
            "--dataset",
            "json_multifile",
        ],
        positional_args=None,
    )
    run_package_compression_script(compression_job, clp_package)

    # Check the correctness of compression.
    verify_package_compression(compression_job.path_to_original_dataset, clp_package)

    # Clear archives.
    package_path_config.clear_package_archives()

    logger.info("Test complete: 'test_clp_json_compression_json_multifile'")


@pytest.mark.search
def test_clp_json_search(clp_package: ClpPackage) -> None:
    """
    Validate that the `clp-json` package successfully searches some dataset.

    :param clp_package:
    """
    logger.info("Starting test: 'test_clp_json_search'")

    validate_package_running(clp_package)

    # TODO: compress a dataset

    # TODO: check the correctness of the compression

    # TODO: search through that dataset and check the correctness of the search results.

    assert True

    logger.info("Test complete: 'test_clp_json_search'")

    # TODO: clean up clp-package/var/data, clp-package/var/log, and clp-package/var/tmp

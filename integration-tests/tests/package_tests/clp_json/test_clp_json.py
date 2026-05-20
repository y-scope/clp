"""Tests for the clp-json package."""

import logging

import pytest

from tests.package_tests.classes import ClpPackage
from tests.package_tests.clp_json.utils.mode import CLP_JSON_MODE
from tests.package_tests.utils.compress import (
    compress_clp_package,
    verify_compress_action,
)
from tests.utils.classes import SampleDataset

logger = logging.getLogger(__name__)


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
def test_clp_json_compression_json_multifile(
    clp_package: ClpPackage,
    json_multifile: SampleDataset,
) -> None:
    """
    Validate that the `clp-json` package successfully compresses the `json-multifile` dataset.

    :param clp_package:
    :param json_multifile:
    """
    logger.info("Starting test: 'test_clp_json_compression_json_multifile'")

    package_path_config = clp_package.path_config
    package_path_config.clear_package_archives()

    compress_action = compress_clp_package(clp_package, json_multifile)
    result = verify_compress_action(compress_action, clp_package, json_multifile)
    assert result, result.failure_message

    package_path_config.clear_package_archives()

    logger.info("Test complete: 'test_clp_json_compression_json_multifile'")


@pytest.mark.search
def test_clp_json_search(clp_package: ClpPackage) -> None:
    """
    Validate that the `clp-json` package successfully searches some dataset.

    :param clp_package:
    """
    logger.info("Starting test: 'test_clp_json_search'")

    # TODO: compress a dataset

    # TODO: check the correctness of the compression

    # TODO: search through that dataset and check the correctness of the search results.

    assert clp_package

    logger.info("Test complete: 'test_clp_json_search'")

    # TODO: clean up clp-package/var/data, clp-package/var/log, and clp-package/var/tmp

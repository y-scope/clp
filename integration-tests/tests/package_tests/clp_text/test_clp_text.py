"""Tests for the clp-text package."""

import logging

import pytest

from tests.package_tests.classes import (
    ClpPackage,
)
from tests.package_tests.clp_text.utils.mode import CLP_TEXT_MODE
from tests.package_tests.utils.compress import (
    compress_clp_package,
    verify_compress_action,
)
from tests.utils.classes import (
    SampleDataset,
)

logger = logging.getLogger(__name__)


# Pytest markers for this module.
pytestmark = [
    pytest.mark.package,
    pytest.mark.clp_text,
    pytest.mark.parametrize(
        "clp_package", [CLP_TEXT_MODE], indirect=True, ids=[CLP_TEXT_MODE.mode_name]
    ),
]


@pytest.mark.startup
def test_clp_text_startup(clp_package: ClpPackage) -> None:
    """
    Validate that the `clp-text` package successfully starts up.

    :param clp_package:
    """
    logger.info("Starting test: 'test_clp_text_startup'")

    assert clp_package

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

    package_path_config = clp_package.path_config
    package_path_config.clear_package_archives()

    compress_action = compress_clp_package(clp_package, text_multifile)
    result = verify_compress_action(compress_action, clp_package, text_multifile)
    assert result, result.failure_message

    package_path_config.clear_package_archives()

    logger.info("Test complete: 'test_clp_text_compression_text_multifile'")

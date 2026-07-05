"""Tests for the clp-text package."""

import logging

import pytest

from tests.package_tests.classes import ClpPackage
from tests.package_tests.clp_text.utils.mode import CLP_TEXT_MODE
from tests.package_tests.clp_text.verification.compress import verify_compress_clp_text
from tests.package_tests.utils.compress import CompressArgs
from tests.utils.classes import ClpAction, SampleDataset

logger = logging.getLogger(__name__)

# Pytest markers for this module.
pytestmark = [
    pytest.mark.package,
    pytest.mark.clp_text,
    pytest.mark.parametrize(
        "clp_package", [CLP_TEXT_MODE], indirect=True, ids=[CLP_TEXT_MODE.mode_name]
    ),
]


@pytest.mark.startstop
def test_clp_text_startstop(clp_package: ClpPackage) -> None:
    """
    Validate that the `clp-text` package successfully starts up.

    :param clp_package:
    """
    assert clp_package


@pytest.mark.compression
@pytest.mark.usefixtures("clear_package_archives")
def test_clp_text_compression_text_multifile(
    clp_package: ClpPackage,
    text_multifile: SampleDataset,
) -> None:
    """
    Validate that the `clp-text` package successfully compresses the `text-multifile` dataset.

    :param clp_package:
    :param text_multifile:
    """
    args = CompressArgs(
        script_path=clp_package.path_config.compress_path,
        config=clp_package.temp_config_file_path,
        paths=[text_multifile.logs_path],
    )

    logger.info("Compressing the 'text_multifile' dataset with the 'clp-text' package.")
    action = ClpAction.from_args(args)
    result = action.verify_returncode()
    assert result, result.failure_message

    logger.info("Verifying the compression of the 'text_multifile' dataset.")
    result = verify_compress_clp_text(action, clp_package, text_multifile)
    assert result, result.failure_message

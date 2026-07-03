"""Tests for the clp-text package."""

import logging

import pytest

from tests.package_tests.classes import ClpPackage
from tests.package_tests.clp_text.utils.mode import CLP_TEXT_MODE
from tests.package_tests.clp_text.verification.compress import verify_compress_clp_text
from tests.package_tests.clp_text.verification.search import verify_search_clp_text
from tests.package_tests.utils.compress import CompressArgs
from tests.package_tests.utils.search import SearchArgs
from tests.utils.classes import ClpAction, SampleDataset, SampleDatasetMetadata

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


@pytest.mark.search
@pytest.mark.usefixtures("clear_package_archives")
def test_clp_text_search_time_range_text_multifile(
    clp_package: ClpPackage,
    text_multifile: SampleDataset,
) -> None:
    """
    Validate that the `clp-text` package honours `--begin-time`/`--end-time` when searching the
    `text-multifile` dataset. The time range is a strict sub-range of the dataset's span
    (`[begin_ts + 1, end_ts - 1]`), which is guaranteed to exclude the earliest and latest log
    events.

    :param clp_package:
    :param text_multifile:
    """
    metadata: SampleDatasetMetadata = text_multifile.metadata
    compress_args = CompressArgs(
        script_path=clp_package.path_config.compress_path,
        config=clp_package.temp_config_file_path,
        paths=[text_multifile.logs_path],
    )

    logger.info("Compressing the 'text_multifile' dataset with the 'clp-text' package.")
    compress_action = ClpAction.from_args(compress_args)
    compress_result = compress_action.verify_returncode()
    assert compress_result, compress_result.failure_message

    logger.info("Verifying the compression of the 'text_multifile' dataset.")
    compress_result = verify_compress_clp_text(compress_action, clp_package, text_multifile)
    assert compress_result, compress_result.failure_message

    search_args = SearchArgs(
        script_path=clp_package.path_config.search_path,
        config=clp_package.temp_config_file_path,
        query="nominal",
        begin_ts=metadata.begin_ts + 1,
        end_ts=metadata.end_ts - 1,
    )

    logger.info(
        "Searching the 'text_multifile' dataset over a time range with the 'clp-text' package."
    )
    search_action = ClpAction.from_args(search_args)
    search_result = search_action.verify_returncode()
    assert search_result, search_result.failure_message

    logger.info("Verifying the time-range search results for the 'text_multifile' dataset.")
    search_result = verify_search_clp_text(search_action, text_multifile)
    assert search_result, search_result.failure_message

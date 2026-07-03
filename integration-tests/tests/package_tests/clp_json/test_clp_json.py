"""Tests for the clp-json package."""

import logging

import pytest

from tests.package_tests.classes import ClpPackage
from tests.package_tests.clp_json.utils.mode import CLP_JSON_MODE
from tests.package_tests.clp_json.verification.compress import (
    verify_compress_structured_clp_json,
    verify_compress_unstructured_clp_json,
)
from tests.package_tests.clp_json.verification.search import verify_search_clp_json
from tests.package_tests.utils.compress import CompressArgs
from tests.package_tests.utils.search import SearchArgs
from tests.utils.classes import (
    ClpAction,
    SampleDataset,
    SampleDatasetMetadata,
)

logger = logging.getLogger(__name__)


# Pytest markers for this module.
pytestmark = [
    pytest.mark.package,
    pytest.mark.clp_json,
    pytest.mark.parametrize(
        "clp_package", [CLP_JSON_MODE], indirect=True, ids=[CLP_JSON_MODE.mode_name]
    ),
]


@pytest.mark.startstop
def test_clp_json_startstop(clp_package: ClpPackage) -> None:
    """
    Validate that the `clp-json` package starts up successfully.

    :param clp_package:
    """
    assert clp_package


@pytest.mark.compression
@pytest.mark.usefixtures("clear_package_archives")
def test_clp_json_compression_json_multifile(
    clp_package: ClpPackage,
    json_multifile: SampleDataset,
) -> None:
    """
    Validate that the `clp-json` package successfully compresses the `json-multifile` dataset.

    :param clp_package:
    :param json_multifile:
    """
    metadata: SampleDatasetMetadata = json_multifile.metadata
    args: CompressArgs = CompressArgs(
        script_path=clp_package.path_config.compress_path,
        config=clp_package.temp_config_file_path,
        dataset=metadata.dataset_name,
        timestamp_key=metadata.timestamp_key,
        unstructured=metadata.unstructured,
        paths=[json_multifile.logs_path],
    )

    logger.info("Compressing the 'json_multifile' dataset with the 'clp-json' package.")
    action = ClpAction.from_args(args)
    result = action.verify_returncode()
    assert result, result.failure_message

    logger.info("Verifying the compression of the 'json_multifile' dataset.")
    result = verify_compress_structured_clp_json(action, clp_package, json_multifile)
    assert result, result.failure_message


@pytest.mark.compression
@pytest.mark.usefixtures("clear_package_archives")
def test_clp_json_compression_text_multifile(
    clp_package: ClpPackage,
    text_multifile: SampleDataset,
) -> None:
    """
    Validate that the `clp-json` package successfully compresses the `text-multifile` dataset as
    unstructured text.

    :param clp_package:
    :param text_multifile:
    """
    metadata: SampleDatasetMetadata = text_multifile.metadata
    args: CompressArgs = CompressArgs(
        script_path=clp_package.path_config.compress_path,
        config=clp_package.temp_config_file_path,
        dataset=metadata.dataset_name,
        timestamp_key=metadata.timestamp_key,
        unstructured=metadata.unstructured,
        paths=[text_multifile.logs_path],
    )

    logger.info("Compressing the 'text_multifile' dataset with the 'clp-json' package.")
    action = ClpAction.from_args(args)
    result = action.verify_returncode()
    assert result, result.failure_message

    logger.info("Verifying the compression of the 'text_multifile' dataset.")
    result = verify_compress_unstructured_clp_json(action, clp_package, text_multifile)
    assert result, result.failure_message


@pytest.mark.search
@pytest.mark.usefixtures("clear_package_archives")
def test_clp_json_search_json_multifile(
    clp_package: ClpPackage,
    json_multifile: SampleDataset,
) -> None:
    """
    Validate that the `clp-json` package successfully searches the `json-multifile` dataset.

    :param clp_package:
    :param json_multifile:
    """
    metadata: SampleDatasetMetadata = json_multifile.metadata
    compress_args: CompressArgs = CompressArgs(
        script_path=clp_package.path_config.compress_path,
        config=clp_package.temp_config_file_path,
        dataset=metadata.dataset_name,
        timestamp_key=metadata.timestamp_key,
        unstructured=metadata.unstructured,
        paths=[json_multifile.logs_path],
    )

    logger.info("Compressing the 'json_multifile' dataset with the 'clp-json' package.")
    compress_action = ClpAction.from_args(compress_args)
    compress_result = compress_action.verify_returncode()
    assert compress_result, compress_result.failure_message

    logger.info("Verifying the compression of the 'json_multifile' dataset.")
    compress_result = verify_compress_structured_clp_json(
        compress_action, clp_package, json_multifile
    )
    assert compress_result, compress_result.failure_message

    search_args: SearchArgs = SearchArgs(
        script_path=clp_package.path_config.search_path,
        config=clp_package.temp_config_file_path,
        query='detail: "*maximum dynamic*"',
        dataset=metadata.dataset_name,
    )

    logger.info("Searching the 'json_multifile' dataset with the 'clp-json' package.")
    search_action = ClpAction.from_args(search_args)
    search_result = search_action.verify_returncode()
    assert search_result, search_result.failure_message

    logger.info("Verifying the search results for the 'json_multifile' dataset.")
    search_result = verify_search_clp_json(search_action, clp_package, json_multifile)
    assert search_result, search_result.failure_message


@pytest.mark.search
@pytest.mark.usefixtures("clear_package_archives")
def test_clp_json_search_time_range_json_multifile(
    clp_package: ClpPackage,
    json_multifile: SampleDataset,
) -> None:
    """
    Validate that the `clp-json` package honours `--begin-time`/`--end-time` when searching the
    `json-multifile` dataset. The time range is a strict sub-range of the dataset's span
    (`[begin_ts + 1, end_ts - 1]`), which is guaranteed to exclude the earliest and latest records.

    :param clp_package:
    :param json_multifile:
    """
    metadata: SampleDatasetMetadata = json_multifile.metadata
    compress_args: CompressArgs = CompressArgs(
        script_path=clp_package.path_config.compress_path,
        config=clp_package.temp_config_file_path,
        dataset=metadata.dataset_name,
        timestamp_key=metadata.timestamp_key,
        unstructured=metadata.unstructured,
        paths=[json_multifile.logs_path],
    )

    logger.info("Compressing the 'json_multifile' dataset with the 'clp-json' package.")
    compress_action = ClpAction.from_args(compress_args)
    compress_result = compress_action.verify_returncode()
    assert compress_result, compress_result.failure_message

    logger.info("Verifying the compression of the 'json_multifile' dataset.")
    compress_result = verify_compress_structured_clp_json(
        compress_action, clp_package, json_multifile
    )
    assert compress_result, compress_result.failure_message

    search_args: SearchArgs = SearchArgs(
        script_path=clp_package.path_config.search_path,
        config=clp_package.temp_config_file_path,
        query='mission: "STS-135"',
        dataset=metadata.dataset_name,
        begin_ts=metadata.begin_ts + 1,
        end_ts=metadata.end_ts - 1,
    )

    logger.info(
        "Searching the 'json_multifile' dataset over a time range with the 'clp-json' package."
    )
    search_action = ClpAction.from_args(search_args)
    search_result = search_action.verify_returncode()
    assert search_result, search_result.failure_message

    logger.info("Verifying the time-range search results for the 'json_multifile' dataset.")
    search_result = verify_search_clp_json(search_action, clp_package, json_multifile)
    assert search_result, search_result.failure_message


@pytest.mark.search
@pytest.mark.usefixtures("clear_package_archives")
def test_clp_json_search_text_multifile(
    clp_package: ClpPackage,
    text_multifile: SampleDataset,
) -> None:
    """
    Validate that the `clp-json` package successfully searches the `text-multifile` dataset.

    :param clp_package:
    :param text_multifile:
    """
    metadata: SampleDatasetMetadata = text_multifile.metadata
    args: CompressArgs = CompressArgs(
        script_path=clp_package.path_config.compress_path,
        config=clp_package.temp_config_file_path,
        dataset=metadata.dataset_name,
        timestamp_key=metadata.timestamp_key,
        unstructured=metadata.unstructured,
        paths=[text_multifile.logs_path],
    )

    logger.info("Compressing the 'text_multifile' dataset with the 'clp-json' package.")
    action = ClpAction.from_args(args)
    result = action.verify_returncode()
    assert result, result.failure_message

    logger.info("Verifying the compression of the 'text_multifile' dataset.")
    result = verify_compress_unstructured_clp_json(action, clp_package, text_multifile)
    assert result, result.failure_message

    search_args: SearchArgs = SearchArgs(
        script_path=clp_package.path_config.search_path,
        config=clp_package.temp_config_file_path,
        query="*Saturn*",
        dataset=metadata.dataset_name,
    )

    logger.info("Searching the 'text_multifile' dataset with the 'clp-json' package.")
    search_action = ClpAction.from_args(search_args)
    search_result = search_action.verify_returncode()
    assert search_result, search_result.failure_message

    logger.info("Verifying the search results for the 'text_multifile' dataset.")
    search_result = verify_search_clp_json(search_action, clp_package, text_multifile)
    assert search_result, search_result.failure_message

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
    Validate that the `clp-json` package successfully compresses the `json_multifile` dataset.

    :param clp_package:
    :param json_multifile:
    """
    _compress_structured_dataset(clp_package, json_multifile)


@pytest.mark.compression
@pytest.mark.usefixtures("clear_package_archives")
def test_clp_json_compression_text_multifile(
    clp_package: ClpPackage,
    text_multifile: SampleDataset,
) -> None:
    """
    Validate that the `clp-json` package successfully compresses the `text_multifile` dataset as
    unstructured text.

    :param clp_package:
    :param text_multifile:
    """
    _compress_unstructured_dataset(clp_package, text_multifile)


@pytest.mark.search
@pytest.mark.usefixtures("clear_package_archives")
def test_clp_json_search_basic_json_multifile(
    clp_package: ClpPackage,
    json_multifile: SampleDataset,
) -> None:
    """
    Validate that the `clp-json` package performs a basic search on the `json_multifile` dataset.

    :param clp_package:
    :param json_multifile:
    """
    _compress_structured_dataset(clp_package, json_multifile)
    _search_basic(clp_package, json_multifile, 'detail: "*maximum dynamic*"')


@pytest.mark.search
@pytest.mark.usefixtures("clear_package_archives")
def test_clp_json_search_ignore_case_json_multifile(
    clp_package: ClpPackage,
    json_multifile: SampleDataset,
) -> None:
    """
    Validate that the `clp-json` package performs a case-insensitive search on the `json_multifile`
    dataset.

    :param clp_package:
    :param json_multifile:
    """
    _compress_structured_dataset(clp_package, json_multifile)
    _search_ignore_case(clp_package, json_multifile, 'detail: "*mAxImUm DyNaMiC*"')


@pytest.mark.search
@pytest.mark.usefixtures("clear_package_archives")
def test_clp_json_search_count_results_json_multifile(
    clp_package: ClpPackage,
    json_multifile: SampleDataset,
) -> None:
    """
    Validate that the `clp-json` package performs a count search on the `json_multifile` dataset.

    :param clp_package:
    :param json_multifile:
    """
    _compress_structured_dataset(clp_package, json_multifile)
    _search_count_results(clp_package, json_multifile, 'detail: "*maximum dynamic*"')


@pytest.mark.search
@pytest.mark.usefixtures("clear_package_archives")
def test_clp_json_search_count_by_time_json_multifile(
    clp_package: ClpPackage,
    json_multifile: SampleDataset,
) -> None:
    """
    Validate that the `clp-json` package performs a count-by-time search on the `json_multifile`
    dataset.

    :param clp_package:
    :param json_multifile:
    """
    _compress_structured_dataset(clp_package, json_multifile)
    _search_count_by_time(clp_package, json_multifile, 'mission: "STS-135"', 10)


@pytest.mark.search
@pytest.mark.usefixtures("clear_package_archives")
def test_clp_json_search_time_range_json_multifile(
    clp_package: ClpPackage,
    json_multifile: SampleDataset,
) -> None:
    """
    Validate that the `clp-json` package honours `--begin-time`/`--end-time` when searching the
    `json_multifile` dataset. The time range is a strict sub-range of the dataset's span
    (`[begin_ts + 1, end_ts - 1]`), which is guaranteed to exclude the earliest and latest records.

    :param clp_package:
    :param json_multifile:
    """
    _compress_structured_dataset(clp_package, json_multifile)
    metadata: SampleDatasetMetadata = json_multifile.metadata
    begin_ts = metadata.begin_ts + 1
    end_ts = metadata.end_ts - 1
    _search_time_range(clp_package, json_multifile, 'mission: "STS-135"', begin_ts, end_ts)


@pytest.mark.search
@pytest.mark.usefixtures("clear_package_archives")
def test_clp_json_search_basic_text_multifile(
    clp_package: ClpPackage,
    text_multifile: SampleDataset,
) -> None:
    """
    Validate that the `clp-json` package performs a basic search on the `text_multifile` dataset.

    :param clp_package:
    :param text_multifile:
    """
    _compress_unstructured_dataset(clp_package, text_multifile)
    _search_basic(clp_package, text_multifile, "*Saturn*")


@pytest.mark.search
@pytest.mark.usefixtures("clear_package_archives")
def test_clp_json_search_ignore_case_text_multifile(
    clp_package: ClpPackage,
    text_multifile: SampleDataset,
) -> None:
    """
    Validate that the `clp-json` package performs a case-insensitive search on the `text_multifile`
    dataset.

    :param clp_package:
    :param text_multifile:
    """
    _compress_unstructured_dataset(clp_package, text_multifile)
    _search_ignore_case(clp_package, text_multifile, "*sAtUrN*")


@pytest.mark.search
@pytest.mark.usefixtures("clear_package_archives")
def test_clp_json_search_count_results_text_multifile(
    clp_package: ClpPackage,
    text_multifile: SampleDataset,
) -> None:
    """
    Validate that the `clp-json` package performs a count search on the `text_multifile` dataset.

    :param clp_package:
    :param text_multifile:
    """
    _compress_unstructured_dataset(clp_package, text_multifile)
    _search_count_results(clp_package, text_multifile, "*apollo-17*")


@pytest.mark.search
@pytest.mark.usefixtures("clear_package_archives")
def test_clp_json_search_count_by_time_text_multifile(
    clp_package: ClpPackage,
    text_multifile: SampleDataset,
) -> None:
    """
    Validate that the `clp-json` package performs a count-by-time search on the `text_multifile`
    dataset.

    :param clp_package:
    :param text_multifile:
    """
    _compress_unstructured_dataset(clp_package, text_multifile)
    _search_count_by_time(clp_package, text_multifile, "*apollo-17*", 10)


@pytest.mark.search
@pytest.mark.usefixtures("clear_package_archives")
def test_clp_json_search_time_range_text_multifile(
    clp_package: ClpPackage,
    text_multifile: SampleDataset,
) -> None:
    """
    Validate that the `clp-json` package honours `--begin-time`/`--end-time` when searching the
    `text_multifile` dataset. The time range is a strict sub-range of the dataset's span
    (`[begin_ts + 1, end_ts - 1]`), which is guaranteed to exclude the earliest and latest records.

    :param clp_package:
    :param text_multifile:
    """
    _compress_unstructured_dataset(clp_package, text_multifile)
    metadata: SampleDatasetMetadata = text_multifile.metadata
    begin_ts = metadata.begin_ts + 1
    end_ts = metadata.end_ts - 1
    _search_time_range(clp_package, text_multifile, "*", begin_ts, end_ts)


@pytest.mark.search
@pytest.mark.usefixtures("clear_package_archives")
def test_clp_json_search_basic_text_singlefile(
    clp_package: ClpPackage,
    text_singlefile: SampleDataset,
) -> None:
    """
    Validate that the `clp-json` package performs a basic search on the `text_singlefile` dataset.

    :param clp_package:
    :param text_singlefile:
    """
    _compress_unstructured_dataset(clp_package, text_singlefile)
    _search_basic(clp_package, text_singlefile, "*TEST1*")


@pytest.mark.search
@pytest.mark.usefixtures("clear_package_archives")
def test_clp_json_search_ignore_case_text_singlefile(
    clp_package: ClpPackage,
    text_singlefile: SampleDataset,
) -> None:
    """
    Validate that the `clp-json` package performs a case-insensitive search on the
    `text_singlefile` dataset.

    :param clp_package:
    :param text_singlefile:
    """
    _compress_unstructured_dataset(clp_package, text_singlefile)
    _search_ignore_case(clp_package, text_singlefile, "*tEsT1*")


@pytest.mark.search
@pytest.mark.usefixtures("clear_package_archives")
def test_clp_json_search_count_results_text_singlefile(
    clp_package: ClpPackage,
    text_singlefile: SampleDataset,
) -> None:
    """
    Validate that the `clp-json` package performs a count search on the `text_singlefile` dataset.

    :param clp_package:
    :param text_singlefile:
    """
    _compress_unstructured_dataset(clp_package, text_singlefile)
    _search_count_results(clp_package, text_singlefile, "*TEST*")


@pytest.mark.search
@pytest.mark.usefixtures("clear_package_archives")
def test_clp_json_search_count_by_time_text_singlefile(
    clp_package: ClpPackage,
    text_singlefile: SampleDataset,
) -> None:
    """
    Validate that the `clp-json` package performs a count-by-time search on the `text_singlefile`
    dataset.

    :param clp_package:
    :param text_singlefile:
    """
    _compress_unstructured_dataset(clp_package, text_singlefile)
    _search_count_by_time(clp_package, text_singlefile, "*TEST*", 10)


def _compress_structured_dataset(clp_package: ClpPackage, dataset: SampleDataset) -> None:
    action = _run_compress(clp_package, dataset)

    logger.info("Verifying the compression of the '%s' dataset.", dataset.metadata.dataset_name)
    result = verify_compress_structured_clp_json(action, clp_package, dataset)
    assert result, result.failure_message


def _compress_unstructured_dataset(clp_package: ClpPackage, dataset: SampleDataset) -> None:
    action = _run_compress(clp_package, dataset)

    logger.info("Verifying the compression of the '%s' dataset.", dataset.metadata.dataset_name)
    result = verify_compress_unstructured_clp_json(action, clp_package, dataset)
    assert result, result.failure_message


def _search_basic(clp_package: ClpPackage, dataset: SampleDataset, query: str) -> None:
    logger.info("Performing BASIC search on the '%s' dataset.", dataset.metadata.dataset_name)
    _run_search(clp_package, dataset, query)


def _search_ignore_case(clp_package: ClpPackage, dataset: SampleDataset, query: str) -> None:
    logger.info("Performing IGNORE-CASE search on the '%s' dataset.", dataset.metadata.dataset_name)
    _run_search(clp_package, dataset, query, ignore_case=True)


def _search_count_results(clp_package: ClpPackage, dataset: SampleDataset, query: str) -> None:
    logger.info("Performing COUNT search on the '%s' dataset.", dataset.metadata.dataset_name)
    _run_search(clp_package, dataset, query, count=True)


def _search_count_by_time(
    clp_package: ClpPackage, dataset: SampleDataset, query: str, time_interval: int
) -> None:
    logger.info(
        "Performing COUNT-BY-TIME search on the '%s' dataset.", dataset.metadata.dataset_name
    )
    _run_search(clp_package, dataset, query, count_by_time=time_interval)


def _search_time_range(
    clp_package: ClpPackage,
    dataset: SampleDataset,
    query: str,
    begin_ts: int | None,
    end_ts: int | None,
) -> None:
    logger.info("Performing TIME-RANGE search on the '%s' dataset.", dataset.metadata.dataset_name)
    _run_search(clp_package, dataset, query, begin_ts=begin_ts, end_ts=end_ts)


def _run_compress(clp_package: ClpPackage, dataset: SampleDataset) -> ClpAction:
    """
    Compresses `dataset` with the `clp-json` package and verifies the action's exit code. This is
    the mode-agnostic mechanism shared by the module's compression helpers; the caller is
    responsible for verifying the compressed output.

    :param clp_package:
    :param dataset:
    :return: The completed compression action, ready to be passed to a verification helper.
    """
    metadata: SampleDatasetMetadata = dataset.metadata
    args = CompressArgs(
        script_path=clp_package.path_config.compress_path,
        config=clp_package.temp_config_file_path,
        dataset=metadata.dataset_name,
        timestamp_key=metadata.timestamp_key,
        unstructured=metadata.unstructured,
        paths=[dataset.logs_path],
    )

    logger.info("Compressing the '%s' dataset with the 'clp-json' package.", metadata.dataset_name)
    action = ClpAction.from_args(args)
    result = action.verify_returncode()
    assert result, result.failure_message

    return action


def _run_search(  # noqa: PLR0913
    clp_package: ClpPackage,
    dataset: SampleDataset,
    query: str,
    *,
    ignore_case: bool = False,
    count: bool = False,
    count_by_time: int | None = None,
    begin_ts: int | None = None,
    end_ts: int | None = None,
) -> None:
    """
    Searches `dataset` with the `clp-json` package for `query` and verifies both the action's exit
    code and its output. This is the mode-agnostic mechanism shared by the module's search helpers;
    the caller selects the search variant via the keyword-only flags and logs the variant being
    performed.

    :param clp_package:
    :param dataset:
    :param query:
    :param ignore_case:
    :param count:
    :param count_by_time:
    :param begin_ts:
    :param end_ts:
    """
    metadata: SampleDatasetMetadata = dataset.metadata
    args = SearchArgs(
        script_path=clp_package.path_config.search_path,
        config=clp_package.temp_config_file_path,
        query=query,
        dataset=metadata.dataset_name,
        ignore_case=ignore_case,
        count=count,
        count_by_time=count_by_time,
        begin_ts=begin_ts,
        end_ts=end_ts,
    )

    action = ClpAction.from_args(args)
    result = action.verify_returncode()
    assert result, result.failure_message

    logger.info("Verifying the search results on the '%s' dataset.", metadata.dataset_name)
    verification = verify_search_clp_json(action, clp_package, dataset)
    assert verification, verification.failure_message

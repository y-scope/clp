"""Tests for the clp-text package."""

import logging
from pathlib import Path

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
    Validate that the `clp-text` package successfully compresses the `text_multifile` dataset.

    :param clp_package:
    :param text_multifile:
    """
    _compress_dataset(clp_package, text_multifile)


@pytest.mark.compression
@pytest.mark.usefixtures("clear_package_archives")
def test_clp_text_compression_text_singlefile(
    clp_package: ClpPackage,
    text_singlefile: SampleDataset,
) -> None:
    """
    Validate that the `clp-text` package successfully compresses the `text_singlefile` dataset.

    :param clp_package:
    :param text_singlefile:
    """
    _compress_dataset(clp_package, text_singlefile)


@pytest.mark.search
@pytest.mark.usefixtures("clear_package_archives")
def test_clp_text_search_basic_text_multifile(
    clp_package: ClpPackage,
    text_multifile: SampleDataset,
) -> None:
    """
    Validate that the `clp-text` package performs a basic search on the `text_multifile` dataset.

    :param clp_package:
    :param text_multifile:
    """
    _compress_dataset(clp_package, text_multifile)
    _search_basic(clp_package, text_multifile, "Saturn")


@pytest.mark.search
@pytest.mark.usefixtures("clear_package_archives")
def test_clp_text_search_ignore_case_text_multifile(
    clp_package: ClpPackage,
    text_multifile: SampleDataset,
) -> None:
    """
    Validate that the `clp-text` package performs a case-insensitive search on the `text_multifile`
    dataset.

    :param clp_package:
    :param text_multifile:
    """
    _compress_dataset(clp_package, text_multifile)
    _search_ignore_case(clp_package, text_multifile, "sAtUrN")


@pytest.mark.search
@pytest.mark.usefixtures("clear_package_archives")
def test_clp_text_search_count_results_text_multifile(
    clp_package: ClpPackage,
    text_multifile: SampleDataset,
) -> None:
    """
    Validate that the `clp-text` package performs a count search on the `text_multifile` dataset.

    :param clp_package:
    :param text_multifile:
    """
    _compress_dataset(clp_package, text_multifile)
    _search_count_results(clp_package, text_multifile, "apollo-17")


@pytest.mark.search
@pytest.mark.usefixtures("clear_package_archives")
def test_clp_text_search_count_by_time_text_multifile(
    clp_package: ClpPackage,
    text_multifile: SampleDataset,
) -> None:
    """
    Validate that the `clp-text` package performs a count-by-time search on the `text_multifile`
    dataset.

    :param clp_package:
    :param text_multifile:
    """
    _compress_dataset(clp_package, text_multifile)
    _search_count_by_time(clp_package, text_multifile, "apollo-17", 10)


@pytest.mark.search
@pytest.mark.usefixtures("clear_package_archives")
def test_clp_text_search_time_range_text_multifile(
    clp_package: ClpPackage,
    text_multifile: SampleDataset,
) -> None:
    """
    Validate that the `clp-text` package honours `--begin-time`/`--end-time` when searching the
    `text_multifile` dataset. The time range is a strict sub-range of the dataset's span
    (`[begin_ts + 1, end_ts - 1]`), which is guaranteed to exclude the earliest and latest log
    events.

    :param clp_package:
    :param text_multifile:
    """
    _compress_dataset(clp_package, text_multifile)
    metadata: SampleDatasetMetadata = text_multifile.metadata
    begin_ts = metadata.begin_ts + 1
    end_ts = metadata.end_ts - 1
    _search_time_range(clp_package, text_multifile, "nominal", begin_ts, end_ts)


@pytest.mark.search
@pytest.mark.usefixtures("clear_package_archives")
def test_clp_text_search_file_path_text_multifile(
    clp_package: ClpPackage,
    text_multifile: SampleDataset,
) -> None:
    """
    Validate that the `clp-text` package performs a search correctly with the `--file-path` flag on
    the `text_multifile` dataset.

    :param clp_package:
    :param text_multifile:
    """
    _compress_dataset(clp_package, text_multifile)
    metadata: SampleDatasetMetadata = text_multifile.metadata
    file_path = text_multifile.logs_path / metadata.file_names[0]
    _search_file_path(clp_package, text_multifile, "nominal", file_path)


@pytest.mark.search
@pytest.mark.usefixtures("clear_package_archives")
def test_clp_text_search_basic_text_singlefile(
    clp_package: ClpPackage,
    text_singlefile: SampleDataset,
) -> None:
    """
    Validate that the `clp-text` package performs a basic search on the `text_singlefile` dataset.

    :param clp_package:
    :param text_singlefile:
    """
    _compress_dataset(clp_package, text_singlefile)
    _search_basic(clp_package, text_singlefile, "TEST1")


@pytest.mark.search
@pytest.mark.usefixtures("clear_package_archives")
def test_clp_text_search_ignore_case_text_singlefile(
    clp_package: ClpPackage,
    text_singlefile: SampleDataset,
) -> None:
    """
    Validate that the `clp-text` package performs a case-insensitive search on the `text_singlefile`
    dataset.

    :param clp_package:
    :param text_singlefile:
    """
    _compress_dataset(clp_package, text_singlefile)
    _search_ignore_case(clp_package, text_singlefile, "tEsT1")


@pytest.mark.search
@pytest.mark.usefixtures("clear_package_archives")
def test_clp_text_search_count_results_text_singlefile(
    clp_package: ClpPackage,
    text_singlefile: SampleDataset,
) -> None:
    """
    Validate that the `clp-text` package performs a count search on the `text_singlefile` dataset.

    :param clp_package:
    :param text_singlefile:
    """
    _compress_dataset(clp_package, text_singlefile)
    _search_count_results(clp_package, text_singlefile, "TEST")


@pytest.mark.search
@pytest.mark.usefixtures("clear_package_archives")
def test_clp_text_search_count_by_time_text_singlefile(
    clp_package: ClpPackage,
    text_singlefile: SampleDataset,
) -> None:
    """
    Validate that the `clp-text` package performs a count-by-time search on the `text_singlefile`
    dataset.

    :param clp_package:
    :param text_singlefile:
    """
    _compress_dataset(clp_package, text_singlefile)
    _search_count_by_time(clp_package, text_singlefile, "TEST", 10)


@pytest.mark.search
@pytest.mark.usefixtures("clear_package_archives")
def test_clp_text_search_file_path_text_singlefile(
    clp_package: ClpPackage,
    text_singlefile: SampleDataset,
) -> None:
    """
    Validate that the `clp-text` package performs a search correctly with the `--file-path` flag on
    the `text_singlefile` dataset.

    :param clp_package:
    :param text_singlefile:
    """
    _compress_dataset(clp_package, text_singlefile)
    metadata: SampleDatasetMetadata = text_singlefile.metadata
    file_path = text_singlefile.logs_path / metadata.file_names[0]
    _search_file_path(clp_package, text_singlefile, "TEST5", file_path)


def _compress_dataset(clp_package: ClpPackage, dataset: SampleDataset) -> None:
    action = _run_compress(clp_package, dataset)

    logger.info("Verifying the compression of the '%s' dataset.", dataset.metadata.dataset_name)
    result = verify_compress_clp_text(action, clp_package, dataset)
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


def _search_file_path(
    clp_package: ClpPackage,
    dataset: SampleDataset,
    query: str,
    file_path: Path,
) -> None:
    logger.info("Performing FILE-PATH search on the '%s' dataset.", dataset.metadata.dataset_name)
    _run_search(clp_package, dataset, query, file_path=file_path)


def _run_compress(clp_package: ClpPackage, dataset: SampleDataset) -> ClpAction:
    """
    Compresses `dataset` with the `clp-text` package and verifies the action's exit code. This is
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
        paths=[dataset.logs_path],
    )

    logger.info("Compressing the '%s' dataset with the 'clp-text' package.", metadata.dataset_name)
    action = ClpAction.from_args(args)
    result = action.verify_returncode()
    assert result, result.failure_message

    return action


def _run_search(  # noqa: PLR0913
    clp_package: ClpPackage,
    dataset: SampleDataset,
    query: str,
    *,
    file_path: Path | None = None,
    ignore_case: bool = False,
    count: bool = False,
    count_by_time: int | None = None,
    begin_ts: int | None = None,
    end_ts: int | None = None,
) -> None:
    """
    Searches `dataset` with the `clp-text` package for `query` and verifies both the action's exit
    code and its output. This is the mode-agnostic mechanism shared by the module's search helpers;
    the caller selects the search variant via the keyword-only flags and logs the variant being
    performed.

    :param clp_package:
    :param dataset:
    :param query:
    :param file_path:
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
        file_path=file_path,
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
    verification = verify_search_clp_text(action, dataset)
    assert verification, verification.failure_message

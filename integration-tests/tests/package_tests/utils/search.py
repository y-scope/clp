"""Functions and classes to facilitate CLP package search."""

import logging
import re
from enum import auto, Enum
from pathlib import Path

import pytest
from clp_py_utils.clp_config import StorageEngine

from tests.package_tests.classes import ClpPackage
from tests.utils.classes import (
    ClpAction,
    ClpVerificationResult,
    CmdArgs,
    NonClpAction,
    SampleDataset,
)
from tests.utils.utils import (
    get_binary_path,
)

logger = logging.getLogger(__name__)


DEFAULT_COUNT_BY_TIME_INTERVAL = 10


class SearchArgs(CmdArgs):
    """Command argument model for searching with the CLP package."""

    script_path: Path
    config: Path
    wildcard_query: str
    raw: bool = True
    dataset: str | None = None
    file_path: Path | None = None
    ignore_case: bool = False
    count: bool = False
    count_by_time: int | None = None
    begin_ts: int | None = None
    end_ts: int | None = None

    def to_cmd(self) -> list[str]:
        """Converts the model attributes to a command list."""
        cmd: list[str] = [
            str(self.script_path),
            "--config",
            str(self.config),
        ]

        if self.dataset:
            cmd.append("--dataset")
            cmd.append(self.dataset)
        if self.file_path:
            cmd.append("--file-path")
            cmd.append(str(self.file_path))
        if self.ignore_case:
            cmd.append("--ignore-case")
        if self.count:
            cmd.append("--count")
        if self.count_by_time is not None:
            cmd.append("--count-by-time")
            cmd.append(str(self.count_by_time))
        if self.begin_ts is not None:
            cmd.append("--begin-time")
            cmd.append(str(self.begin_ts))
        if self.end_ts is not None:
            cmd.append("--end-time")
            cmd.append(str(self.end_ts))
        if self.raw:
            cmd.append("--raw")

        cmd.append(self.wildcard_query)

        return cmd


class ClpPackageSearchType(Enum):
    """Possible search types."""

    BASIC = auto()
    FILE_PATH = auto()
    IGNORE_CASE = auto()
    COUNT_RESULTS = auto()
    COUNT_BY_TIME = auto()
    TIME_RANGE = auto()


def search_clp_package(
    clp_package: ClpPackage,
    dataset: SampleDataset,
    search_type: ClpPackageSearchType,
    wildcard_query: str,
) -> ClpAction:
    """
    Performs the specified search on the dataset using the CLP package.

    :param clp_package:
    :param dataset:
    :param search_type:
    :param wildcard_query:
    :return: The `ClpAction` instance that runs the search.
    """
    logger.info(
        "Performing '%s' search on the '%s' dataset.",
        search_type.name,
        dataset.dataset_name,
    )

    args: SearchArgs = _construct_args(clp_package, dataset, search_type, wildcard_query)
    return ClpAction(cmd=args.to_cmd(), args=args)


def _construct_args(
    clp_package: ClpPackage,
    dataset: SampleDataset,
    search_type: ClpPackageSearchType,
    wildcard_query: str,
) -> SearchArgs:
    """Construct the `SearchArgs` object for the specified search on the dataset."""
    path_config = clp_package.path_config
    args = SearchArgs(
        script_path=path_config.search_path,
        config=clp_package.temp_config_file_path,
        wildcard_query=wildcard_query,
    )

    if clp_package.clp_config.package.storage_engine == StorageEngine.CLP_S:
        args.dataset = dataset.metadata.dataset_name

    match search_type:
        case ClpPackageSearchType.BASIC:
            pass
        case ClpPackageSearchType.FILE_PATH:
            args.file_path = dataset.logs_path / dataset.metadata.single_match_file
        case ClpPackageSearchType.IGNORE_CASE:
            args.ignore_case = True
        case ClpPackageSearchType.COUNT_RESULTS:
            args.count = True
        case ClpPackageSearchType.COUNT_BY_TIME:
            args.count_by_time = DEFAULT_COUNT_BY_TIME_INTERVAL
        case ClpPackageSearchType.TIME_RANGE:
            args.begin_ts = dataset.metadata.begin_ts
            args.end_ts = dataset.metadata.end_ts
        case _:
            pytest.fail(f"Unsupported search type for CLP package: '{search_type}'")

    return args


def verify_search_action(
    action: ClpAction,
    search_type: ClpPackageSearchType,
    original_dataset: SampleDataset,
) -> ClpVerificationResult:
    """
    Verifies the search action.

    :param action:
    :param search_type:
    :param original_dataset:
    :return: A `ClpVerificationResult` indicating the success or failure of the verification.
    """
    logger.info(
        "Verifying '%s' search on the '%s' dataset.",
        search_type.name,
        original_dataset.dataset_name,
    )

    returncode_result = action.verify_returncode()
    if not returncode_result:
        return returncode_result

    args = action.args
    assert isinstance(args, SearchArgs)

    # Construct and run grep command.
    grep_action = NonClpAction(
        cmd=_construct_grep_verification_cmd(args, search_type, original_dataset)
    )
    grep_action.check_returncode(dependent_action=action)

    # Compare grep result with search result.
    formatted_grep_result = _format_grep_result_for_search_type(
        grep_action.completed_proc.stdout, search_type
    )
    formatted_search_result = _format_search_result_for_search_type(
        action.completed_proc.stdout, search_type
    )
    if formatted_grep_result == formatted_search_result:
        return action.pass_verification()

    return action.fail_verification(
        f"Search verification failure: mismatch between formatted search result"
        f" '{formatted_search_result}' and formatted grep result '{formatted_grep_result}'.",
        supporting_action=grep_action,
    )


def _construct_grep_verification_cmd(
    args: SearchArgs,
    search_type: ClpPackageSearchType,
    original_dataset: SampleDataset,
) -> list[str]:
    grep_cmd_options = _get_grep_options_from_search_type(search_type)
    path_for_grep = args.file_path or original_dataset.logs_path
    return [
        get_binary_path("grep"),
        *grep_cmd_options,
        args.wildcard_query,
        str(path_for_grep),
    ]


def _get_grep_options_from_search_type(search_type: ClpPackageSearchType) -> list[str]:
    grep_cmd_options: list[str] = [
        "--recursive",
        "--no-filename",
        "--color=never",
    ]

    match search_type:
        case (
            ClpPackageSearchType.BASIC
            | ClpPackageSearchType.FILE_PATH
            | ClpPackageSearchType.COUNT_RESULTS
            | ClpPackageSearchType.COUNT_BY_TIME
            | ClpPackageSearchType.TIME_RANGE
        ):
            return grep_cmd_options
        case ClpPackageSearchType.IGNORE_CASE:
            grep_cmd_options.append("--ignore-case")
            return grep_cmd_options
        case _:
            pytest.fail(
                f"Search type '{search_type.name}' not configured for grep command construction."
            )


def _format_grep_result_for_search_type(grep_result: str, search_type: ClpPackageSearchType) -> str:
    match search_type:
        case (
            ClpPackageSearchType.BASIC
            | ClpPackageSearchType.FILE_PATH
            | ClpPackageSearchType.IGNORE_CASE
            | ClpPackageSearchType.TIME_RANGE
        ):
            return grep_result
        case ClpPackageSearchType.COUNT_RESULTS | ClpPackageSearchType.COUNT_BY_TIME:
            return str(len(grep_result.splitlines())) + "\n"
        case _:
            pytest.fail(
                f"Search type '{search_type.name}' not configured for grep result formatting."
            )


def _format_search_result_for_search_type(
    search_result: str, search_type: ClpPackageSearchType
) -> str:
    match search_type:
        case (
            ClpPackageSearchType.BASIC
            | ClpPackageSearchType.FILE_PATH
            | ClpPackageSearchType.IGNORE_CASE
            | ClpPackageSearchType.TIME_RANGE
        ):
            return search_result
        case ClpPackageSearchType.COUNT_RESULTS | ClpPackageSearchType.COUNT_BY_TIME:
            match = re.search(r"count: (\d+)", search_result)
            if match:
                return match.group(1) + "\n"
            pytest.fail(f"The search result '{search_result}' wasn't in the correct format.")
        case _:
            pytest.fail(
                f"Search type '{search_type.name}' not configured for search result formatting."
            )

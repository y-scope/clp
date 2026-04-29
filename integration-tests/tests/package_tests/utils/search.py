"""Functions and classes to facilitate CLP package search."""

import logging
import re
from enum import auto, Enum
from pathlib import Path

import pytest
from clp_py_utils.clp_config import StorageEngine

from tests.package_tests.classes import ClpPackage
from tests.utils.classes import CmdArgs, ExternalAction, IntegrationTestDataset, VerificationResult
from tests.utils.logging_utils import format_action_failure_msg
from tests.utils.utils import get_binary_path

logger = logging.getLogger(__name__)


DEFAULT_COUNT_BY_TIME_INTERVAL = 10


class SearchArgs(CmdArgs):
    """Docstring."""

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
        """Docstring."""
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
    """An enumeration of the types of search we can perform with the CLP package."""

    BASIC = auto()
    FILE_PATH = auto()
    IGNORE_CASE = auto()
    COUNT_RESULTS = auto()
    COUNT_BY_TIME = auto()
    TIME_RANGE = auto()


def search_clp_package(
    clp_package: ClpPackage,
    dataset: IntegrationTestDataset,
    search_type: ClpPackageSearchType,
    wildcard_query: str,
) -> ExternalAction:
    """Docstring."""
    logger.info(
        "Performing '%s' search on the '%s' dataset.", search_type.name, dataset.dataset_name
    )

    args: SearchArgs = _construct_args(clp_package, dataset, search_type, wildcard_query)
    return ExternalAction(cmd=args.to_cmd(), args=args)


def _construct_args(
    clp_package: ClpPackage,
    dataset: IntegrationTestDataset,
    search_type: ClpPackageSearchType,
    wildcard_query: str,
) -> SearchArgs:
    """Docstring."""
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
            args.file_path = dataset.logs_path / dataset.metadata.file_names[0]
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
    action: ExternalAction,
    search_type: ClpPackageSearchType,
    original_dataset: IntegrationTestDataset,
) -> VerificationResult:
    """Docstring."""
    logger.info("Verifying search.")
    if action.completed_proc.returncode != 0:
        return VerificationResult.fail(
            format_action_failure_msg(
                "The 'search.sh' subprocess returned a non-zero exit code.", action
            )
        )

    args = action.args
    assert isinstance(args, SearchArgs)

    # Construct and run grep command.
    grep_action = ExternalAction(
        cmd=_construct_grep_verification_cmd(args, search_type, original_dataset)
    )

    if grep_action.completed_proc.returncode != 0:
        pytest.fail(
            "During search action verification, internal grep command returned a non-zero exit"
            f" code. Subprocess log: {grep_action.log_file_path}"
        )

    # Compare grep result with search result.
    formatted_grep_result = _format_grep_result_for_search_type(
        grep_action.completed_proc.stdout, search_type
    )
    formatted_search_result = _format_search_result_for_search_type(
        action.completed_proc.stdout, search_type
    )
    if formatted_grep_result == formatted_search_result:
        return VerificationResult.ok()

    return VerificationResult.fail(
        format_action_failure_msg(
            f"Search verification failure: mismatch between formatted search result"
            f" '{formatted_search_result}' and formatted grep result"
            f" '{formatted_grep_result}'.",
            action,
            grep_action,
        )
    )


def _construct_grep_verification_cmd(
    args: SearchArgs,
    search_type: ClpPackageSearchType,
    original_dataset: IntegrationTestDataset,
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

"""Search verification helpers specific to the clp-text package."""

import logging
import re

import pytest

from tests.package_tests.utils.search import parse_timestamp_to_epoch_ms, SearchArgs
from tests.utils.classes import (
    ClpAction,
    ClpVerificationResult,
    EpochMsTimestampFormat,
    NonClpAction,
    SampleDataset,
    TimestampFormat,
)
from tests.utils.utils import get_binary_path

logger = logging.getLogger(__name__)


def verify_search_clp_text(
    action: ClpAction,
    dataset: SampleDataset,
) -> ClpVerificationResult:
    """
    Verifies that the search action performed on the `clp-text` package returns the expected
    results. Verification is performed as follows:

    1. A grep command written with respect to the original search arguments is run on the dataset.
    2. The filtered grep lines are compared against the original search action's output. For a
       count-style search (`--count`), the number of matched lines is compared against the reported
       count; otherwise the set of matched lines is compared against the set of output lines.

    :param action:
    :param dataset:
    :return: A `ClpVerificationResult` indicating the success or failure of the verification.
    """
    args = action.args
    if not isinstance(args, SearchArgs):
        err_msg = "Verification expects a 'SearchArgs' action."
        raise TypeError(err_msg)

    grep_action = NonClpAction(cmd=_construct_grep_verification_cmd(args, dataset))
    grep_action.check_returncode(dependent_action=action)
    grep_lines = grep_action.completed_proc.stdout.splitlines()

    # Filter the found entries if a time-range filter was used in the original query.
    if args.begin_ts is not None or args.end_ts is not None:
        grep_lines = _filter_lines_by_time_range(
            grep_lines, dataset.metadata.timestamp_format, args.begin_ts, args.end_ts
        )

    search_output = action.completed_proc.stdout
    if args.is_count_query:
        expected_count = str(len(grep_lines)) + "\n"
        actual_count = _extract_count_from_search_output(search_output)

        if expected_count == actual_count:
            return action.pass_verification()

        return action.fail_verification(
            "Search verification failure: mismatch between search count and expected count.\n"
            f"Expected count: '{expected_count}'\n"
            f"Actual count: '{actual_count}'",
            supporting_action=grep_action,
        )

    formatted_grep_result = "\n".join(sorted(grep_lines))
    formatted_search_result = "\n".join(sorted(search_output.splitlines()))
    if formatted_grep_result == formatted_search_result:
        return action.pass_verification()

    return action.fail_verification(
        f"Search verification failure: mismatch between formatted search result and formatted grep"
        f" result.\n"
        f"Formatted search result: '{formatted_search_result}'\n"
        f"Formatted grep result: '{formatted_grep_result}'",
        supporting_action=grep_action,
    )


def _construct_grep_verification_cmd(
    args: SearchArgs,
    dataset: SampleDataset,
) -> list[str]:
    """
    Constructs a grep command that reproduces the search described by `args` on `dataset`'s original
    logs.

    :param args:
    :param dataset:
    :return: The grep command as a list of arguments.
    """
    grep_cmd_options: list[str] = [
        "--recursive",
        "--no-filename",
        "--color=never",
    ]

    if args.ignore_case:
        grep_cmd_options.append("--ignore-case")

    path_for_grep = args.file_path or dataset.logs_path
    return [
        get_binary_path("grep"),
        *grep_cmd_options,
        args.query,
        str(path_for_grep),
    ]


def _filter_lines_by_time_range(
    lines: list[str],
    timestamp_format: TimestampFormat | None,
    begin_ts: int | None,
    end_ts: int | None,
) -> list[str]:
    """
    Filters `lines` down to those whose leading timestamp falls within the inclusive
    `[begin_ts, end_ts]` range. Each line must begin with a whitespace-delimited timestamp token
    interpreted according to `timestamp_format`.

    :param lines:
    :param timestamp_format:
    :param begin_ts:
    :param end_ts:
    :return: The lines that fall within the time range.
    """
    if timestamp_format is None:
        pytest.fail(
            "clp-text time-range search verification requires the original dataset's metadata to"
            " define a 'timestamp_format'."
        )

    filtered_entries: list[str] = []
    for line in lines:
        if not line.strip():
            continue
        timestamp = _parse_leading_timestamp_ms(line, timestamp_format)
        if (begin_ts is None or begin_ts <= timestamp) and (end_ts is None or timestamp <= end_ts):
            filtered_entries.append(line)
    return filtered_entries


def _parse_leading_timestamp_ms(line: str, timestamp_format: TimestampFormat) -> int:
    """
    Parses the leading whitespace-delimited timestamp token of `line` according to
    `timestamp_format`.

    :param line:
    :param timestamp_format:
    :return: The parsed timestamp as an integer number of milliseconds since the UNIX epoch.
    """
    token = line.split(maxsplit=1)[0]
    if isinstance(timestamp_format, EpochMsTimestampFormat):
        return int(token)

    try:
        return parse_timestamp_to_epoch_ms(token, timestamp_format.pattern)
    except ValueError:
        pytest.fail(
            f"Failed to parse timestamp token '{token}' from line '{line}' using pattern"
            f" '{timestamp_format.pattern}'."
        )


def _extract_count_from_search_output(search_output: str) -> str:
    """
    Extracts the count reported by a count-style search.

    :param search_output:
    :return: The reported count, followed by a newline.
    """
    match = re.search(r"count: (\d+)", search_output)
    if match:
        return match.group(1) + "\n"
    pytest.fail(f"The search result '{search_output}' wasn't in the correct format.")

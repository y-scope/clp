"""Search verification helpers specific to the clp-json package."""

import json
import logging
import re
from typing import Any

import pytest

from tests.package_tests.classes import ClpPackage
from tests.package_tests.utils.search import parse_timestamp_to_epoch_ms, SearchArgs
from tests.utils.classes import (
    ClpAction,
    ClpVerificationResult,
    EpochMsTimestampFormat,
    SampleDataset,
    SampleDatasetMetadata,
    TimestampFormat,
)

logger = logging.getLogger(__name__)


WILDCARD_MULTIMATCH_CHAR = "*"
KV_DELIMITER_COLON = ":"
MIN_QUOTED_LENGTH = 2


def verify_search_clp_json(
    action: ClpAction,
    clp_package: ClpPackage,
    dataset: SampleDataset,
) -> ClpVerificationResult:
    """
    Verifies that the search action performed on the `clp-json` package returns the expected
    results. Verification is performed as follows:

    1. The query string from the search action is parsed into a key-value pair.
    2. The dataset's log entries are gathered into a list of dicts: read directly from the original
       JSONL files for a structured dataset, or recovered by re-running the search with a match-all
       query for an unstructured dataset (whose original logs aren't JSON).
    3. The list of dicts is searched for entries that match the key-value pair.
    4. The found entries are compared against the original search action's output. For a count-style
       search (`--count`), the number of found entries is compared against the reported count;
       otherwise the set of found entries is compared against the set of output logs.

    :param action:
    :param clp_package:
    :param dataset:
    :return: A `ClpVerificationResult` indicating the success or failure of the verification.
    """
    # Verify that the action's arguments are of the expected type.
    args = action.args
    if not isinstance(args, SearchArgs):
        err_msg = "Verification expects a 'SearchArgs' action."
        raise TypeError(err_msg)

    # Construct KV-pair from query.
    kv_pair = _construct_kv_pair_from_query(args.query)

    # Convert original logs to list[dict] objects for processing.
    metadata: SampleDatasetMetadata = dataset.metadata
    if metadata.unstructured:
        # Get the structured version of the unstructured data. Could use log-converter directly.
        supporting_search_args: SearchArgs = SearchArgs(
            script_path=clp_package.path_config.search_path,
            config=clp_package.temp_config_file_path,
            query="*",
            dataset=metadata.dataset_name,
        )
        supporting_search_action = ClpAction.from_args(supporting_search_args)
        search_result = supporting_search_action.verify_returncode()
        if not search_result:
            return action.fail_verification(
                "During search action verification, supporting call to 'search.sh' returned a"
                " non-zero exit code.",
                supporting_action=supporting_search_action,
            )

        supporting_output = supporting_search_action.completed_proc.stdout
        all_logs = [json.loads(line) for line in supporting_output.splitlines() if line.strip()]
    else:
        all_logs = _load_all_jsonl_logs_from_dataset(dataset)

    # Find entries that match the kv_pair from the original query.
    found_entries: list[dict[str, Any]] = _search_all_logs_for_kv_pair(
        all_logs, kv_pair, args, metadata
    )

    # Compare the found entries with the search action output.
    search_output = action.completed_proc.stdout
    if args.is_count_query:
        expected_count = str(len(found_entries)) + "\n"
        actual_count = _extract_count_from_search_output(search_output)

        if expected_count == actual_count:
            return action.pass_verification()

        return action.fail_verification(
            "Search verification failure: mismatch between search count and expected count.\n"
            f"Expected count: '{expected_count}'\nActual count: '{actual_count}'",
        )

    search_output_list = [json.loads(line) for line in search_output.splitlines() if line.strip()]
    if _normalize_entries(found_entries) == _normalize_entries(search_output_list):
        return action.pass_verification()

    return action.fail_verification(
        "Search verification failure: mismatch between search output and expected output.\n"
        f"Expected output: '{found_entries}'\nActual output: '{search_output_list}'",
    )


def _construct_kv_pair_from_query(query: str) -> tuple[str, Any]:
    """
    Constructs a key-value pair from a query string. The query is split on the first
    `KV_DELIMITER_COLON` that is not inside a quoted group (`"..."` or `'...'`).

    :param query:
    :return: `(KEY, VALUE)` if `KV_DELIMITER_COLON` is present, else
        `(WILDCARD_MULTIMATCH_CHAR, VALUE)`.
    """
    colon_index = _find_unquoted_colon(query)
    if colon_index is None:
        return WILDCARD_MULTIMATCH_CHAR, _strip_surrounding_quotes(query.strip())
    key = _strip_surrounding_quotes(query[:colon_index].strip())
    value = _strip_surrounding_quotes(query[colon_index + 1 :].strip())
    return key, value


def _find_unquoted_colon(query: str) -> int | None:
    """
    Finds the first colon in `query` that is not inside a quoted group (`"..."` or `'...'`).

    :param query:
    :return: The index of the first unquoted colon, or `None` if there is none.
    """
    open_quote: str | None = None
    for index, char in enumerate(query):
        if open_quote is not None:
            if char == open_quote:
                open_quote = None
        elif char in ('"', "'"):
            open_quote = char
        elif char == KV_DELIMITER_COLON:
            return index
    return None


def _strip_surrounding_quotes(text: str) -> str:
    """
    Removes a single pair of matching surrounding quotes (`"` or `'`) from `text`, if present.

    :param text:
    :return: The modified text string.
    """
    if len(text) >= MIN_QUOTED_LENGTH and text[0] == text[-1] and text[0] in ('"', "'"):
        return text[1:-1]
    return text


def _load_all_jsonl_logs_from_dataset(dataset: SampleDataset) -> list[dict[str, Any]]:
    """
    Loads every JSONL log from a dataset into a single list of dicts.

    :param dataset:
    :return: A list of dicts, each of which represents a JSONL record.
    """
    records: list[dict[str, Any]] = []
    for path in dataset.metadata.file_names:
        absolute_path = dataset.logs_path / path
        with absolute_path.open("r", encoding="utf-8") as f:
            for line in f:
                stripped = line.strip()
                if not stripped:
                    continue
                records.append(json.loads(stripped))
    return records


def _search_all_logs_for_kv_pair(
    all_logs: list[dict[str, Any]],
    kv_pair: tuple[str, Any],
    args: SearchArgs,
    metadata: SampleDatasetMetadata,
) -> list[dict[str, Any]]:
    """
    Searches `all_logs` for entries that contain `kv_pair`, then applies the same time-range filter
    the search applied (`--begin-time`/`--end-time`). Both the key and the value in `kv_pair` may
    contain `WILDCARD_MULTIMATCH_CHAR`, which matches zero or more characters. Following the
    semantics of the package's `--ignore-case` flag, `args.ignore_case` makes value matching (but
    not key matching) case-insensitive. When `args` specifies a time range, `metadata`'s timestamp
    key and format are used to read and interpret each entry's timestamp.

    :param all_logs:
    :param kv_pair:
    :param args:
    :param metadata:
    :return: A list of dicts that contain the `kv_pair` and fall within the search's time range.
    """
    key, value = kv_pair
    key_regex = _convert_wildcard_to_regex(str(key), ignore_case=False)
    value_regex = _convert_wildcard_to_regex(str(value), ignore_case=args.ignore_case)
    found_entries = [
        entry
        for entry in all_logs
        if any(
            key_regex.fullmatch(str(entry_key)) and value_regex.fullmatch(str(entry_value))
            for entry_key, entry_value in entry.items()
        )
    ]

    # Filter the found entries if a time-range filter was used in the original query.
    if args.begin_ts is not None or args.end_ts is not None:
        timestamp_key = metadata.timestamp_key
        timestamp_format = metadata.timestamp_format
        if timestamp_key is None or timestamp_format is None:
            pytest.fail(
                "Time-range search verification requires a structured dataset whose metadata"
                " defines a top-level 'timestamp_key' and a 'timestamp_format'."
            )

        found_entries = _filter_entries_by_time_range(
            found_entries,
            timestamp_key,
            timestamp_format,
            args.begin_ts,
            args.end_ts,
        )

    return found_entries


def _convert_wildcard_to_regex(pattern: str, ignore_case: bool) -> re.Pattern[str]:
    """
    Compiles a wildcard `pattern` into a regex in which each `WILDCARD_MULTIMATCH_CHAR` matches
    zero or more characters and all other characters are matched literally. When `ignore_case` is
    set, the compiled regex matches case-insensitively.

    :param pattern:
    :param ignore_case:
    :return: A compiled regex pattern.
    """
    regex = ".*".join(re.escape(segment) for segment in pattern.split(WILDCARD_MULTIMATCH_CHAR))
    flags = re.DOTALL
    if ignore_case:
        flags |= re.IGNORECASE
    return re.compile(regex, flags)


def _filter_entries_by_time_range(
    entries: list[dict[str, Any]],
    timestamp_key: str,
    timestamp_format: TimestampFormat,
    begin_ts: int | None,
    end_ts: int | None,
) -> list[dict[str, Any]]:
    """
    Filters `entries` down to those whose timestamp falls within the inclusive `[begin_ts, end_ts]`
    range, mirroring the package's `--begin-time`/`--end-time` semantics. `begin_ts` and `end_ts`
    are epoch-millisecond bounds, either of which may be `None` to leave that end unbounded. Each
    entry's timestamp is read from the top-level `timestamp_key` and interpreted according to
    `timestamp_format`.

    :param entries:
    :param timestamp_key:
    :param timestamp_format:
    :param begin_ts:
    :param end_ts:
    :return: The entries that fall within the time range.
    """
    filtered_entries: list[dict[str, Any]] = []
    for entry in entries:
        if timestamp_key not in entry:
            pytest.fail(f"Log entry '{entry}' is missing the timestamp key '{timestamp_key}'.")
        timestamp = _resolve_timestamp_to_ms(entry[timestamp_key], timestamp_key, timestamp_format)
        if (begin_ts is None or begin_ts <= timestamp) and (end_ts is None or timestamp <= end_ts):
            filtered_entries.append(entry)
    return filtered_entries


def _resolve_timestamp_to_ms(
    raw_timestamp: Any,
    timestamp_key: str,
    timestamp_format: TimestampFormat,
) -> int:
    """
    Resolves a log entry's raw timestamp value into epoch milliseconds according to
    `timestamp_format`. For an `epoch_ms` format, `raw_timestamp` must be an integer number of
    milliseconds; for a `strptime` format, it must be a string parseable by the format's pattern.
    `timestamp_key` is only used to contextualize failure messages.

    :param raw_timestamp:
    :param timestamp_key:
    :param timestamp_format:
    :return: The timestamp as an integer number of milliseconds since the UNIX epoch.
    """
    if isinstance(timestamp_format, EpochMsTimestampFormat):
        if not isinstance(raw_timestamp, int):
            pytest.fail(
                f"Log entry timestamp '{raw_timestamp}' under key '{timestamp_key}' is not an"
                " integer, which the 'epoch_ms' timestamp kind requires."
            )
        return raw_timestamp

    if not isinstance(raw_timestamp, str):
        pytest.fail(
            f"Log entry timestamp '{raw_timestamp}' under key '{timestamp_key}' is not a string,"
            " which the 'strptime' timestamp kind requires."
        )

    try:
        return parse_timestamp_to_epoch_ms(raw_timestamp, timestamp_format.pattern)
    except ValueError:
        pytest.fail(
            f"Failed to parse log entry timestamp '{raw_timestamp}' under key '{timestamp_key}'"
            f" using pattern '{timestamp_format.pattern}'."
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


def _normalize_entries(entries: list[dict[str, Any]]) -> list[str]:
    """
    Serializes each entry into a canonical JSON string and returns the sorted list.

    :param entries:
    :return: A sorted list of canonical JSON strings.
    """
    return sorted(json.dumps(entry, sort_keys=True) for entry in entries)

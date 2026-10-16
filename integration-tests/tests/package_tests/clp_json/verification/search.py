"""Search verification helpers specific to the clp-json package."""

import json
import logging
import re
from typing import Any

import pytest

from tests.package_tests.classes import ClpPackage
from tests.package_tests.utils.search import SearchArgs
from tests.utils.classes import (
    ClpAction,
    ClpVerificationResult,
    SampleDataset,
    SampleDatasetMetadata,
)
from tests.utils.timestamps import TimestampFormat
from tests.utils.utils import strip_surrounding_quotes

logger = logging.getLogger(__name__)


ESCAPE_CHAR = "\\"
KV_DELIMITER = ":"
WILDCARD_SINGLEMATCH_CHAR = "?"
WILDCARD_MULTIMATCH_CHAR = "*"
UNSTRUCTURED_TIMESTAMP_KEY = "timestamp"


def verify_search_clp_json(
    action: ClpAction,
    clp_package: ClpPackage,
    dataset: SampleDataset,
) -> ClpVerificationResult:
    """
    Verifies that the search action performed on the `clp-json` package returns the expected
    results. Verification is performed as follows:

    1. The query string from the search action is parsed into a key-value pair. Note that the query
       string must not be a composite query, i.e., queries with `AND`, `OR`, or other logical
       operators are not supported for verification.
    2. The dataset's log entries are loaded into a list of dicts for processing.
    3. The list of dicts is searched for entries that match or contain the key-value pair.
    4. The found entries are compared against the original search action's output. For a count-style
       search (`--count`), the number of found entries is compared against the reported count;
       otherwise the set of found entries is compared against the set of output logs.

    :param action:
    :param clp_package:
    :param dataset:
    :return: A `ClpVerificationResult` indicating the success or failure of the verification.
    """
    args = action.args
    if not isinstance(args, SearchArgs):
        err_msg = "Verification expects a 'SearchArgs' action."
        raise TypeError(err_msg)

    kv_pair = _construct_kv_pair_from_query(args.query)

    metadata: SampleDatasetMetadata = dataset.metadata
    if metadata.unstructured:
        # Use search.sh to get the structured version of the unstructured data.
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
        all_logs = _load_all_json_logs_from_dataset(dataset)

    found_entries: list[dict[str, Any]] = _search_all_logs_for_kv_pair(
        all_logs, kv_pair, args, metadata
    )

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
    Constructs a key-value pair from a query string. The query is split on the first `KV_DELIMITER`
    that is not inside a quoted group (`"..."` or `'...'`).

    :param query:
    :return: `(KEY, VALUE)` if `KV_DELIMITER` is present, else `(WILDCARD_MULTIMATCH_CHAR, VALUE)`.
    """
    kv_delimiter_index = _find_unquoted_kv_delimiter(query)
    if kv_delimiter_index is None:
        return WILDCARD_MULTIMATCH_CHAR, strip_surrounding_quotes(query.strip())
    key = strip_surrounding_quotes(query[:kv_delimiter_index].strip())
    value = strip_surrounding_quotes(query[kv_delimiter_index + 1 :].strip())
    return key, value


def _find_unquoted_kv_delimiter(query: str) -> int | None:
    """
    Finds the first unescaped instance of `KV_DELIMITER` in `query` that is not inside a quoted
    group (`"..."` or `'...'`).

    :param query:
    :return: Index of the first unescaped unquoted `KV_DELIMITER`, or `None` if there is none.
    """
    open_quote: str | None = None
    escaped = False
    for index, char in enumerate(query):
        if escaped:
            escaped = False
        elif char == ESCAPE_CHAR:
            escaped = True
        elif open_quote is not None:
            if char == open_quote:
                open_quote = None
        elif char in ('"', "'"):
            open_quote = char
        elif char == KV_DELIMITER:
            return index
    return None


def _load_all_json_logs_from_dataset(dataset: SampleDataset) -> list[dict[str, Any]]:
    """
    Loads every line-delimited JSON log from a dataset into a single list of dicts.

    :param dataset:
    :return: A list of dicts, each of which represents a line-delimited JSON record.
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
    Searches `all_logs` for entries that contain `kv_pair`. The search respects the various
    constraints posed by the content of `kv_pair`, `args` and `metadata`.

    :param all_logs:
    :param kv_pair:
    :param args:
    :param metadata:
    :return: A list of dicts that contain the `kv_pair` when searched w.r.t. the constraints.
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
        timestamp_format = metadata.timestamp_format
        if timestamp_format is None:
            pytest.fail(
                "clp-json time-range search verification requires the original dataset's metadata"
                " to define a 'timestamp_format'."
            )
        timestamp_key = _resolve_timestamp_key(metadata)

        found_entries = _filter_entries_by_time_range(
            found_entries,
            timestamp_key,
            timestamp_format,
            args.begin_ts,
            args.end_ts,
        )

    return found_entries


def _resolve_timestamp_key(metadata: SampleDatasetMetadata) -> str:
    """
    Resolves the top-level key under which each entry's timestamp is stored in the clp-json
    structured representation. Unstructured logs are assigned the `UNSTRUCTURED_TIMESTAMP_KEY` key
    when compressed with clp-json, whereas structured datasets carry the `timestamp_key` declared
    in their metadata.

    :param metadata:
    :return: The top-level timestamp key.
    """
    if metadata.unstructured:
        return UNSTRUCTURED_TIMESTAMP_KEY

    if metadata.timestamp_key is None:
        pytest.fail(
            "clp-json time-range search verification of a structured dataset requires the original"
            " dataset's metadata to define a top-level 'timestamp_key'."
        )

    return metadata.timestamp_key


def _convert_wildcard_to_regex(pattern: str, ignore_case: bool) -> re.Pattern[str]:
    """
    Compiles a wildcard `pattern` into a regex in which each `WILDCARD_MULTIMATCH_CHAR` matches
    zero or more characters and all other characters are matched literally. When `ignore_case` is
    set, the compiled regex matches case-insensitively.

    :param pattern:
    :param ignore_case:
    :return: A compiled regex pattern.
    """
    regex_parts: list[str] = []
    index = 0
    while index < len(pattern):
        char = pattern[index]
        if char == ESCAPE_CHAR and index + 1 < len(pattern):
            regex_parts.append(re.escape(pattern[index + 1]))
            index += 2
        elif char == WILDCARD_MULTIMATCH_CHAR:
            regex_parts.append(".*")
            index += 1
        elif char == WILDCARD_SINGLEMATCH_CHAR:
            regex_parts.append(".")
            index += 1
        else:
            regex_parts.append(re.escape(char))
            index += 1

    flags = re.DOTALL
    if ignore_case:
        flags |= re.IGNORECASE
    return re.compile("".join(regex_parts), flags)


def _filter_entries_by_time_range(
    entries: list[dict[str, Any]],
    timestamp_key: str,
    timestamp_format: TimestampFormat,
    begin_ts: int | None,
    end_ts: int | None,
) -> list[dict[str, Any]]:
    """
    Filters `entries` down to those whose timestamp falls within the inclusive `[begin_ts, end_ts]`
    range. Each entry's timestamp is read from the top-level `timestamp_key` and interpreted
    according to `timestamp_format`.

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
        raw_timestamp = entry[timestamp_key]
        try:
            timestamp = timestamp_format.to_epoch_ms(raw_timestamp)
        except (TypeError, ValueError) as e:
            pytest.fail(
                f"Failed to resolve log entry timestamp '{raw_timestamp}' under key"
                f" '{timestamp_key}': {e}"
            )
        if (begin_ts is None or begin_ts <= timestamp) and (end_ts is None or timestamp <= end_ts):
            filtered_entries.append(entry)
    return filtered_entries


def _extract_count_from_search_output(search_output: str) -> str:
    """
    Extracts the total count reported by a count-style search. A count-by-time search reports a
    separate count for each time bucket, so all per-bucket counts are summed.

    :param search_output:
    :return: The total reported count, followed by a newline.
    """
    matches = re.findall(r"count: (\d+)", search_output)
    if matches:
        return str(sum(int(count) for count in matches)) + "\n"
    pytest.fail(f"The search result '{search_output}' wasn't in the correct format.")


def _normalize_entries(entries: list[dict[str, Any]]) -> list[str]:
    """
    Serializes each entry into a canonical JSON string and returns the sorted list.

    :param entries:
    :return: A sorted list of canonical JSON strings.
    """
    return sorted(json.dumps(entry, sort_keys=True) for entry in entries)

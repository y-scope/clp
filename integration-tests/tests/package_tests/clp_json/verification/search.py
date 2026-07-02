"""Search verification helpers specific to the clp-json package."""

import json
import logging
import re
from typing import Any

from tests.package_tests.classes import ClpPackage
from tests.package_tests.utils.search import SearchArgs
from tests.utils.classes import (
    ClpAction,
    ClpVerificationResult,
    SampleDataset,
    SampleDatasetMetadata,
)

logger = logging.getLogger(__name__)


WILDCARD_MULTIMATCH_CHAR = "*"
KV_DELIMITER_COLON = ":"
MIN_QUOTED_LENGTH = 2


def verify_search_clp_json(
    action: ClpAction, clp_package: ClpPackage, dataset: SampleDataset
) -> ClpVerificationResult:
    """Docstring."""
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
        log_list = [json.loads(line) for line in supporting_output.splitlines() if line.strip()]
    else:
        log_list = _load_all_jsonl_logs_from_dataset(dataset)

    # Find entries that match the kv_pair from the original query.
    found_entries: list[dict[str, Any]] = _search_log_list_for_kv_pair(log_list, kv_pair)

    # Compare the found entries with the structurized search action output.
    search_output = action.completed_proc.stdout
    search_output_list = [json.loads(line) for line in search_output.splitlines() if line.strip()]
    if _normalize_entries(found_entries) == _normalize_entries(search_output_list):
        return action.pass_verification()

    return action.fail_verification(
        "Search verification failure: mismatch between search output and expected output."
        f" Expected output: '{found_entries}', actual output: '{search_output_list}'",
    )


def _normalize_entries(entries: list[dict[str, Any]]) -> list[str]:
    """
    Serializes each entry into a canonical JSON string and returns the sorted list, so that two
    result sets can be compared independent of ordering while tolerating unhashable dict entries.
    """
    return sorted(json.dumps(entry, sort_keys=True) for entry in entries)


def _construct_kv_pair_from_query(query: str) -> tuple[str, Any]:
    """
    Constructs a key-value pair from a query string. The query is split on the first
    `KV_DELIMITER_COLON` that is not inside a quoted group (`"..."` or `'...'`).

    :param query: The query string to parse.
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
    """Returns the index of the first colon not in a quoted group, or None if there is none."""
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
    """Removes a single pair of matching surrounding quotes (`"` or `'`) from `text`, if present."""
    if len(text) >= MIN_QUOTED_LENGTH and text[0] == text[-1] and text[0] in ('"', "'"):
        return text[1:-1]
    return text


def _load_all_jsonl_logs_from_dataset(dataset: SampleDataset) -> list[dict[str, Any]]:
    """Load every JSONL record from a dataset into a single list of dicts."""
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


def _search_log_list_for_kv_pair(
    log_list: list[dict[str, Any]], kv_pair: tuple[str, Any]
) -> list[dict[str, Any]]:
    """
    Searches a list of log entries for those that match a given key-value pair. Both the key
    and value may contain `WILDCARD_MULTIMATCH_CHAR`, which matches zero or more characters.
    """
    key, value = kv_pair
    key_regex = _convert_wildcard_to_regex(str(key))
    value_regex = _convert_wildcard_to_regex(str(value))
    return [
        entry
        for entry in log_list
        if any(
            key_regex.fullmatch(str(entry_key)) and value_regex.fullmatch(str(entry_value))
            for entry_key, entry_value in entry.items()
        )
    ]


def _convert_wildcard_to_regex(pattern: str) -> re.Pattern[str]:
    """
    Compiles a wildcard `pattern` into a regex in which each `WILDCARD_MULTIMATCH_CHAR` matches
    zero or more characters and all other characters are matched literally.
    """
    regex = ".*".join(re.escape(segment) for segment in pattern.split(WILDCARD_MULTIMATCH_CHAR))
    return re.compile(regex, re.DOTALL)

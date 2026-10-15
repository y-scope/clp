"""
Integration tests verifying the CLP+ binary performs round-trip compression and extraction, search
over structurized log messages, projection output structure, and value encoding correctness.
"""

import json
import os
from collections.abc import Iterator, Sequence
from pathlib import Path
from typing import Any, Final, NamedTuple

import pytest

from tests.utils.classes import (
    ClpAction,
    IntegrationTestPathConfig,
    SampleDataset,
)
from tests.utils.config import (
    ClpCorePathConfig,
    CompressionTestPathConfig,
)
from tests.utils.fs_validation import is_json_file_structurally_equal

pytestmark = pytest.mark.core

LeafValueT = Sequence[float | int | str]
MatchT = dict[str, LeafValueT | str]
DecomposedT = dict[str, list[MatchT] | LeafValueT | str]

_DECOMPOSITION_SUPPORTED: Final[bool] = os.environ.get("CLP_BUILD_CLPP_DECOMPOSITION") == "1"
_requires_decomposition = pytest.mark.skipif(
    not _DECOMPOSITION_SUPPORTED,
    reason="Requires clp-s built with -DCLP_BUILD_CLPP_DECOMPOSITION=ON",
)


def _project_matches(matches: list[MatchT], fields: list[str]) -> list[MatchT]:
    """Returns a copy of each expected occurrence entry containing only the given fields."""
    return [{field: match[field] for field in fields} for match in matches]


class _InflightMatch:
    """
    Expected values for the log event matched by `query`: an event with two `key_value` parent
    rule occurrences that share the same shape (`%key_value.key%: %key_value.int_value%`).

    This is dependant on both the elasticsearch dataset and the heuristic parsing spec. If either
    change these constants will need updating.
    """

    query: Final[str] = (
        'message.key_value.key: "estimate"'
        " and message.key_value.int_value > 90"
        " and message.key_value.int_value < 100"
    )
    result_count: Final[int] = 1
    text: Final[str] = (
        "[inflight_requests] Adding [46b][<http_request>] to used bytes"
        " [new used: [46b], limit: 33285996544 [31gb], estimate: 92 [92b]]"
    )
    kv_shape: Final[str] = "%key_value.key%: %key_value.int_value%"
    shape: Final[str] = (
        "[inflight_requests] Adding [%hex_fallback%][<http_request>] to used bytes"
        f" [new used: [%hex_fallback%], {kv_shape} [%has_number%],"
        f" {kv_shape} [%hex_fallback%]]"
    )
    limit_match: Final[MatchT] = {
        "text": "limit: 33285996544",
        "shape": kv_shape,
        "key": ["limit"],
        "int_value": [33285996544],
    }
    estimate_match: Final[MatchT] = {
        "text": "estimate: 92",
        "shape": kv_shape,
        "key": ["estimate"],
        "int_value": [92],
    }

    matches: Final[list[MatchT]] = [limit_match, estimate_match]

    decomposed_message: Final[DecomposedT] = {
        "shape": shape,
        "has_number": ["31gb"],
        "hex_fallback": ["46b", "46b", "92b"],
        "key_value": _project_matches(matches, ["key", "int_value"]),
    }


class _DiskMatch:
    """
    A log shape with specific values that appears 4 times in the data.

    This is dependant on both the elasticsearch dataset and the heuristic parsing spec. If either
    change these constants will need updating.
    """

    result_count: Final[int] = 4
    float_rule_name: Final[str] = "float_fallback"
    float_value: Final[float] = 26.057864583579928
    int_rule_name: Final[str] = "int_fallback"
    int_value: Final[int] = 769174777856
    decomposed_message: Final[DecomposedT] = {
        "shape": "node [%has_number%] has %float_fallback%%% free disk (%int_fallback% bytes)",
        "has_number": ["iKPGCkp9RVOKXOj20uOt4g"],
        float_rule_name: [float_value],
        int_rule_name: [int_value],
    }


class _MixedKvMatch:
    """
    A log event with two `key_value` parent rule occurrences of different shapes: an `int_value`
    variant followed by a `str_value` variant.

    This is dependant on both the `json_clpp_synthetic` dataset and the heuristic parsing spec. If
    either change these constants will need updating.
    """

    query: Final[str] = 'message.key_value.key: "clppmulti_a"'
    result_count: Final[int] = 1
    text: Final[str] = "clppmulti clppmulti_a: 1234 clppmulti_b=replicated"
    int_shape: Final[str] = "%key_value.key%: %key_value.int_value%"
    str_shape: Final[str] = "%key_value.key%=%key_value.str_value%"
    int_match: Final[MatchT] = {
        "text": "clppmulti_a: 1234",
        "shape": int_shape,
        "key": ["clppmulti_a"],
        "int_value": [1234],
    }
    str_match: Final[MatchT] = {
        "text": "clppmulti_b=replicated",
        "shape": str_shape,
        "key": ["clppmulti_b"],
        "str_value": ["replicated"],
    }

    matches: Final[list[MatchT]] = [int_match, str_match]

    # Decompose-mode emission (shape + leaves, without reconstructed text).
    decomposed_matches: Final[list[MatchT]] = [
        {"shape": int_shape, "key": ["clppmulti_a"], "int_value": [1234]},
        {"shape": str_shape, "key": ["clppmulti_b"], "str_value": ["replicated"]},
    ]


def _make_test_paths(
    integration_test_path_config: IntegrationTestPathConfig,
    dataset: SampleDataset,
) -> CompressionTestPathConfig:
    """
    Creates per-module test paths for the given dataset.

    :param integration_test_path_config:
    :param dataset:
    :return: The created path configuration.
    """
    return CompressionTestPathConfig(
        test_name=f"clpp-{dataset.dataset_name}",
        logs_source_path=dataset.logs_path,
        integration_test_path_config=integration_test_path_config,
    )


@pytest.fixture(scope="module", name="clpp_test_paths")
def clpp_test_paths_fixture(
    integration_test_path_config: IntegrationTestPathConfig,
    json_multifile: SampleDataset,
) -> Iterator[CompressionTestPathConfig]:
    """
    Provides test paths for clp+ tests on the `json_multifile` dataset and cleans up outputs on
    teardown.

    :param integration_test_path_config:
    :param json_multifile:
    :return: An iterator yielding the path configuration.
    """
    test_paths = _make_test_paths(integration_test_path_config, json_multifile)
    test_paths.clear_test_outputs()
    yield test_paths
    test_paths.clear_test_outputs()


@pytest.fixture(scope="module", name="clpp_synthetic_test_paths")
def clpp_synthetic_test_paths_fixture(
    integration_test_path_config: IntegrationTestPathConfig,
    json_clpp_synthetic: SampleDataset,
) -> Iterator[CompressionTestPathConfig]:
    """
    Provides test paths for clp+ tests on the `json_clpp_synthetic` dataset and cleans up outputs
    on teardown.

    :param integration_test_path_config:
    :param json_clpp_synthetic:
    :return: An iterator yielding the path configuration.
    """
    test_paths = _make_test_paths(integration_test_path_config, json_clpp_synthetic)
    test_paths.clear_test_outputs()
    yield test_paths
    test_paths.clear_test_outputs()


def _compress_dataset_with_clpp(
    clp_core_path_config: ClpCorePathConfig,
    integration_test_path_config: IntegrationTestPathConfig,
    dataset: SampleDataset,
    compression_dir: Path,
) -> None:
    """
    Compresses the given dataset with clp+ into `compression_dir`.

    :param clp_core_path_config:
    :param integration_test_path_config: Used to resolve the parsing spec path relative to the
    repository root.
    :param dataset:
    :param compression_dir:
    """
    parsing_spec_path = (
        integration_test_path_config.integration_tests_project_root.parent
        / "components/package-template/src/etc/parsing-spec.template.txt"
    )
    if not parsing_spec_path.is_file():
        pytest.fail(f"Parsing specification not found: '{parsing_spec_path}'")

    timestamp_key = dataset.metadata.timestamp_key
    if timestamp_key is None:
        pytest.fail(f"The `{dataset.dataset_name}` dataset must define a `timestamp_key`.")

    compression_cmd = [
        str(clp_core_path_config.clp_s_binary_path),
        "c",
        "--flavour",
        "clpp",
        "--timestamp-key",
        timestamp_key,
        f"--parsing-specification={parsing_spec_path}",
        str(compression_dir),
        str(dataset.logs_path),
    ]
    compression_action = ClpAction.from_cmd(compression_cmd)
    compression_result = compression_action.verify_returncode()
    assert compression_result, compression_result.failure_message


@pytest.fixture(scope="module", name="clpp_archive")
def clpp_archive_fixture(
    clp_core_path_config: ClpCorePathConfig,
    integration_test_path_config: IntegrationTestPathConfig,
    json_multifile: SampleDataset,
    clpp_test_paths: CompressionTestPathConfig,
) -> Path:
    """
    Compresses the `json_multifile` dataset with clp+; the module-scoped archive is shared by the
    tests that request this fixture.

    :param clp_core_path_config:
    :param integration_test_path_config:
    :param json_multifile:
    :param clpp_test_paths:
    :return: The path to the compressed archive directory.
    """
    _compress_dataset_with_clpp(
        clp_core_path_config,
        integration_test_path_config,
        json_multifile,
        clpp_test_paths.compression_dir,
    )
    return clpp_test_paths.compression_dir


@pytest.fixture(scope="module", name="clpp_synthetic_archive")
def clpp_synthetic_archive_fixture(
    clp_core_path_config: ClpCorePathConfig,
    integration_test_path_config: IntegrationTestPathConfig,
    json_clpp_synthetic: SampleDataset,
    clpp_synthetic_test_paths: CompressionTestPathConfig,
) -> Path:
    """
    Compresses the `json_clpp_synthetic` dataset with clp+; the module-scoped archive is shared by
    the tests that request this fixture.

    :param clp_core_path_config:
    :param integration_test_path_config:
    :param json_clpp_synthetic:
    :param clpp_synthetic_test_paths:
    :return: The path to the compressed archive directory.
    """
    _compress_dataset_with_clpp(
        clp_core_path_config,
        integration_test_path_config,
        json_clpp_synthetic,
        clpp_synthetic_test_paths.compression_dir,
    )
    return clpp_synthetic_test_paths.compression_dir


@pytest.mark.clpp
def test_clpp_roundtrip(
    clp_core_path_config: ClpCorePathConfig,
    json_multifile: SampleDataset,
    clpp_test_paths: CompressionTestPathConfig,
    clpp_archive: Path,
) -> None:
    """
    Validates that clp+ compression and extraction are lossless (allowing key reordering).

    :param clp_core_path_config:
    :param json_multifile:
    :param clpp_test_paths:
    :param clpp_archive:
    """
    decompression_cmd = [
        str(clp_core_path_config.clp_s_binary_path),
        "x",
        "--ordered",
        str(clpp_archive),
        str(clpp_test_paths.decompression_dir),
    ]
    decompression_action = ClpAction.from_cmd(decompression_cmd)
    decompression_result = decompression_action.verify_returncode()
    assert decompression_result, decompression_result.failure_message

    extracted_paths = list(clpp_test_paths.decompression_dir.glob("*.jsonl"))
    assert 1 == len(extracted_paths)
    extracted_path = extracted_paths[0]

    consolidated_input_path = clpp_test_paths.decompression_dir / "clpp-consolidated-input.jsonl"
    with consolidated_input_path.open("w", encoding="utf-8") as consolidated_input_file:
        for file_name in json_multifile.metadata.file_names:
            content = (json_multifile.logs_path / file_name).read_text(encoding="utf-8")
            consolidated_input_file.write(content)

    assert is_json_file_structurally_equal(consolidated_input_path, extracted_path), (
        f"Mismatch between clp+ input {consolidated_input_path} and output {extracted_path}."
    )


_DURATION_EVENT_TEXT: Final[str] = (
    "recovery completed from [shard_store], took [13.4s]\n"
    "    index    : files           [0] with total_size [0b], took[9.5s]\n"
    "             : recovered_files [0] with total_size [0b]\n"
    "             : reusing_files   [0] with total_size [0b]\n"
    "    verify_index    : took [0s], check_index [0s]\n"
    "    translog : number_of_operations [0], took [3.7s]"
)


@pytest.mark.clpp
@pytest.mark.parametrize(
    ("query", "expected_text"),
    [
        pytest.param(_InflightMatch.query, _InflightMatch.text, id="leaf_rule"),
        pytest.param(
            'message.duration: "13.4s"',
            _DURATION_EVENT_TEXT,
            id="parent_rule",
            marks=[pytest.mark.clpp_decomposition, _requires_decomposition],
        ),
        pytest.param(
            'message: "*Adjusted breaker*"',
            "[inflight_requests] Adjusted breaker by [0] bytes, now [0]",
            id="full_message",
            marks=[pytest.mark.clpp_decomposition, _requires_decomposition],
        ),
    ],
)
def test_clpp_search_single_message(
    clp_core_path_config: ClpCorePathConfig,
    clpp_archive: Path,
    query: str,
    expected_text: str,
) -> None:
    """
    Validates the search result of a query that matches exactly one log event.

    :param clp_core_path_config:
    :param clpp_archive:
    :param query: The query expected to match exactly one event.
    :param expected_text: The expected reconstructed text of the event's message.
    """
    results = _search(clp_core_path_config, clpp_archive, query)
    assert len(results) == 1, f"Query '{query}' expected 1 result, got {len(results)}."

    actual_message = results[0]["message"]["text"]
    assert actual_message == expected_text, (
        f"Query '{query}' returned unexpected message.\n"
        f"  Expected: {expected_text!r}\n"
        f"  Actual:   {actual_message!r}"
    )


@pytest.mark.clpp
@pytest.mark.clpp_decomposition
@_requires_decomposition
def test_clpp_search_no_match(
    clp_core_path_config: ClpCorePathConfig,
    clpp_archive: Path,
) -> None:
    """
    Validates that a wildcard query with no matching tokens returns zero results.

    :param clp_core_path_config:
    :param clpp_archive:
    """
    query = 'message: "*ZZZ_NO_SUCH_TOKEN*"'

    results = _search(clp_core_path_config, clpp_archive, query)
    assert len(results) == 0, f"Query '{query}' expected 0 results, got {len(results)}."


class _NegationCase(NamedTuple):
    """
    A query's negation results are the results of `scope_query` minus the events matching the
    positive query.
    """

    query: str  # The positive query being negated.
    scope_query: str  # The query scoping the negation (the events the column belongs to).
    matched_count: int  # The number of events the positive query matches.
    excluded_text: str | None = None  # A message text the negated query must not return.


@pytest.mark.clpp
@pytest.mark.clpp_decomposition
@_requires_decomposition
@pytest.mark.parametrize(
    "case",
    [
        pytest.param(
            _NegationCase(
                'message: "*Adjusted breaker*"',
                "message: *",
                1,
                "[inflight_requests] Adjusted breaker by [0] bytes, now [0]",
            ),
            id="full_message",
        ),
        pytest.param(
            _NegationCase('message.duration: "13.4s"', "message.duration: *", 1),
            id="parent_rule",
        ),
        pytest.param(
            _NegationCase('message: "*ZZZ_NO_SUCH_TOKEN*"', "message: *", 0),
            id="no_match",
        ),
    ],
)
def test_clpp_search_negated(
    clp_core_path_config: ClpCorePathConfig,
    clpp_archive: Path,
    case: _NegationCase,
) -> None:
    """
    Validates that a negated query is scoped to the events the column belongs to, excluding exactly
    the events matching the positive query.

    :param clp_core_path_config:
    :param clpp_archive:
    :param case:
    """
    total_count = len(_search(clp_core_path_config, clpp_archive, case.scope_query))
    results = _search(clp_core_path_config, clpp_archive, f"not {case.query}")
    assert len(results) == total_count - case.matched_count, (
        f"Negated query of '{case.query}' expected {total_count - case.matched_count} results,"
        f" got {len(results)}."
    )
    if case.excluded_text is None:
        return
    assert all(result["message"]["text"] != case.excluded_text for result in results), (
        f"Query 'not {case.query}' returned the excluded message {case.excluded_text!r}."
    )


@pytest.mark.clpp
@pytest.mark.clpp_decomposition
@_requires_decomposition
@pytest.mark.parametrize(
    ("query", "scope_query", "matches"),
    [
        pytest.param(
            'message: "*estimate: 9*"',
            "message: *",
            True,
            id="log_message_same_occurrence",
        ),
        pytest.param(
            'message: "*limit: 9*"',
            "message: *",
            False,
            id="log_message_crossed_occurrences",
        ),
        pytest.param(
            'message: "*estimate: 3*"',
            "message: *",
            False,
            id="log_message_crossed_reversed",
        ),
        pytest.param(
            'message.key_value: "estimate: 9*"',
            "message.key_value: *",
            True,
            id="parent_rule_same_occurrence",
        ),
        pytest.param(
            'message.key_value: "limit: 9*"',
            "message.key_value: *",
            False,
            id="parent_rule_crossed_occurrences",
        ),
    ],
)
def test_clpp_search_occurrence_precision(
    clp_core_path_config: ClpCorePathConfig,
    clpp_archive: Path,
    query: str,
    scope_query: str,
    matches: bool,
) -> None:
    """
    Validates that a decomposed query constrains the leaves of a single occurrence of a repeated
    parent rule rather than any occurrence, for both the query and its negation.

    :param clp_core_path_config:
    :param clpp_archive:
    :param query:
    :param scope_query: The query whose results scope the negation (i.e. the events the column
        belongs to).
    :param matches: Whether `query` should match the `_InflightMatch` event.
    """
    expected_count = 1 if matches else 0
    results = _search(clp_core_path_config, clpp_archive, query)
    assert len(results) == expected_count, (
        f"Query '{query}' expected {expected_count} results, got {len(results)}."
    )
    assert all(result["message"]["text"] == _InflightMatch.text for result in results), (
        f"Query '{query}' returned a non-matching message: {results}."
    )

    total_count = len(_search(clp_core_path_config, clpp_archive, scope_query))
    negated_query = f"not {query}"
    negated_results = _search(clp_core_path_config, clpp_archive, negated_query)
    assert len(negated_results) == total_count - expected_count, (
        f"Query '{negated_query}' expected {total_count - expected_count} results,"
        f" got {len(negated_results)}."
    )


@pytest.mark.clpp
@pytest.mark.parametrize(
    ("projection", "expected_message"),
    [
        pytest.param(
            None,
            {"text": _InflightMatch.text},
            id="return_all_columns",
        ),
        pytest.param(
            ["message"],
            {"text": _InflightMatch.text},
            id="text",
        ),
        pytest.param(
            ["shape(message)"],
            {"shape": _InflightMatch.shape},
            id="shape",
        ),
        pytest.param(
            ["decompose(message)"],
            _InflightMatch.decomposed_message,
            id="decompose",
        ),
        pytest.param(
            ["message", "shape(message)"],
            {"text": _InflightMatch.text, "shape": _InflightMatch.shape},
            id="message_shape",
        ),
        pytest.param(
            ["message", "decompose(message)"],
            {"text": _InflightMatch.text, **_InflightMatch.decomposed_message},
            id="message_decompose",
        ),
        pytest.param(
            ["message.key_value"],
            {"key_value": _project_matches(_InflightMatch.matches, ["text"])},
            id="parent_rule",
        ),
        pytest.param(
            ["message.key_value.key"],
            {"key_value": _project_matches(_InflightMatch.matches, ["key"])},
            id="leaf_only",
        ),
        pytest.param(
            ["message.key_value", "message.key_value.int_value"],
            {"key_value": _project_matches(_InflightMatch.matches, ["text", "int_value"])},
            id="combined",
        ),
        pytest.param(
            ["shape(message.key_value)"],
            {"key_value": _project_matches(_InflightMatch.matches, ["shape"])},
            id="shape_parent_rule",
        ),
        pytest.param(
            ["decompose(message.key_value)"],
            {"key_value": _project_matches(_InflightMatch.matches, ["shape", "key", "int_value"])},
            id="decompose_parent_rule",
        ),
        pytest.param(
            ["message.key_value", "decompose(message.key_value)"],
            {
                "key_value": _project_matches(
                    _InflightMatch.matches, ["text", "shape", "key", "int_value"]
                )
            },
            id="parent_rule_decompose",
        ),
        pytest.param(
            ["message", "decompose(message.key_value)"],
            {
                "text": _InflightMatch.text,
                "key_value": _project_matches(
                    _InflightMatch.matches, ["shape", "key", "int_value"]
                ),
            },
            id="message_decompose_parent_rule",
        ),
    ],
)
def test_clpp_projection(
    clp_core_path_config: ClpCorePathConfig,
    clpp_archive: Path,
    projection: list[str] | None,
    expected_message: Any,
) -> None:
    """
    Validates that all fundamental projection behaviors produce the expected JSON output structure.

    :param clp_core_path_config:
    :param clpp_archive:
    :param projection: Column specifiers to pass to `--projection`, or `None` for default behavior.
    :param expected_message: Expected JSON `message` object depending on the projection.
    """
    results = _search(clp_core_path_config, clpp_archive, _InflightMatch.query, projection)
    assert len(results) == 1, (
        f"Query '{_InflightMatch.query}' expected 1 result, got {len(results)}."
    )

    assert results[0].get("message") == expected_message, (
        f"Projection {projection} produced unexpected output.\n"
        f"  Expected: {expected_message!r}\n"
        f"  Actual:   {results[0].get('message')!r}"
    )


@pytest.mark.clpp
@pytest.mark.parametrize(
    "projection",
    [
        pytest.param(["shape(message.key_value.int_value)"], id="shape_on_leaf"),
        pytest.param(["decompose(message.key_value.int_value)"], id="decompose_on_leaf"),
    ],
)
def test_clpp_projection_error(
    clp_core_path_config: ClpCorePathConfig,
    clpp_archive: Path,
    projection: list[str],
) -> None:
    """
    Validates that `shape()` and `decompose()` applied to a leaf rule cause an error.

    :param clp_core_path_config:
    :param clpp_archive:
    :param projection: Column specifiers applying `shape()`/`decompose()` to a leaf column, passed
        via `--projection`.
    """
    search_cmd = _build_search_cmd(
        clp_core_path_config, clpp_archive, _InflightMatch.query, projection
    )
    action = ClpAction.from_cmd(search_cmd)
    assert 0 != action.completed_proc.returncode, (
        f"Projection {projection} should have failed but exited 0.\n"
        f"stdout: {action.completed_proc.stdout!r}\n"
        f"stderr: {action.completed_proc.stderr!r}"
    )
    assert "no LogMessage or ParentRule nodes match" in action.completed_proc.stderr, (
        f"Projection {projection} did not produce expected error.\n"
        f"stderr: {action.completed_proc.stderr!r}"
    )


@pytest.mark.clpp
@pytest.mark.parametrize(
    ("projection", "expected_message"),
    [
        pytest.param(
            ["message.key_value"],
            {"key_value": _project_matches(_MixedKvMatch.matches, ["text"])},
            id="parent_rule_text",
        ),
        pytest.param(
            ["shape(message.key_value)"],
            {"key_value": _project_matches(_MixedKvMatch.matches, ["shape"])},
            id="parent_rule_shape",
        ),
        pytest.param(
            ["decompose(message.key_value)"],
            {"key_value": _MixedKvMatch.decomposed_matches},
            id="parent_rule_decompose",
        ),
        pytest.param(
            ["message", "message.key_value"],
            {
                "text": _MixedKvMatch.text,
                "key_value": _project_matches(_MixedKvMatch.matches, ["text"]),
            },
            id="message_and_parent_rule_text",
        ),
    ],
)
def test_clpp_projection_repeated_parent_rule(
    clp_core_path_config: ClpCorePathConfig,
    clpp_synthetic_archive: Path,
    projection: list[str],
    expected_message: Any,
) -> None:
    """
    Validates that when one log event contains multiple occurrences of the same parent rule with
    different shapes, each occurrence is emitted with its own text, shape, and leaf values.

    :param clp_core_path_config:
    :param clpp_synthetic_archive:
    :param projection: Column specifiers to pass to `--projection`.
    :param expected_message: Expected JSON `message` object depending on the projection.
    """
    results = _search(clp_core_path_config, clpp_synthetic_archive, _MixedKvMatch.query, projection)
    assert len(results) == _MixedKvMatch.result_count, (
        f"Query '{_MixedKvMatch.query}' expected {_MixedKvMatch.result_count} results,"
        f" got {len(results)}."
    )

    assert results[0].get("message") == expected_message, (
        f"Projection {projection} produced unexpected output.\n"
        f"  Expected: {expected_message!r}\n"
        f"  Actual:   {results[0].get('message')!r}"
    )


@pytest.mark.clpp
@pytest.mark.parametrize(
    ("rule_name", "lower_bound", "upper_bound"),
    [
        pytest.param(
            _DiskMatch.int_rule_name,
            _DiskMatch.int_value - 1,
            _DiskMatch.int_value + 1,
            id="int",
        ),
        pytest.param(_DiskMatch.float_rule_name, 26.05, 26.06, id="float"),
    ],
)
def test_clpp_encoding_range_query(
    clp_core_path_config: ClpCorePathConfig,
    clpp_archive: Path,
    rule_name: str,
    lower_bound: float,
    upper_bound: float,
) -> None:
    """
    Validates that numeric values are encoded as numeric columns by using a range query (`>` and
    `<`) around the encoded value.

    :param clp_core_path_config:
    :param clpp_archive:
    :param rule_name: The leaf rule whose value column is queried.
    :param lower_bound: Exclusive lower bound of the range query.
    :param upper_bound: Exclusive upper bound of the range query.
    """
    query = f"message.{rule_name} > {lower_bound} and message.{rule_name} < {upper_bound}"
    results = _search(clp_core_path_config, clpp_archive, query)
    assert len(results) == _DiskMatch.result_count, (
        f"Query '{query}' expected {_DiskMatch.result_count} results, got {len(results)}."
    )


@pytest.mark.clpp
def test_clpp_encoding_decompose(
    clp_core_path_config: ClpCorePathConfig,
    clpp_archive: Path,
) -> None:
    """
    Validates that decomposed output contains native JSON numbers (not strings) for encoded
    values.

    :param clp_core_path_config:
    :param clpp_archive:
    """
    query = f"message.{_DiskMatch.int_rule_name}: {_DiskMatch.int_value}"
    results = _search(clp_core_path_config, clpp_archive, query, ["decompose(message)"])
    assert len(results) == _DiskMatch.result_count, (
        f"Query '{query}' expected {_DiskMatch.result_count} results, got {len(results)}."
    )

    for result in results:
        message = result.get("message")
        assert isinstance(message, dict), f"Expected a nested `message` object, got: {result}"
        assert message == _DiskMatch.decomposed_message, f"Unexpected message object: {message}"

        int_fallback = message["int_fallback"]
        assert isinstance(int_fallback, list), (
            f"Expected `int_fallback` to be a list, got {type(int_fallback)}: {message}"
        )
        assert all(isinstance(value, int) for value in int_fallback), (
            f"Expected `int_fallback` leaves to be native ints, got: {int_fallback}"
        )

        float_fallback = message["float_fallback"]
        assert isinstance(float_fallback, list), (
            f"Expected `float_fallback` to be a list, got {type(float_fallback)}: {message}"
        )
        assert all(isinstance(value, float) for value in float_fallback), (
            f"Expected `float_fallback` leaves to be native floats, got: {float_fallback}"
        )


def _build_search_cmd(
    clp_core_path_config: ClpCorePathConfig,
    archive_path: Path,
    query: str,
    projection: list[str] | None = None,
) -> list[str]:
    """
    Builds the `clp-s s` command for the given query and optional projection.

    :param clp_core_path_config:
    :param archive_path:
    :param query: KQL query string.
    :param projection: Optional column specifiers passed via `--projection`.
    :return: The constructed command.
    """
    search_cmd = [
        str(clp_core_path_config.clp_s_binary_path),
        "s",
        str(archive_path),
        query,
    ]
    if projection:
        search_cmd.append("--projection")
        search_cmd.extend(projection)
    return search_cmd


def _search(
    clp_core_path_config: ClpCorePathConfig,
    archive_path: Path,
    query: str,
    projection: list[str] | None = None,
) -> list[dict[str, Any]]:
    """
    Runs `clp-s s` and parses each output line as a JSON object. Any non-JSON line
    will result in a failure.

    :param clp_core_path_config:
    :param archive_path:
    :param query: KQL query string.
    :param projection: Optional column specifiers passed via `--projection`.
    :return: The parsed search results.
    """
    search_action = ClpAction.from_cmd(
        _build_search_cmd(clp_core_path_config, archive_path, query, projection)
    )
    search_result = search_action.verify_returncode()
    assert search_result, search_result.failure_message

    results: list[dict[str, Any]] = []
    for line in search_action.completed_proc.stdout.splitlines():
        if not line.startswith("{"):
            pytest.fail(
                f"Search output line is not a JSON object.\n"
                f"Query:     {query!r}\n"
                f"Archive:   {archive_path}\n"
                f"Projection: {projection!r}\n"
                f"Line:      {line!r}"
            )
        try:
            results.append(json.loads(line))
        except json.JSONDecodeError as e:
            pytest.fail(
                f"Failed to parse search output as JSON.\n"
                f"Query:     {query!r}\n"
                f"Archive:   {archive_path}\n"
                f"Projection: {projection!r}\n"
                f"Error:     {e}\n"
                f"Line:      {line!r}"
            )
    return results

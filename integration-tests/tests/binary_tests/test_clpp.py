"""
Integration tests verifying the CLP+ binary performs round-trip compression and extraction, search
over structurized log messages, projection output structure, and value encoding correctness.
"""

from collections.abc import Iterator, Sequence
from pathlib import Path
from typing import Any, Final, NamedTuple

import pytest

from tests.binary_tests.clp_s_utils import (
    assert_dataset_roundtrip,
    build_search_cmd,
    clpp_archive_for,
    ClppArchive,
    compress,
    search,
)
from tests.utils.classes import (
    ClpAction,
    IntegrationTestPathConfig,
    SampleDataset,
)
from tests.utils.config import ClpCorePathConfig

pytestmark = pytest.mark.core

# Marks `pytest.param` cases that need clp-s built with decomposition support.
_decomposed = pytest.mark.clpp_decomposition

LeafValueT = Sequence[float | int | str]
MatchT = dict[str, LeafValueT | str]
DecomposedT = dict[str, list[MatchT] | LeafValueT | str]


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


@pytest.fixture(scope="module", name="clpp_archive")
def clpp_archive_fixture(
    clp_core_path_config: ClpCorePathConfig,
    integration_test_path_config: IntegrationTestPathConfig,
    json_multifile: SampleDataset,
) -> Iterator[ClppArchive]:
    """
    Provides the `json_multifile` dataset compressed with clp+, shared by this module's tests.

    :param clp_core_path_config:
    :param integration_test_path_config:
    :param json_multifile:
    :return: An iterator yielding the compressed archive.
    """
    with clpp_archive_for(
        clp_core_path_config, integration_test_path_config, json_multifile
    ) as archive:
        yield archive


@pytest.fixture(scope="module", name="clpp_synthetic_archive")
def clpp_synthetic_archive_fixture(
    clp_core_path_config: ClpCorePathConfig,
    integration_test_path_config: IntegrationTestPathConfig,
    json_clpp_synthetic: SampleDataset,
) -> Iterator[ClppArchive]:
    """
    Provides the `json_clpp_synthetic` dataset compressed with clp+, shared by this module's tests.

    :param clp_core_path_config:
    :param integration_test_path_config:
    :param json_clpp_synthetic:
    :return: An iterator yielding the compressed archive.
    """
    with clpp_archive_for(
        clp_core_path_config, integration_test_path_config, json_clpp_synthetic
    ) as archive:
        yield archive


@pytest.mark.clpp
def test_clpp_roundtrip(
    clp_core_path_config: ClpCorePathConfig,
    clpp_archive: ClppArchive,
) -> None:
    """
    Validates that clp+ compression and extraction are lossless (allowing key reordering).

    :param clp_core_path_config:
    :param clpp_archive:
    """
    assert_dataset_roundtrip(clp_core_path_config, clpp_archive)


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
            marks=_decomposed,
        ),
        pytest.param(
            'message: "*Adjusted breaker*"',
            "[inflight_requests] Adjusted breaker by [0] bytes, now [0]",
            id="full_message",
            marks=_decomposed,
        ),
    ],
)
def test_clpp_search_single_message(
    clp_core_path_config: ClpCorePathConfig,
    clpp_archive: ClppArchive,
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
    results = search(clp_core_path_config, clpp_archive.path, query)
    assert len(results) == 1, f"Query '{query}' expected 1 result, got {len(results)}."

    actual_message = results[0]["message"]
    assert actual_message == expected_text, (
        f"Query '{query}' returned unexpected message.\n"
        f"  Expected: {expected_text!r}\n"
        f"  Actual:   {actual_message!r}"
    )


@pytest.mark.clpp
@pytest.mark.clpp_decomposition
def test_clpp_search_no_match(
    clp_core_path_config: ClpCorePathConfig,
    clpp_archive: ClppArchive,
) -> None:
    """
    Validates that a wildcard query with no matching tokens returns zero results.

    :param clp_core_path_config:
    :param clpp_archive:
    """
    query = 'message: "*ZZZ_NO_SUCH_TOKEN*"'

    results = search(clp_core_path_config, clpp_archive.path, query)
    assert len(results) == 0, f"Query '{query}' expected 0 results, got {len(results)}."


@pytest.mark.clpp
@pytest.mark.parametrize("query", ["message < abcdef", "NOT message < abcdef"])
def test_clpp_search_logically_false(
    clp_core_path_config: ClpCorePathConfig,
    clpp_archive: ClppArchive,
    query: str,
) -> None:
    """
    Validates that a query no column type can satisfy is rejected as logically false before schema
    matching, including on a StructurizedText column (which requires a string operand).

    :param clp_core_path_config:
    :param clpp_archive:
    :param query: A range query whose operand can't be a number or a string under the operation.
    """
    search_cmd = build_search_cmd(clp_core_path_config, clpp_archive.path, query)
    action = ClpAction.from_cmd(search_cmd)
    assert 0 != action.completed_proc.returncode, (
        f"Query '{query}' should have failed but exited 0.\n"
        f"stderr: {action.completed_proc.stderr!r}"
    )
    assert "is logically false" in action.completed_proc.stderr, (
        f"Query '{query}' did not produce expected error.\nstderr: {action.completed_proc.stderr!r}"
    )


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
    clpp_archive: ClppArchive,
    case: _NegationCase,
) -> None:
    """
    Validates that a negated query is scoped to the events the column belongs to, excluding exactly
    the events matching the positive query.

    :param clp_core_path_config:
    :param clpp_archive:
    :param case:
    """
    total_count = len(search(clp_core_path_config, clpp_archive.path, case.scope_query))
    results = search(clp_core_path_config, clpp_archive.path, f"not {case.query}")
    assert len(results) == total_count - case.matched_count, (
        f"Negated query of '{case.query}' expected {total_count - case.matched_count} results,"
        f" got {len(results)}."
    )
    if case.excluded_text is None:
        return
    assert all(result["message"] != case.excluded_text for result in results), (
        f"Query 'not {case.query}' returned the excluded message {case.excluded_text!r}."
    )


@pytest.mark.clpp
@pytest.mark.clpp_decomposition
@pytest.mark.parametrize(
    ("query", "scope_query", "matches"),
    [
        pytest.param(
            'message: "*estimate: 9*"',
            "message: *",
            True,
            id="whole_text_key_and_value_from_same_occurrence",
        ),
        pytest.param(
            'message: "*limit: 9*"',
            "message: *",
            False,
            id="whole_text_key_from_first_value_from_second",
        ),
        pytest.param(
            'message: "*estimate: 3*"',
            "message: *",
            False,
            id="whole_text_key_from_second_value_from_first",
        ),
        pytest.param(
            'message.key_value: "estimate: 9*"',
            "message.key_value: *",
            True,
            id="parent_rule_key_and_value_from_same_occurrence",
        ),
        pytest.param(
            'message.key_value: "limit: 9*"',
            "message.key_value: *",
            False,
            id="parent_rule_key_from_first_value_from_second",
        ),
    ],
)
def test_clpp_search_occurrence_precision(
    clp_core_path_config: ClpCorePathConfig,
    clpp_archive: ClppArchive,
    query: str,
    scope_query: str,
    matches: bool,
) -> None:
    """
    Validates that a decomposed query constrains the leaves of a single occurrence of a repeated
    parent rule rather than any occurrence, for both the query and its negation.

    The `_InflightMatch` event has two `key_value` occurrences: `limit: 33285996544` (first) and
    `estimate: 92` (second). A query only matches when its key and value come from the same
    occurrence.

    :param clp_core_path_config:
    :param clpp_archive:
    :param query:
    :param scope_query: The query whose results scope the negation (i.e. the events the column
        belongs to).
    :param matches: Whether `query` should match the `_InflightMatch` event.
    """
    expected_count = 1 if matches else 0
    results = search(clp_core_path_config, clpp_archive.path, query)
    assert len(results) == expected_count, (
        f"Query '{query}' expected {expected_count} results, got {len(results)}."
    )
    assert all(result["message"] == _InflightMatch.text for result in results), (
        f"Query '{query}' returned a non-matching message: {results}."
    )

    total_count = len(search(clp_core_path_config, clpp_archive.path, scope_query))
    negated_query = f"not {query}"
    negated_results = search(clp_core_path_config, clpp_archive.path, negated_query)
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
            _InflightMatch.text,
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
    clpp_archive: ClppArchive,
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
    results = search(clp_core_path_config, clpp_archive.path, _InflightMatch.query, projection)
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
    clpp_archive: ClppArchive,
    projection: list[str],
) -> None:
    """
    Validates that `shape()` and `decompose()` applied to a leaf rule cause an error.

    :param clp_core_path_config:
    :param clpp_archive:
    :param projection: Column specifiers applying `shape()`/`decompose()` to a leaf column, passed
        via `--projection`.
    """
    search_cmd = build_search_cmd(
        clp_core_path_config, clpp_archive.path, _InflightMatch.query, projection
    )
    action = ClpAction.from_cmd(search_cmd)
    assert 0 != action.completed_proc.returncode, (
        f"Projection {projection} should have failed but exited 0.\n"
        f"stdout: {action.completed_proc.stdout!r}\n"
        f"stderr: {action.completed_proc.stderr!r}"
    )
    assert "no StructurizedText or ParentRule nodes match" in action.completed_proc.stderr, (
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
    clpp_synthetic_archive: ClppArchive,
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
    results = search(
        clp_core_path_config, clpp_synthetic_archive.path, _MixedKvMatch.query, projection
    )
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
    clpp_archive: ClppArchive,
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
    results = search(clp_core_path_config, clpp_archive.path, query)
    assert len(results) == _DiskMatch.result_count, (
        f"Query '{query}' expected {_DiskMatch.result_count} results, got {len(results)}."
    )


@pytest.mark.clpp
def test_clpp_encoding_decompose(
    clp_core_path_config: ClpCorePathConfig,
    clpp_archive: ClppArchive,
) -> None:
    """
    Validates that decomposed output contains native JSON numbers (not strings) for encoded
    values.

    :param clp_core_path_config:
    :param clpp_archive:
    """
    query = f"message.{_DiskMatch.int_rule_name}: {_DiskMatch.int_value}"
    results = search(clp_core_path_config, clpp_archive.path, query, ["decompose(message)"])
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


class _ParentRuleScenario(NamedTuple):
    """
    A single-event clpp archive built from a custom parsing specification, and the `message` object
    each projection of it is expected to produce.
    """

    parsing_spec: str  # The parsing specification's contents.
    message: str  # The event's `message` field.
    expected_messages: list[tuple[str, Any]]  # (space-separated projections, expected `message`).


# Two `g` occurrences each contain one `inner` occurrence, with different shapes (`key=int_value`
# vs `key:str_value`): the second `inner` must get its own shape and text, not the first's.
_NESTED_INNER_SHAPES: Final[list[str]] = [
    "%g.inner.key%=%g.inner.int_value%",
    "%g.inner.key%:%g.inner.str_value%",
]
_NESTED_INNER_LEAVES: Final[list[MatchT]] = [
    {"key": ["first"], "int_value": [11]},
    {"key": ["second"], "str_value": ["xy"]},
]
_NESTED_SCENARIO: Final[_ParentRuleScenario] = _ParentRuleScenario(
    parsing_spec=(
        'delimiters: " \\t\\r\\n"\n'
        "\n"
        '!txt: "[\\w_-]+"\n'
        "\n"
        'g: "^\\[(?<inner>(?<key>(?<txt>))=(?<int_value>-?\\d+))\\]$"\n'
        'g: "^\\[(?<inner>(?<key>(?<txt>)):(?<str_value>(?<txt>)))\\]$"\n'
        "\n"
        'word: "^(?<word>(?<txt>))$"\n'
    ),
    message="wrap [first=11] [second:xy]",
    expected_messages=[
        (
            "decompose(message)",
            {
                "shape": "%word.word% " + " ".join(f"[{shape}]" for shape in _NESTED_INNER_SHAPES),
                "word": [{"word": ["wrap"]}],
                "g": [{"inner": [leaves]} for leaves in _NESTED_INNER_LEAVES],
            },
        ),
        (
            "message.g.inner",
            {"g": [{"inner": [{"text": text}]} for text in ["first=11", "second:xy"]]},
        ),
        (
            "shape(message.g.inner)",
            {"g": [{"inner": [{"shape": shape}]} for shape in _NESTED_INNER_SHAPES]},
        ),
        (
            "decompose(message.g)",
            {
                "g": [
                    {"shape": f"[{shape}]", "inner": [leaves]}
                    for shape, leaves in zip(
                        _NESTED_INNER_SHAPES, _NESTED_INNER_LEAVES, strict=True
                    )
                ]
            },
        ),
    ],
)

# The first `g` occurrence matches with no leaves (`v` is optional), so only the second appears in
# the decomposed output and it must get its own shape and text, not the first's.
_ZERO_LEAF_SCENARIO: Final[_ParentRuleScenario] = _ParentRuleScenario(
    parsing_spec='delimiters: " \\t\\r\\n"\n\ng: "^\\[(?<inner>x(?<v>\\d+)?)\\]$"\n',
    message="[x] [x5]",
    expected_messages=[
        (
            "decompose(message)",
            {"shape": "[x] [x%g.inner.v%]", "g": [{"inner": [{"v": [5]}]}]},
        ),
        ("message.g.inner", {"g": [{"inner": [{"text": "x5"}]}]}),
        ("shape(message.g.inner)", {"g": [{"inner": [{"shape": "x%g.inner.v%"}]}]}),
        ("message.g shape(message.g)", {"g": [{"text": "[x5]", "shape": "[x%g.inner.v%]"}]}),
    ],
)


@pytest.mark.clpp
@pytest.mark.parametrize(
    "scenario",
    [
        pytest.param(_NESTED_SCENARIO, id="nested_repeated_rule"),
        pytest.param(_ZERO_LEAF_SCENARIO, id="zero_leaf_occurrence"),
    ],
)
def test_clpp_projection_parent_rule_occurrence_shapes(
    clp_core_path_config: ClpCorePathConfig,
    tmp_path: Path,
    scenario: _ParentRuleScenario,
) -> None:
    """
    Validates that each parent rule occurrence's projected text and shape are its own, even when
    same-named occurrences have different shapes or an occurrence has no leaf matches.

    :param clp_core_path_config:
    :param tmp_path:
    :param scenario:
    """
    spec_path = tmp_path / "parsing-spec.txt"
    spec_path.write_text(scenario.parsing_spec)
    log_path = tmp_path / "event.log"
    log_path.write_text(f'{{"message":"{scenario.message}"}}\n')
    archive_path = tmp_path / "archives"
    compress(clp_core_path_config, archive_path, log_path, parsing_spec_path=spec_path)

    query = "message: *"
    for projection, expected_message in scenario.expected_messages:
        results = search(clp_core_path_config, archive_path, query, projection.split())
        assert len(results) == 1, f"Query '{query}' expected 1 result, got {len(results)}."
        assert results[0].get("message") == expected_message, (
            f"Projection '{projection}' produced unexpected output.\n"
            f"  Expected: {expected_message!r}\n"
            f"  Actual:   {results[0].get('message')!r}"
        )

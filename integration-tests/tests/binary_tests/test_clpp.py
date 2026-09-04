"""
Integration tests verifying the CLP+ binary performs round-trip compression and extraction, search
over structurized log messages, projection output structure, and value encoding correctness.
"""

import json
from collections.abc import Iterator, Sequence
from pathlib import Path
from typing import Any, Final

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


class _InflightMatch:
    """
    Structured result values for the provided query.
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
        "key_value": [{field: match[field] for field in ["key", "int_value"]} for match in matches],
    }

    @classmethod
    def project_matches(cls, fields: list[str]) -> list[MatchT] | LeafValueT | str:
        return [{field: match[field] for field in fields} for match in cls.matches]


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


@pytest.fixture(scope="module", name="clpp_test_paths")
def clpp_test_paths_fixture(
    integration_test_path_config: IntegrationTestPathConfig,
    json_multifile: SampleDataset,
) -> Iterator[CompressionTestPathConfig]:
    """
    Provides per-module test paths for clp+ tests and cleans up outputs on teardown.

    :param integration_test_path_config:
    :param json_multifile:
    :return: An iterator yielding the path configuration.
    """
    test_paths = CompressionTestPathConfig(
        test_name=f"clpp-{json_multifile.dataset_name}",
        logs_source_path=json_multifile.logs_path,
        integration_test_path_config=integration_test_path_config,
    )
    test_paths.clear_test_outputs()
    yield test_paths
    test_paths.clear_test_outputs()


@pytest.fixture(scope="module", name="clpp_archive")
def clpp_archive_fixture(
    clp_core_path_config: ClpCorePathConfig,
    integration_test_path_config: IntegrationTestPathConfig,
    json_multifile: SampleDataset,
    clpp_test_paths: CompressionTestPathConfig,
) -> Path:
    """
    Compresses the `json_multifile` dataset once with clp+, shared by all tests in this module.

    :param clp_core_path_config:
    :param integration_test_path_config: Used to resolve the parsing spec path relative to the
    repository root.
    :param json_multifile:
    :param clpp_test_paths:
    :return: The path to the compressed archive directory.
    """
    parsing_spec_path = (
        integration_test_path_config.integration_tests_project_root.parent
        / "components/package-template/src/etc/parsing-spec.template.txt"
    )
    if not parsing_spec_path.is_file():
        pytest.fail(f"Parsing specification not found: '{parsing_spec_path}'")

    timestamp_key = json_multifile.metadata.timestamp_key
    if timestamp_key is None:
        pytest.fail("The `json_multifile` dataset must define a `timestamp_key`.")

    compression_cmd = [
        str(clp_core_path_config.clp_s_binary_path),
        "c",
        "--experimental",
        "--timestamp-key",
        timestamp_key,
        f"--parsing-specification={parsing_spec_path}",
        str(clpp_test_paths.compression_dir),
        str(clpp_test_paths.logs_source_path),
    ]
    compression_action = ClpAction.from_cmd(compression_cmd)
    compression_result = compression_action.verify_returncode()
    assert compression_result, compression_result.failure_message

    return clpp_test_paths.compression_dir


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
        "--experimental",
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


@pytest.mark.clpp
def test_clpp_search_leaf(
    clp_core_path_config: ClpCorePathConfig,
    clpp_archive: Path,
) -> None:
    """
    Validates the search result of a leaf rule query.

    :param clp_core_path_config:
    :param clpp_archive:
    """
    query = _InflightMatch.query
    expected_count = 1
    expected_message = _InflightMatch.text

    results = _search(clp_core_path_config, clpp_archive, query)
    assert len(results) == expected_count, (
        f"Query '{query}' expected {expected_count} results, got {len(results)}."
    )

    actual_message = results[0]["message"]["text"]
    assert actual_message == expected_message, (
        f"Query '{query}' returned unexpected message.\n"
        f"  Expected: {expected_message!r}\n"
        f"  Actual:   {actual_message!r}"
    )


@pytest.mark.clpp
def test_clpp_search_parent_rule(
    clp_core_path_config: ClpCorePathConfig,
    clpp_archive: Path,
) -> None:
    """
    Validates the search result of a parent rule query.

    :param clp_core_path_config:
    :param clpp_archive:
    """
    query = 'message.duration: "13.4s"'
    expected_count = 1
    expected_message = (
        "recovery completed from [shard_store], took [13.4s]\n"
        "    index    : files           [0] with total_size [0b], took[9.5s]\n"
        "             : recovered_files [0] with total_size [0b]\n"
        "             : reusing_files   [0] with total_size [0b]\n"
        "    verify_index    : took [0s], check_index [0s]\n"
        "    translog : number_of_operations [0], took [3.7s]"
    )

    results = _search(clp_core_path_config, clpp_archive, query)
    assert len(results) == expected_count, (
        f"Query '{query}' expected {expected_count} results, got {len(results)}."
    )

    actual_message = results[0]["message"]["text"]
    assert actual_message == expected_message, (
        f"Query '{query}' returned unexpected message.\n"
        f"  Expected: {expected_message!r}\n"
        f"  Actual:   {actual_message!r}"
    )


@pytest.mark.clpp
def test_clpp_search_full_message(
    clp_core_path_config: ClpCorePathConfig,
    clpp_archive: Path,
) -> None:
    """
    Validates the search result of a wildcard full message query.

    :param clp_core_path_config:
    :param clpp_archive:
    """
    query = 'message: "*Adjusted breaker*"'
    expected_count = 1
    expected_message = "[inflight_requests] Adjusted breaker by [0] bytes, now [0]"

    results = _search(clp_core_path_config, clpp_archive, query)
    assert len(results) == expected_count, (
        f"Query '{query}' expected {expected_count} results, got {len(results)}."
    )

    actual_message = results[0]["message"]["text"]
    assert actual_message == expected_message, (
        f"Query '{query}' returned unexpected message.\n"
        f"  Expected: {expected_message!r}\n"
        f"  Actual:   {actual_message!r}"
    )


@pytest.mark.clpp
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
            {"key_value": _InflightMatch.project_matches(["text"])},
            id="parent_rule",
        ),
        pytest.param(
            ["message.key_value.key"],
            {"key_value": _InflightMatch.project_matches(["key"])},
            id="leaf_only",
        ),
        pytest.param(
            ["message.key_value", "message.key_value.int_value"],
            {"key_value": _InflightMatch.project_matches(["text", "int_value"])},
            id="combined",
        ),
        pytest.param(
            ["shape(message.key_value)"],
            {"key_value": _InflightMatch.project_matches(["shape"])},
            id="shape_parent_rule",
        ),
        pytest.param(
            ["decompose(message.key_value)"],
            {"key_value": _InflightMatch.project_matches(["shape", "key", "int_value"])},
            id="decompose_parent_rule",
        ),
        pytest.param(
            ["message.key_value", "decompose(message.key_value)"],
            {"key_value": _InflightMatch.project_matches(["text", "shape", "key", "int_value"])},
            id="parent_rule_decompose",
        ),
        pytest.param(
            ["message", "decompose(message.key_value)"],
            {
                "text": _InflightMatch.text,
                "key_value": _InflightMatch.project_matches(["shape", "key", "int_value"]),
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
    :param projection: Field name of a leaf rule match.
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
def test_clpp_encoding_int(
    clp_core_path_config: ClpCorePathConfig,
    clpp_archive: Path,
) -> None:
    """
    Validates that integer values are encoded as numeric columns by using > and < queries.

    :param clp_core_path_config:
    :param clpp_archive:
    """
    query = (
        f"message.{_DiskMatch.int_rule_name} > {_DiskMatch.int_value - 1}"
        f" and message.{_DiskMatch.int_rule_name} < {_DiskMatch.int_value + 1}"
    )
    results = _search(
        clp_core_path_config,
        clpp_archive,
        query,
    )
    assert len(results) == _DiskMatch.result_count, (
        f"Query '{query}' expected {_DiskMatch.result_count} results, got {len(results)}."
    )


@pytest.mark.clpp
def test_clpp_encoding_float(
    clp_core_path_config: ClpCorePathConfig,
    clpp_archive: Path,
) -> None:
    """
    Validates that float values are encoded as numeric columns by using a range query with
    fractional thresholds.

    :param clp_core_path_config:
    :param clpp_archive:
    """
    query = (
        f"message.{_DiskMatch.float_rule_name} > 26.05"
        f" and message.{_DiskMatch.float_rule_name} < 26.06"
    )
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
    Builds the `clp-s s --experimental` command for the given query and optional projection.

    :param clp_core_path_config:
    :param archive_path:
    :param query: KQL query string.
    :param projection: Optional column specifiers passed via `--projection`.
    :return: The constructed command.
    """
    search_cmd = [
        str(clp_core_path_config.clp_s_binary_path),
        "s",
        "--experimental",
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
    Runs `clp-s s --experimental` and parses each output line as a JSON object. Any non-JSON line
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

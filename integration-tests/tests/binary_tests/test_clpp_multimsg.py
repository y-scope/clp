"""
Integration tests verifying CLP+ handles multiple sibling unstructured text fields per event.

A log event may have several string fields that are each parsed as an unstructured log message,
which produces several sibling `LogMessage` nodes in a single schema. These tests validate that
search, decomposition, and projection remain correctly scoped to the queried field rather than
only honoring the first `LogMessage` node in the schema.
"""

import json
import os
from collections.abc import Iterator
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

_DECOMPOSITION_SUPPORTED: Final[bool] = os.environ.get("CLP_BUILD_CLPP_DECOMPOSITION") == "1"
_requires_decomposition = pytest.mark.skipif(
    not _DECOMPOSITION_SUPPORTED,
    reason="Requires clp-s built with -DCLP_BUILD_CLPP_DECOMPOSITION=ON",
)


class _MultiMessage:
    """
    Expected shape/value data for the `json_multimsg` dataset and the heuristic parsing spec.

    The dataset contains five events:
    0. message="alpha 111 beta",           details="gamma 222 delta"
    1. message="shared 333 text",          details="shared 444 text"
    2. message="count: 10 done",           details="count: 20 done"
    3. message="solo field 555"            (no `details` field)
    4. message="alpha 666 beta",           details="shared 777 text"
    """

    alpha_message_shape: Final[str] = "alpha %int_fallback% beta"
    gamma_details_shape: Final[str] = "gamma %int_fallback% delta"
    shared_shape: Final[str] = "shared %int_fallback% text"
    shared_details_events: Final[int] = 2


@pytest.fixture(scope="module", name="multimsg_test_paths")
def multimsg_test_paths_fixture(
    integration_test_path_config: IntegrationTestPathConfig,
    json_multimsg: SampleDataset,
) -> Iterator[CompressionTestPathConfig]:
    """
    Provides per-module test paths for the multi-message clp+ tests and cleans up on teardown.

    :param integration_test_path_config:
    :param json_multimsg:
    :return: An iterator yielding the path configuration.
    """
    test_paths = CompressionTestPathConfig(
        test_name=f"clpp-multimsg-{json_multimsg.dataset_name}",
        logs_source_path=json_multimsg.logs_path,
        integration_test_path_config=integration_test_path_config,
    )
    test_paths.clear_test_outputs()
    yield test_paths
    test_paths.clear_test_outputs()


@pytest.fixture(scope="module", name="multimsg_archive")
def multimsg_archive_fixture(
    clp_core_path_config: ClpCorePathConfig,
    integration_test_path_config: IntegrationTestPathConfig,
    json_multimsg: SampleDataset,
    multimsg_test_paths: CompressionTestPathConfig,
) -> Path:
    """
    Compresses the `json_multimsg` dataset once with clp+, shared by all tests in this module.

    :param clp_core_path_config:
    :param integration_test_path_config:
    :param json_multimsg:
    :param multimsg_test_paths:
    :return: The path to the compressed archive directory.
    """
    parsing_spec_path = (
        integration_test_path_config.integration_tests_project_root.parent
        / "components/package-template/src/etc/parsing-spec.template.txt"
    )
    if not parsing_spec_path.is_file():
        pytest.fail(f"Parsing specification not found: '{parsing_spec_path}'")

    timestamp_key = json_multimsg.metadata.timestamp_key
    if timestamp_key is None:
        pytest.fail("The `json_multimsg` dataset must define a `timestamp_key`.")

    compression_cmd = [
        str(clp_core_path_config.clp_s_binary_path),
        "c",
        "--experimental",
        "--timestamp-key",
        timestamp_key,
        f"--parsing-specification={parsing_spec_path}",
        str(multimsg_test_paths.compression_dir),
        str(multimsg_test_paths.logs_source_path),
    ]
    compression_action = ClpAction.from_cmd(compression_cmd)
    compression_result = compression_action.verify_returncode()
    assert compression_result, compression_result.failure_message

    return multimsg_test_paths.compression_dir


@pytest.mark.clpp
def test_multimsg_roundtrip(
    clp_core_path_config: ClpCorePathConfig,
    json_multimsg: SampleDataset,
    multimsg_test_paths: CompressionTestPathConfig,
    multimsg_archive: Path,
) -> None:
    """
    Validates that compressing and extracting an event with two unstructured fields is lossless.

    :param clp_core_path_config:
    :param json_multimsg:
    :param multimsg_test_paths:
    :param multimsg_archive:
    """
    decompression_cmd = [
        str(clp_core_path_config.clp_s_binary_path),
        "x",
        "--experimental",
        "--ordered",
        str(multimsg_archive),
        str(multimsg_test_paths.decompression_dir),
    ]
    decompression_action = ClpAction.from_cmd(decompression_cmd)
    decompression_result = decompression_action.verify_returncode()
    assert decompression_result, decompression_result.failure_message

    extracted_paths = list(multimsg_test_paths.decompression_dir.glob("*.jsonl"))
    assert 1 == len(extracted_paths), f"Expected one output file, got: {extracted_paths}"

    consolidated_input_path = (
        multimsg_test_paths.decompression_dir / "multimsg-consolidated-input.jsonl"
    )
    with consolidated_input_path.open("w", encoding="utf-8") as consolidated_input_file:
        for file_name in json_multimsg.metadata.file_names:
            content = (json_multimsg.logs_path / file_name).read_text(encoding="utf-8")
            consolidated_input_file.write(content)

    assert is_json_file_structurally_equal(consolidated_input_path, extracted_paths[0]), (
        f"Mismatch between clp+ input {consolidated_input_path} and output {extracted_paths[0]}."
    )


@pytest.mark.clpp
@pytest.mark.clpp_decomposition
@_requires_decomposition
@pytest.mark.parametrize(
    ("query", "expected_count"),
    [
        pytest.param('message: "*alpha*"', 2, id="message_alpha"),
        pytest.param('details: "*gamma*"', 1, id="details_gamma"),
        pytest.param('message: "*shared*"', 1, id="message_shared"),
        pytest.param('details: "*shared*"', 2, id="details_shared"),
        pytest.param("details: *", 4, id="details_exists"),
        pytest.param(
            'message: "*alpha*" and details: "*shared*"',
            1,
            id="cross_field_and",
        ),
    ],
)
def test_multimsg_search_scoping(
    clp_core_path_config: ClpCorePathConfig,
    multimsg_archive: Path,
    query: str,
    expected_count: int,
) -> None:
    """
    Validates that a value query is scoped to the queried field and does not leak across siblings.

    :param clp_core_path_config:
    :param multimsg_archive:
    :param query: KQL query string.
    :param expected_count: Expected number of matching events.
    """
    results = _search(clp_core_path_config, multimsg_archive, query)
    assert len(results) == expected_count, (
        f"Query '{query}' expected {expected_count} results, got {len(results)}."
    )


@pytest.mark.clpp
@pytest.mark.clpp_decomposition
@_requires_decomposition
def test_multimsg_shared_shape_disambiguation(
    clp_core_path_config: ClpCorePathConfig,
    multimsg_archive: Path,
) -> None:
    """
    Validates that two sibling fields with the same shape are attributed to the correct field.

    Events 1 and 4 both contain `message`/`details` with shape `shared %int_fallback% text`. A
    query on `message` must match only the message occurrences, and the same for `details`.
    """
    message_results = _search(clp_core_path_config, multimsg_archive, 'message: "*shared*"')
    assert 1 == len(message_results), f"Expected one `message` match, got {len(message_results)}."
    assert message_results[0]["message"]["text"] == "shared 333 text"

    details_results = _search(clp_core_path_config, multimsg_archive, 'details: "*shared*"')
    assert _MultiMessage.shared_details_events == len(details_results), (
        f"Expected {_MultiMessage.shared_details_events} `details` matches, "
        f"got {len(details_results)}."
    )
    assert {result["details"]["text"] for result in details_results} == {
        "shared 444 text",
        "shared 777 text",
    }


@pytest.mark.clpp
@pytest.mark.clpp_decomposition
@_requires_decomposition
def test_multimsg_same_parent_rule_name_both_fields(
    clp_core_path_config: ClpCorePathConfig,
    multimsg_archive: Path,
) -> None:
    """
    Validates that a parent rule present in both fields is not merged across the sibling scopes.

    Event 2 has `message="count: 10 done"` and `details="count: 20 done"`, both decomposed through
    the same `key_value` parent rule with the same rule path. The values must stay in their field,
    and each field must not match the other field's value.
    """
    message_results = _search(
        clp_core_path_config,
        multimsg_archive,
        "message.key_value.int_value: 10",
        ["decompose(message)"],
    )
    assert 1 == len(message_results), f"Expected one `message` match, got {len(message_results)}."
    assert message_results[0]["message"]["key_value"] == [{"key": ["count"], "int_value": [10]}]

    details_results = _search(
        clp_core_path_config,
        multimsg_archive,
        "details.key_value.int_value: 20",
        ["decompose(details)"],
    )
    assert 1 == len(details_results), f"Expected one `details` match, got {len(details_results)}."
    assert details_results[0]["details"]["key_value"] == [{"key": ["count"], "int_value": [20]}]

    assert 0 == len(
        _search(clp_core_path_config, multimsg_archive, "message.key_value.int_value: 20")
    ), "`message` unexpectedly matched the `details` value."
    assert 0 == len(
        _search(clp_core_path_config, multimsg_archive, "details.key_value.int_value: 10")
    ), "`details` unexpectedly matched the `message` value."


@pytest.mark.clpp
@pytest.mark.clpp_decomposition
@_requires_decomposition
def test_multimsg_negation_scoped_to_field(
    clp_core_path_config: ClpCorePathConfig,
    multimsg_archive: Path,
) -> None:
    """
    Validates that negating a value query excludes events based only on the queried field.

    :param clp_core_path_config:
    :param multimsg_archive:
    """
    total_details = len(_search(clp_core_path_config, multimsg_archive, "details: *"))
    query = 'not details: "*shared*"'

    results = _search(clp_core_path_config, multimsg_archive, query)
    assert len(results) == total_details - _MultiMessage.shared_details_events, (
        f"Query '{query}' expected {total_details - _MultiMessage.shared_details_events} results, "
        f"got {len(results)}."
    )
    assert all(result["details"]["text"] != "shared 444 text" for result in results)
    assert all(result["details"]["text"] != "shared 777 text" for result in results)


@pytest.mark.clpp
@pytest.mark.clpp_decomposition
@_requires_decomposition
@pytest.mark.parametrize(
    ("query", "expected_count"),
    [
        pytest.param('shape(message): "alpha*"', 2, id="shape_message_alpha"),
        pytest.param('shape(details): "shared*"', 2, id="shape_details_shared"),
    ],
)
def test_multimsg_shape_filter_scoping(
    clp_core_path_config: ClpCorePathConfig,
    multimsg_archive: Path,
    query: str,
    expected_count: int,
) -> None:
    """
    Validates that `shape()` filters are scoped to the queried sibling field.

    :param clp_core_path_config:
    :param multimsg_archive:
    :param query: KQL shape query string.
    :param expected_count: Expected number of matching events.
    """
    results = _search(clp_core_path_config, multimsg_archive, query)
    assert len(results) == expected_count, (
        f"Query '{query}' expected {expected_count} results, got {len(results)}."
    )


@pytest.mark.clpp
@pytest.mark.clpp_decomposition
@_requires_decomposition
@pytest.mark.parametrize(
    ("query", "projection", "expected"),
    [
        pytest.param(
            'message: "*alpha*" and details: "*gamma*"',
            ["decompose(message)", "decompose(details)"],
            {
                "message": {"shape": _MultiMessage.alpha_message_shape, "int_fallback": [111]},
                "details": {"shape": _MultiMessage.gamma_details_shape, "int_fallback": [222]},
            },
            id="decompose_both_fields",
        ),
        pytest.param(
            'details: "*777*"',
            ["message", "decompose(details)"],
            {
                "message": {"text": "alpha 666 beta"},
                "details": {"shape": _MultiMessage.shared_shape, "int_fallback": [777]},
            },
            id="text_plus_decompose_sibling",
        ),
    ],
)
def test_multimsg_projection_scoping(
    clp_core_path_config: ClpCorePathConfig,
    multimsg_archive: Path,
    query: str,
    projection: list[str],
    expected: dict[str, Any],
) -> None:
    """
    Validates that text/shape/decomposed projections nest under the correct sibling field.

    :param clp_core_path_config:
    :param multimsg_archive:
    :param query: KQL query string.
    :param projection: Column specifiers passed to `--projection`.
    :param expected: Expected `message`/`details` objects.
    """
    results = _search(clp_core_path_config, multimsg_archive, query, projection)
    assert 1 == len(results), f"Expected one result, got {len(results)}."
    assert results[0]["message"] == expected["message"], (
        f"Query '{query}' produced unexpected `message`.\n"
        f"  Expected: {expected['message']!r}\n"
        f"  Actual:   {results[0]['message']!r}"
    )
    assert results[0]["details"] == expected["details"], (
        f"Query '{query}' produced unexpected `details`.\n"
        f"  Expected: {expected['details']!r}\n"
        f"  Actual:   {results[0]['details']!r}"
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
    Runs `clp-s s --experimental` and parses each output line as a JSON object.

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

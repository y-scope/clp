"""
Integration tests verifying CLP+ handles multiple sibling unstructured text fields per event.

A log event may have several string fields that are each parsed as unstructured text, which
produces several sibling `StructurizedText` nodes in a single schema. These tests validate that
search, decomposition, and projection remain correctly scoped to the queried field rather than
only honoring the first `StructurizedText` node in the schema.
"""

from collections.abc import Iterator
from typing import Any, Final

import pytest

from tests.binary_tests.clp_s_utils import (
    assert_dataset_roundtrip,
    clpp_archive_for,
    ClppArchive,
    search,
)
from tests.utils.classes import (
    IntegrationTestPathConfig,
    SampleDataset,
)
from tests.utils.config import ClpCorePathConfig

pytestmark = pytest.mark.core

# Marks `pytest.param` cases that need clp-s built with decomposition support.
_decomposed = pytest.mark.clpp_decomposition


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


@pytest.fixture(scope="module", name="multimsg_archive")
def multimsg_archive_fixture(
    clp_core_path_config: ClpCorePathConfig,
    integration_test_path_config: IntegrationTestPathConfig,
    json_multimsg: SampleDataset,
) -> Iterator[ClppArchive]:
    """
    Provides the `json_multimsg` dataset compressed with clp+, shared by this module's tests.

    :param clp_core_path_config:
    :param integration_test_path_config:
    :param json_multimsg:
    :return: An iterator yielding the compressed archive.
    """
    with clpp_archive_for(
        clp_core_path_config, integration_test_path_config, json_multimsg
    ) as archive:
        yield archive


@pytest.mark.clpp
def test_multimsg_roundtrip(
    clp_core_path_config: ClpCorePathConfig,
    multimsg_archive: ClppArchive,
) -> None:
    """
    Validates that compressing and extracting an event with two unstructured fields is lossless.

    :param clp_core_path_config:
    :param multimsg_archive:
    """
    assert_dataset_roundtrip(clp_core_path_config, multimsg_archive)


@pytest.mark.clpp
@pytest.mark.parametrize(
    ("query", "expected_count"),
    [
        pytest.param('message: "*alpha*"', 2, id="message_alpha", marks=_decomposed),
        pytest.param('details: "*gamma*"', 1, id="details_gamma", marks=_decomposed),
        pytest.param('message: "*shared*"', 1, id="message_shared", marks=_decomposed),
        pytest.param('details: "*shared*"', 2, id="details_shared", marks=_decomposed),
        pytest.param("details: *", 4, id="details_exists"),
        pytest.param(
            'message: "*alpha*" and details: "*shared*"',
            1,
            id="cross_field_and",
            marks=_decomposed,
        ),
    ],
)
def test_multimsg_search_scoping(
    clp_core_path_config: ClpCorePathConfig,
    multimsg_archive: ClppArchive,
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
    results = search(clp_core_path_config, multimsg_archive.path, query)
    assert len(results) == expected_count, (
        f"Query '{query}' expected {expected_count} results, got {len(results)}."
    )


@pytest.mark.clpp
@pytest.mark.clpp_decomposition
def test_multimsg_shared_shape_disambiguation(
    clp_core_path_config: ClpCorePathConfig,
    multimsg_archive: ClppArchive,
) -> None:
    """
    Validates that two sibling fields with the same shape are attributed to the correct field.

    Events 1 and 4 both contain `message`/`details` with shape `shared %int_fallback% text`. A
    query on `message` must match only the message occurrences, and the same for `details`.
    """
    message_results = search(clp_core_path_config, multimsg_archive.path, 'message: "*shared*"')
    assert 1 == len(message_results), f"Expected one `message` match, got {len(message_results)}."
    assert message_results[0]["message"] == "shared 333 text"

    details_results = search(clp_core_path_config, multimsg_archive.path, 'details: "*shared*"')
    assert _MultiMessage.shared_details_events == len(details_results), (
        f"Expected {_MultiMessage.shared_details_events} `details` matches, "
        f"got {len(details_results)}."
    )
    assert {result["details"] for result in details_results} == {
        "shared 444 text",
        "shared 777 text",
    }


@pytest.mark.clpp
def test_multimsg_same_parent_rule_name_both_fields(
    clp_core_path_config: ClpCorePathConfig,
    multimsg_archive: ClppArchive,
) -> None:
    """
    Validates that a parent rule present in both fields is not merged across the sibling scopes.

    Event 2 has `message="count: 10 done"` and `details="count: 20 done"`, both decomposed through
    the same `key_value` parent rule with the same rule path. The values must stay in their field,
    and each field must not match the other field's value.
    """
    message_results = search(
        clp_core_path_config,
        multimsg_archive.path,
        "message.key_value.int_value: 10",
        ["decompose(message)"],
    )
    assert 1 == len(message_results), f"Expected one `message` match, got {len(message_results)}."
    assert message_results[0]["message"]["key_value"] == [{"key": ["count"], "int_value": [10]}]

    details_results = search(
        clp_core_path_config,
        multimsg_archive.path,
        "details.key_value.int_value: 20",
        ["decompose(details)"],
    )
    assert 1 == len(details_results), f"Expected one `details` match, got {len(details_results)}."
    assert details_results[0]["details"]["key_value"] == [{"key": ["count"], "int_value": [20]}]

    assert 0 == len(
        search(clp_core_path_config, multimsg_archive.path, "message.key_value.int_value: 20")
    ), "`message` unexpectedly matched the `details` value."
    assert 0 == len(
        search(clp_core_path_config, multimsg_archive.path, "details.key_value.int_value: 10")
    ), "`details` unexpectedly matched the `message` value."


@pytest.mark.clpp
@pytest.mark.clpp_decomposition
def test_multimsg_negation_scoped_to_field(
    clp_core_path_config: ClpCorePathConfig,
    multimsg_archive: ClppArchive,
) -> None:
    """
    Validates that negating a value query excludes events based only on the queried field.

    :param clp_core_path_config:
    :param multimsg_archive:
    """
    total_details = len(search(clp_core_path_config, multimsg_archive.path, "details: *"))
    query = 'not details: "*shared*"'

    results = search(clp_core_path_config, multimsg_archive.path, query)
    assert len(results) == total_details - _MultiMessage.shared_details_events, (
        f"Query '{query}' expected {total_details - _MultiMessage.shared_details_events} results, "
        f"got {len(results)}."
    )
    assert all(result["details"] != "shared 444 text" for result in results)
    assert all(result["details"] != "shared 777 text" for result in results)


@pytest.mark.clpp
@pytest.mark.parametrize(
    ("query", "expected_count"),
    [
        pytest.param('shape(message): "alpha*"', 2, id="shape_message_alpha"),
        pytest.param('shape(details): "shared*"', 2, id="shape_details_shared"),
    ],
)
def test_multimsg_shape_filter_scoping(
    clp_core_path_config: ClpCorePathConfig,
    multimsg_archive: ClppArchive,
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
    results = search(clp_core_path_config, multimsg_archive.path, query)
    assert len(results) == expected_count, (
        f"Query '{query}' expected {expected_count} results, got {len(results)}."
    )


@pytest.mark.clpp
@pytest.mark.clpp_decomposition
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
    multimsg_archive: ClppArchive,
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
    results = search(clp_core_path_config, multimsg_archive.path, query, projection)
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

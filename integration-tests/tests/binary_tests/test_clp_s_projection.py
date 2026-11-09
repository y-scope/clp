"""
Integration tests verifying how `clp-s s --projection` interprets its column arguments.

Each argument is treated as a double-quoted KQL column name, so `my key` names the key `my key`
rather than the key `my`. Standard KQL escape sequences are accepted.
"""

import json
from pathlib import Path
from typing import Any, Final

import pytest

from tests.binary_tests.clp_s_utils import compress, search
from tests.utils.config import ClpCorePathConfig

pytestmark = pytest.mark.core

# One event whose keys collide with how a projection argument could be mis-tokenized: `my` is a
# prefix of `my key`, and `a.b` is both a dotted key and a nested path.
_EVENT: Final[dict[str, Any]] = {
    "my key": "value",
    "my": "value 2",
    "a.b": 1,
    "a": {"b": 2},
    "foo(bar)": 3,
}


@pytest.fixture(scope="module")
def projection_archive(
    clp_core_path_config: ClpCorePathConfig, tmp_path_factory: pytest.TempPathFactory
) -> Path:
    """
    :param clp_core_path_config:
    :param tmp_path_factory:
    :return: The path to a clp-s archive containing `_EVENT`.
    """
    tmp_path = tmp_path_factory.mktemp("clp-s-projection")
    log_path = tmp_path / "event.jsonl"
    log_path.write_text(json.dumps(_EVENT) + "\n")
    archive_path = tmp_path / "archives"
    compress(clp_core_path_config, archive_path, log_path)
    return archive_path


@pytest.mark.clp_s
@pytest.mark.parametrize(
    ("projection", "expected"),
    [
        pytest.param(["my key"], {"my key": "value"}, id="space_in_key"),
        pytest.param(["my"], {"my": "value 2"}, id="prefix_key"),
        pytest.param(['"my key"'], {}, id="quotes_are_literal"),
        pytest.param(["a.b"], {"a": {"b": 2}}, id="dot_is_path_separator"),
        pytest.param(["a\\.b"], {"a.b": 1}, id="escaped_dot"),
        pytest.param(["foo(bar)"], {"foo(bar)": 3}, id="unescaped_parentheses"),
        pytest.param(["foo\\(bar\\)"], {"foo(bar)": 3}, id="escaped_parentheses"),
        pytest.param(["foo\\u0028bar\\u0029"], {"foo(bar)": 3}, id="unicode_escapes"),
        pytest.param(["my key", "a.b"], {"my key": "value", "a": {"b": 2}}, id="multiple"),
    ],
)
def test_clp_s_projection_bare_column_is_literal(
    clp_core_path_config: ClpCorePathConfig,
    projection_archive: Path,
    projection: list[str],
    expected: dict[str, Any],
) -> None:
    """
    Validates that each bare `--projection` argument is treated as a double-quoted column name.

    :param clp_core_path_config:
    :param projection_archive:
    :param projection: Column specifiers passed via `--projection`.
    :param expected: The expected (single) search result.
    """
    results = search(clp_core_path_config, projection_archive, "*", projection)
    assert results == [expected], (
        f"Projection {projection} produced unexpected output.\n"
        f"  Expected: {expected!r}\n"
        f"  Actual:   {results!r}"
    )

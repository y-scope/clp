"""Helpers for driving the `clp-s` binary in integration tests."""

import json
from pathlib import Path
from typing import Any

import pytest

from tests.utils.classes import ClpAction
from tests.utils.config import ClpCorePathConfig


def compress(
    clp_core_path_config: ClpCorePathConfig,
    archives_dir: Path,
    input_path: Path,
    *,
    timestamp_key: str | None = None,
) -> None:
    """
    Runs `clp-s c`, failing the test on a non-zero exit.

    :param clp_core_path_config:
    :param archives_dir:
    :param input_path:
    :param timestamp_key: Passed via `--timestamp-key` when set.
    """
    cmd = [str(clp_core_path_config.clp_s_binary_path), "c"]
    if timestamp_key is not None:
        cmd.extend(["--timestamp-key", timestamp_key])
    cmd.extend([str(archives_dir), str(input_path)])
    _run_and_verify(cmd)


def extract(
    clp_core_path_config: ClpCorePathConfig,
    archive_path: Path,
    output_dir: Path,
    *,
    ordered: bool = False,
) -> None:
    """
    Runs `clp-s x`, failing the test on a non-zero exit.

    :param clp_core_path_config:
    :param archive_path:
    :param output_dir:
    :param ordered: Passes `--ordered` when set.
    """
    cmd = [str(clp_core_path_config.clp_s_binary_path), "x"]
    if ordered:
        cmd.append("--ordered")
    cmd.extend([str(archive_path), str(output_dir)])
    _run_and_verify(cmd)


def build_search_cmd(
    clp_core_path_config: ClpCorePathConfig,
    archive_path: Path,
    query: str,
    projection: list[str] | None = None,
) -> list[str]:
    """
    :param clp_core_path_config:
    :param archive_path:
    :param query: KQL query string.
    :param projection: Optional column specifiers passed via `--projection`.
    :return: The `clp-s s` command for the given query and optional projection.
    """
    cmd = [str(clp_core_path_config.clp_s_binary_path), "s", str(archive_path), query]
    if projection:
        cmd.append("--projection")
        cmd.extend(projection)
    return cmd


def search(
    clp_core_path_config: ClpCorePathConfig,
    archive_path: Path,
    query: str,
    projection: list[str] | None = None,
) -> list[dict[str, Any]]:
    """
    Runs `clp-s s` and parses each output line as a JSON object. A non-zero exit or any non-JSON
    output line fails the test.

    :param clp_core_path_config:
    :param archive_path:
    :param query: KQL query string.
    :param projection: Optional column specifiers passed via `--projection`.
    :return: The parsed search results.
    """
    cmd = build_search_cmd(clp_core_path_config, archive_path, query, projection)
    action = _run_and_verify(cmd)

    context = f"Query:      {query!r}\nArchive:    {archive_path}\nProjection: {projection!r}\n"
    results: list[dict[str, Any]] = []
    for line in action.completed_proc.stdout.splitlines():
        if not line.startswith("{"):
            pytest.fail(f"Search output line is not a JSON object.\n{context}Line:       {line!r}")
        try:
            results.append(json.loads(line))
        except json.JSONDecodeError as e:
            pytest.fail(
                f"Failed to parse search output as JSON.\n{context}"
                f"Error:      {e}\nLine:       {line!r}"
            )
    return results


def _run_and_verify(cmd: list[str]) -> ClpAction:
    """
    Runs a CLP command, failing the test on a non-zero exit.

    :param cmd:
    :return: The completed action.
    """
    action = ClpAction.from_cmd(cmd)
    result = action.verify_returncode()
    assert result, result.failure_message
    return action

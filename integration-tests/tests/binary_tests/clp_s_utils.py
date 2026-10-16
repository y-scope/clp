"""
Helpers for driving the `clp-s` binary in integration tests.

The read path (`clp-s x` / `clp-s s`) detects an archive's flavour from the archive itself, so the
same helpers are used for clp-s and clpp archives. Only compression takes a flavour: passing a
parsing specification to `compress` selects `--flavour clpp`.
"""

import json
from collections.abc import Iterator
from contextlib import contextmanager
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Final

import pytest

from tests.utils.classes import ClpAction, IntegrationTestPathConfig, SampleDataset
from tests.utils.config import ClpCorePathConfig, CompressionTestPathConfig
from tests.utils.fs_validation import is_json_file_structurally_equal

_PARSING_SPEC_PATH_FROM_REPO_ROOT: Final[str] = (
    "components/package-template/src/etc/parsing-spec.template.txt"
)


def get_parsing_spec_path(integration_test_path_config: IntegrationTestPathConfig) -> Path:
    """
    :param integration_test_path_config:
    :return: The path to the parsing specification used to compress clpp archives.
    """
    parsing_spec_path = (
        integration_test_path_config.integration_tests_project_root.parent
        / _PARSING_SPEC_PATH_FROM_REPO_ROOT
    )
    if not parsing_spec_path.is_file():
        pytest.fail(f"Parsing specification not found: '{parsing_spec_path}'")
    return parsing_spec_path


def compress(
    clp_core_path_config: ClpCorePathConfig,
    archives_dir: Path,
    input_path: Path,
    *,
    timestamp_key: str | None = None,
    parsing_spec_path: Path | None = None,
) -> None:
    """
    Runs `clp-s c`, failing the test on a non-zero exit.

    :param clp_core_path_config:
    :param archives_dir:
    :param input_path:
    :param timestamp_key: Passed via `--timestamp-key` when set.
    :param parsing_spec_path: When set, compresses with `--flavour clpp` using this parsing
        specification; otherwise compresses with the default clp-s flavour.
    """
    cmd = [str(clp_core_path_config.clp_s_binary_path), "c"]
    if timestamp_key is not None:
        cmd.extend(["--timestamp-key", timestamp_key])
    if parsing_spec_path is not None:
        cmd.extend(["--flavour", "clpp", f"--parsing-specification={parsing_spec_path}"])
    cmd.extend([str(archives_dir), str(input_path)])
    _run_and_verify(cmd)


def compress_dataset_with_clpp(
    clp_core_path_config: ClpCorePathConfig,
    integration_test_path_config: IntegrationTestPathConfig,
    dataset: SampleDataset,
    archives_dir: Path,
) -> None:
    """
    Compresses a dataset's logs with the clpp flavour, keyed on the dataset's timestamp key.

    :param clp_core_path_config:
    :param integration_test_path_config:
    :param dataset:
    :param archives_dir:
    """
    timestamp_key = dataset.metadata.timestamp_key
    if timestamp_key is None:
        pytest.fail(f"The `{dataset.dataset_name}` dataset must define a `timestamp_key`.")
    compress(
        clp_core_path_config,
        archives_dir,
        dataset.logs_path,
        timestamp_key=timestamp_key,
        parsing_spec_path=get_parsing_spec_path(integration_test_path_config),
    )


@dataclass(frozen=True)
class ClppArchive:
    """A dataset compressed with the clpp flavour, plus a scratch directory for extracting it."""

    dataset: SampleDataset
    path: Path
    decompression_dir: Path


@contextmanager
def clpp_archive_for(
    clp_core_path_config: ClpCorePathConfig,
    integration_test_path_config: IntegrationTestPathConfig,
    dataset: SampleDataset,
) -> Iterator[ClppArchive]:
    """
    Compresses `dataset` with the clpp flavour into fresh test output directories, removing them on
    exit. Intended to back module-scoped fixtures so the archive is shared across a module's tests.

    :param clp_core_path_config:
    :param integration_test_path_config:
    :param dataset:
    :return: An iterator yielding the compressed archive.
    """
    test_paths = CompressionTestPathConfig(
        test_name=f"clpp-{dataset.dataset_name}",
        logs_source_path=dataset.logs_path,
        integration_test_path_config=integration_test_path_config,
    )
    test_paths.clear_test_outputs()
    try:
        compress_dataset_with_clpp(
            clp_core_path_config, integration_test_path_config, dataset, test_paths.compression_dir
        )
        yield ClppArchive(dataset, test_paths.compression_dir, test_paths.decompression_dir)
    finally:
        test_paths.clear_test_outputs()


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


def assert_dataset_roundtrip(clp_core_path_config: ClpCorePathConfig, archive: ClppArchive) -> None:
    """
    Extracts `archive` in log order and asserts the result is structurally equal (i.e., equal up to
    JSON key order) to the dataset's files concatenated in metadata order.

    :param clp_core_path_config:
    :param archive:
    """
    dataset = archive.dataset
    output_dir = archive.decompression_dir
    extract(clp_core_path_config, archive.path, output_dir, ordered=True)

    extracted_paths = list(output_dir.glob("*.jsonl"))
    assert 1 == len(extracted_paths), f"Expected one output file, got: {extracted_paths}"
    extracted_path = extracted_paths[0]

    consolidated_input_path = output_dir / f"{dataset.dataset_name}-consolidated-input.jsonl"
    with consolidated_input_path.open("w", encoding="utf-8") as consolidated_input_file:
        for file_name in dataset.metadata.file_names:
            consolidated_input_file.write(
                (dataset.logs_path / file_name).read_text(encoding="utf-8")
            )

    assert is_json_file_structurally_equal(consolidated_input_path, extracted_path), (
        f"Mismatch between input {consolidated_input_path} and output {extracted_path}."
    )


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

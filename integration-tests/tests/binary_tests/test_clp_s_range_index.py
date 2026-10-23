"""
Integration tests verifying the per-ingestion-unit metadata that `clp-s` records in the range index
of each archive it creates.
"""

import json
from datetime import datetime, timedelta, timezone
from pathlib import Path
from typing import Any

import pytest

from tests.utils.classes import (
    ClpAction,
    IntegrationTestPathConfig,
    NonClpAction,
    SampleDataset,
)
from tests.utils.config import (
    ClpCorePathConfig,
    CompressionTestPathConfig,
    ConversionTestPathConfig,
)

# Matching `LogSerializer::cTimestampKey`.
LOG_CONVERTER_OUTPUT_TIMESTAMP_KEY = "timestamp"

# Matching `RangeIndexWriter::cMetadataFieldsName`.
RANGE_FIELDS_KEY = "f"

# Matching `clp_s::constants::range_index`.
FILENAME_KEY = "_filename"
NUM_UNCOMPRESSED_BYTES_KEY = "_num_uncompressed_bytes"
TIMESTAMP_RANGE_BEGIN_MILLIS_KEY = "_timestamp_range_begin_millis"
TIMESTAMP_RANGE_END_MILLIS_KEY = "_timestamp_range_end_millis"

_UNIX_EPOCH = datetime(1970, 1, 1, tzinfo=timezone.utc)

pytestmark = pytest.mark.core


@pytest.mark.clp_s
@pytest.mark.parametrize(
    "use_timestamp_key", [True, False], ids=["timestamp-key", "no-timestamp-key"]
)
def test_clp_s_json_range_index(
    use_timestamp_key: bool,
    clp_core_path_config: ClpCorePathConfig,
    integration_test_path_config: IntegrationTestPathConfig,
    json_multifile: SampleDataset,
) -> None:
    """
    Validates that `clp-s` records the size and time range of every JSON file it ingests.

    :param use_timestamp_key: Whether to compress with the dataset's authoritative timestamp key.
    :param clp_core_path_config:
    :param integration_test_path_config:
    :param json_multifile:
    """
    timestamp_key = json_multifile.metadata.timestamp_key
    assert timestamp_key is not None

    expected_metadata: dict[str, dict[str, Any]] = {}
    for file_name in json_multifile.metadata.file_names:
        file_path = json_multifile.logs_path / file_name
        with file_path.open(encoding="utf-8") as f:
            timestamps_millis = [json.loads(line)[timestamp_key] for line in f]
        expected_metadata[Path(file_name).name] = _get_expected_metadata(
            file_path.stat().st_size, timestamps_millis if use_timestamp_key else None
        )

    test_paths = CompressionTestPathConfig(
        test_name=f"clp-s-range-index-{json_multifile.dataset_name}",
        logs_source_path=json_multifile.logs_path,
        integration_test_path_config=integration_test_path_config,
    )
    test_paths.clear_test_outputs()
    try:
        compression_action = _compress_with_archive_stats(
            clp_core_path_config,
            test_paths.compression_dir,
            [test_paths.logs_source_path],
            ["--timestamp-key", timestamp_key] if use_timestamp_key else [],
        )
        _verify_ingestion_unit_metadata(compression_action, expected_metadata)
    finally:
        test_paths.clear_test_outputs()


@pytest.mark.clp_s
@pytest.mark.parametrize(
    "use_timestamp_key", [True, False], ids=["timestamp-key", "no-timestamp-key"]
)
def test_clp_s_kv_ir_range_index(
    use_timestamp_key: bool,
    clp_core_path_config: ClpCorePathConfig,
    integration_test_path_config: IntegrationTestPathConfig,
    text_multifile: SampleDataset,
) -> None:
    """
    Validates that `clp-s` records the size and time range of every KV-IR stream it ingests, using
    KV-IR streams converted from unstructured text by `log-converter`.

    :param use_timestamp_key: Whether to compress with `log-converter`'s timestamp key.
    :param clp_core_path_config:
    :param integration_test_path_config:
    :param text_multifile:
    """
    test_paths = ConversionTestPathConfig(
        test_name=f"clp-s-range-index-{text_multifile.dataset_name}",
        logs_source_path=text_multifile.logs_path,
        integration_test_path_config=integration_test_path_config,
    )
    test_paths.clear_test_outputs()
    try:
        test_paths.conversion_dir.mkdir(parents=True)
        expected_metadata: dict[str, dict[str, Any]] = {}
        for file_name in text_multifile.metadata.file_names:
            text_file_path = text_multifile.logs_path / file_name
            # `log-converter` gives its output a random name, so convert each file into its own
            # directory to know which source file each KV-IR stream came from.
            kv_ir_path = _convert_to_uncompressed_kv_ir(
                clp_core_path_config,
                text_file_path,
                test_paths.conversion_dir / Path(file_name).stem,
            )
            with text_file_path.open(encoding="utf-8") as f:
                timestamps_millis = [_parse_leading_timestamp_millis(line) for line in f]
            expected_metadata[kv_ir_path.name] = _get_expected_metadata(
                kv_ir_path.stat().st_size, timestamps_millis if use_timestamp_key else None
            )

        compression_action = _compress_with_archive_stats(
            clp_core_path_config,
            test_paths.compression_dir,
            sorted(test_paths.conversion_dir.glob("*/*")),
            ["--timestamp-key", LOG_CONVERTER_OUTPUT_TIMESTAMP_KEY] if use_timestamp_key else [],
        )
        _verify_ingestion_unit_metadata(compression_action, expected_metadata)
    finally:
        test_paths.clear_test_outputs()


@pytest.mark.clp_s
def test_clp_s_disable_log_order_omits_range_index(
    clp_core_path_config: ClpCorePathConfig,
    integration_test_path_config: IntegrationTestPathConfig,
    json_multifile: SampleDataset,
) -> None:
    """
    Validates that `clp-s` records no range index when compressing with `--disable-log-order`.

    :param clp_core_path_config:
    :param integration_test_path_config:
    :param json_multifile:
    """
    timestamp_key = json_multifile.metadata.timestamp_key
    assert timestamp_key is not None

    test_paths = CompressionTestPathConfig(
        test_name=f"clp-s-range-index-disabled-{json_multifile.dataset_name}",
        logs_source_path=json_multifile.logs_path,
        integration_test_path_config=integration_test_path_config,
    )
    test_paths.clear_test_outputs()
    try:
        compression_action = _compress_with_archive_stats(
            clp_core_path_config,
            test_paths.compression_dir,
            [test_paths.logs_source_path],
            ["--disable-log-order", "--timestamp-key", timestamp_key],
        )
        archive_stats = _get_archive_stats(compression_action)
        assert len(archive_stats) > 0, "Expected `clp-s` to print stats for at least one archive."
        for stats in archive_stats:
            assert stats["range_index"] == [], f"Expected an empty range index: {stats}"
    finally:
        test_paths.clear_test_outputs()


def _compress_with_archive_stats(
    clp_core_path_config: ClpCorePathConfig,
    compression_dir: Path,
    input_paths: list[Path],
    options: list[str],
) -> ClpAction:
    compression_action = ClpAction.from_cmd(
        [
            str(clp_core_path_config.clp_s_binary_path),
            "c",
            "--print-archive-stats",
            *options,
            str(compression_dir),
            *(str(path) for path in input_paths),
        ]
    )
    compression_result = compression_action.verify_returncode()
    assert compression_result, compression_result.failure_message
    return compression_action


def _convert_to_uncompressed_kv_ir(
    clp_core_path_config: ClpCorePathConfig,
    text_file_path: Path,
    output_dir: Path,
) -> Path:
    """:return: The path of the single KV-IR stream converted from `text_file_path`."""
    # fmt: off
    conversion_action = NonClpAction(
        cmd=[
            str(clp_core_path_config.log_converter_binary_path),
            str(text_file_path),
            "--output-dir", str(output_dir),
            "--no-compress-converted-files",
        ]
    )
    # fmt: on
    conversion_action.check_returncode()
    converted_paths = list(output_dir.iterdir())
    if len(converted_paths) != 1:
        err_msg = f"Expected one converted file in '{output_dir}', found: {converted_paths}"
        raise RuntimeError(err_msg)
    return converted_paths[0]


def _parse_leading_timestamp_millis(line: str) -> int:
    """:return: The zoneless ISO 8601 timestamp that begins `line`, as UTC epoch milliseconds."""
    timestamp = datetime.fromisoformat(line.split(" ", 1)[0]).replace(tzinfo=timezone.utc)
    return (timestamp - _UNIX_EPOCH) // timedelta(milliseconds=1)


def _get_expected_metadata(num_bytes: int, timestamps_millis: list[int] | None) -> dict[str, Any]:
    """:return: The range index metadata expected for an ingestion unit."""
    return {
        NUM_UNCOMPRESSED_BYTES_KEY: num_bytes,
        TIMESTAMP_RANGE_BEGIN_MILLIS_KEY: min(timestamps_millis) if timestamps_millis else None,
        TIMESTAMP_RANGE_END_MILLIS_KEY: max(timestamps_millis) if timestamps_millis else None,
    }


def _get_archive_stats(compression_action: ClpAction) -> list[dict[str, Any]]:
    return [json.loads(line) for line in compression_action.completed_proc.stdout.splitlines()]


def _verify_ingestion_unit_metadata(
    compression_action: ClpAction,
    expected_metadata: dict[str, dict[str, Any]],
) -> None:
    """
    :param compression_action: A `clp-s` compression run with `--print-archive-stats`.
    :param expected_metadata: Map from each ingestion unit's file name to its expected metadata.
    """
    metadata: dict[str, dict[str, Any]] = {}
    for stats in _get_archive_stats(compression_action):
        for range_entry in stats["range_index"]:
            fields = range_entry[RANGE_FIELDS_KEY]
            metadata[Path(fields[FILENAME_KEY]).name] = {
                key: fields[key]
                for key in (
                    NUM_UNCOMPRESSED_BYTES_KEY,
                    TIMESTAMP_RANGE_BEGIN_MILLIS_KEY,
                    TIMESTAMP_RANGE_END_MILLIS_KEY,
                )
                if key in fields
            }
    assert metadata == expected_metadata, (
        f"See subprocess log at: '{compression_action.log_file_path}'."
    )

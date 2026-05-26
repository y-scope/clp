"""
Integration tests verifying that the log-converter to clp-s pipeline is able to parse and compress
logs as expected.
"""

import json
from pathlib import Path

import pytest

from tests.utils.archive import create_tar_gz_from_dir
from tests.utils.classes import (
    ClpAction,
    IntegrationTestPathConfig,
    NonClpAction,
    SampleDataset,
)
from tests.utils.config import (
    ClpCorePathConfig,
    ConversionTestPathConfig,
)

# Matching `LogSerializer::cTimestampKey`.
LOG_CONVERTER_OUTPUT_TIMESTAMP_KEY = "timestamp"

pytestmark = pytest.mark.core


@pytest.mark.clp_s
def test_log_converter_transform(
    clp_core_path_config: ClpCorePathConfig,
    integration_test_path_config: IntegrationTestPathConfig,
    text_singlefile: SampleDataset,
) -> None:
    """
    Validate that converted logs from the core binary `log-converter` can be ingested successfully
    by `clp-s`.

    :param clp_core_path_config:
    :param integration_test_path_config:
    :param text_singlefile:
    """
    num_log_events = 0
    for file_name in text_singlefile.metadata.file_names:
        with (text_singlefile.logs_path / file_name).open(encoding="utf-8") as f:
            num_log_events += sum(1 for _ in f)

    test_paths = ConversionTestPathConfig(
        test_name=f"clp-s-{text_singlefile.dataset_name}",
        logs_source_dir=text_singlefile.logs_path,
        num_log_events=num_log_events,
        integration_test_path_config=integration_test_path_config,
    )
    try:
        _convert_and_compress(clp_core_path_config, test_paths, test_paths.logs_source_dir)
    finally:
        test_paths.clear_test_outputs()


@pytest.mark.clp_s
def test_log_converter_tar_gz_transform(
    clp_core_path_config: ClpCorePathConfig,
    integration_test_path_config: IntegrationTestPathConfig,
    text_singlefile: SampleDataset,
) -> None:
    """
    Validate that `log-converter` can ingest a .tar.gz archive and that the converted logs can be
    compressed and searched by `clp-s`.

    :param clp_core_path_config:
    :param integration_test_path_config:
    :param text_singlefile:
    """
    num_log_events = 0
    for file_name in text_singlefile.metadata.file_names:
        with (text_singlefile.logs_path / file_name).open(encoding="utf-8") as f:
            num_log_events += sum(1 for _ in f)

    tar_gz_path = (
        integration_test_path_config.test_cache_dir
        / f"clp-s-{text_singlefile.dataset_name}-input.tar.gz"
    )
    create_tar_gz_from_dir(text_singlefile.logs_path, tar_gz_path)

    test_paths = ConversionTestPathConfig(
        test_name=f"clp-s-{text_singlefile.dataset_name}-tar-gz",
        logs_source_dir=text_singlefile.logs_path,
        num_log_events=num_log_events,
        integration_test_path_config=integration_test_path_config,
    )
    try:
        _convert_and_compress(clp_core_path_config, test_paths, tar_gz_path)
    finally:
        test_paths.clear_test_outputs()


def _convert_and_compress(
    clp_core_path_config: ClpCorePathConfig,
    test_paths: ConversionTestPathConfig,
    src_path: Path,
) -> None:
    log_converter_bin_path = str(clp_core_path_config.log_converter_binary_path)
    clp_s_bin_path = str(clp_core_path_config.clp_s_binary_path)
    conversion_path = str(test_paths.conversion_dir)
    compression_path = str(test_paths.compression_dir)
    conversion_action = NonClpAction(
        cmd=[log_converter_bin_path, src_path, "--output-dir", conversion_path]
    )
    conversion_action.check_returncode()

    compression_action = ClpAction.from_cmd(
        [
            clp_s_bin_path,
            "c",
            compression_path,
            conversion_path,
            "--timestamp-key",
            LOG_CONVERTER_OUTPUT_TIMESTAMP_KEY,
        ]
    )
    compression_result = compression_action.verify_returncode()
    assert compression_result, compression_result.failure_message

    _verify_search_results(clp_s_bin_path, compression_path, test_paths.num_log_events)


def _verify_search_results(
    clp_s_bin_path: str,
    compression_path: str,
    num_log_events: int | None,
) -> None:
    if num_log_events is None:
        return

    search_action = ClpAction.from_cmd([clp_s_bin_path, "s", compression_path, "timestamp > 0"])
    search_result = search_action.verify_returncode()
    assert search_result, search_result.failure_message
    lines = search_action.completed_proc.stdout.splitlines()
    if len(lines) != num_log_events:
        pytest.fail(
            f"Expected {num_log_events} log events after conversion, but found {len(lines)}."
        )

    # Verify every event's message field.
    for idx, line in enumerate(lines):
        event = json.loads(line)
        message = event.get("message", "")
        if f"TEST{idx + 1}" not in message:
            pytest.fail(f"Expected 'TEST{idx + 1}' in message of event {idx + 1}, but got: {event}")

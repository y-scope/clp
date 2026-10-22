"""
Integration tests verifying that the log-converter to clp-s pipeline is able to parse and compress
logs as expected.
"""

from pathlib import Path

import pytest

from tests.binary_tests import clp_s_utils
from tests.utils.archive import create_tar_gz_from_dir
from tests.utils.classes import (
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
@pytest.mark.parametrize("input_type", ["directory", "tar_gz"])
def test_log_converter_transform(
    input_type: str,
    clp_core_path_config: ClpCorePathConfig,
    integration_test_path_config: IntegrationTestPathConfig,
    text_singlefile: SampleDataset,
) -> None:
    """
    Validates that converted logs from the core binary `log-converter` can be ingested successfully
    by `clp-s`, for both raw directory and `.tar.gz` archive inputs.

    :param input_type: "directory" for raw logs dir, "tar_gz" for a `.tar.gz` archive.
    :param clp_core_path_config:
    :param integration_test_path_config:
    :param text_singlefile:
    """
    num_log_events = 0
    for file_name in text_singlefile.metadata.file_names:
        with (text_singlefile.logs_path / file_name).open(encoding="utf-8") as f:
            num_log_events += sum(1 for _ in f)

    if input_type == "tar_gz":
        tar_gz_path = (
            integration_test_path_config.test_cache_dir
            / f"clp-s-{text_singlefile.dataset_name}-input.tar.gz"
        )
        create_tar_gz_from_dir(text_singlefile.logs_path, tar_gz_path)
        logs_source_path = tar_gz_path
        test_name_suffix = "-tar-gz"
    else:  # input_type == "directory"
        logs_source_path = text_singlefile.logs_path
        test_name_suffix = ""

    test_paths = ConversionTestPathConfig(
        test_name=f"clp-s-{text_singlefile.dataset_name}{test_name_suffix}",
        logs_source_path=logs_source_path,
        num_log_events=num_log_events,
        integration_test_path_config=integration_test_path_config,
    )
    try:
        _convert_and_compress(clp_core_path_config, test_paths)
    finally:
        test_paths.clear_test_outputs()


def _convert_and_compress(
    clp_core_path_config: ClpCorePathConfig,
    test_paths: ConversionTestPathConfig,
) -> None:
    conversion_action = NonClpAction(
        cmd=[
            str(clp_core_path_config.log_converter_binary_path),
            str(test_paths.logs_source_path),
            "--output-dir",
            str(test_paths.conversion_dir),
        ]
    )
    conversion_action.check_returncode()

    clp_s_utils.compress(
        clp_core_path_config,
        test_paths.compression_dir,
        test_paths.conversion_dir,
        timestamp_key=LOG_CONVERTER_OUTPUT_TIMESTAMP_KEY,
    )

    _verify_search_results(
        clp_core_path_config, test_paths.compression_dir, test_paths.num_log_events
    )


def _verify_search_results(
    clp_core_path_config: ClpCorePathConfig,
    archives_dir: Path,
    num_log_events: int | None,
) -> None:
    if num_log_events is None:
        return

    events = clp_s_utils.search(clp_core_path_config, archives_dir, "timestamp > 0")
    if len(events) != num_log_events:
        pytest.fail(
            f"Expected {num_log_events} log events after conversion, but found {len(events)}."
        )

    # Verify every event's message field.
    for idx, event in enumerate(events):
        message = event.get("message", "")
        if f"TEST{idx + 1}" not in message:
            pytest.fail(f"Expected 'TEST{idx + 1}' in message of event {idx + 1}, but got: {event}")

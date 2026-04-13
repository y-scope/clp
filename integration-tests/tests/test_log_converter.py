"""
Integration tests verifying that CLP core compression binaries perform lossless round-trip
compression and decompression.
"""

import pytest

from tests.utils.config import (
    ClpCorePathConfig,
    ConversionTestPathConfig,
    IntegrationTestLogs,
    IntegrationTestPathConfig,
)
from tests.utils.subprocess_utils import run_and_log_subprocess

pytestmark = pytest.mark.core

text_datasets = pytest.mark.parametrize(
    "test_logs_fixture",
    [
        "simple_unstructured",
    ],
)


@pytest.mark.clp_s
@text_datasets
def test_log_converter_transform(
    request: pytest.FixtureRequest,
    clp_core_path_config: ClpCorePathConfig,
    integration_test_path_config: IntegrationTestPathConfig,
    test_logs_fixture: str,
) -> None:
    """
    Validate that converted logs from the core binary `log-converter` can be ingested successfully
    by `clp-s`.

    :param request:
    :param clp_core_path_config:
    :param integration_test_path_config:
    :param test_logs_fixture:
    """
    integration_test_logs: IntegrationTestLogs = request.getfixturevalue(test_logs_fixture)
    test_logs_name = integration_test_logs.name

    test_paths = ConversionTestPathConfig(
        test_name=f"clp-s-{test_logs_name}",
        logs_source_dir=integration_test_logs.extraction_dir,
        num_log_events=integration_test_logs.num_log_events,
        integration_test_path_config=integration_test_path_config,
    )
    _convert_and_compress(clp_core_path_config, test_paths)

    test_paths.clear_test_outputs()


def _convert_and_compress(
    clp_core_path_config: ClpCorePathConfig,
    test_paths: ConversionTestPathConfig,
) -> None:
    test_paths.clear_test_outputs()
    log_converter_bin_path = str(clp_core_path_config.log_converter_binary_path)
    clp_s_bin_path = str(clp_core_path_config.clp_s_binary_path)
    src_path = str(test_paths.logs_source_dir)
    conversion_path = str(test_paths.conversion_dir)
    compression_path = str(test_paths.compression_dir)
    run_and_log_subprocess([log_converter_bin_path, src_path, "--output-dir", conversion_path])
    run_and_log_subprocess(
        [clp_s_bin_path, "c", compression_path, conversion_path, "--timestamp-key", "timestamp"]
    )

    if test_paths.num_log_events is None:
        return

    output = run_and_log_subprocess([clp_s_bin_path, "s", compression_path, "timestamp > 0"])
    num_events = 0 if output.stdout is None else len(output.stdout.strip().split("\n"))
    if num_events != test_paths.num_log_events:
        pytest.fail(
            f"Expected {test_paths.num_log_events} log events after conversion, "
            f"but found {num_events}."
        )

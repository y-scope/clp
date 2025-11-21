"""
Integration tests verifying that CLP core compression binaries perform lossless round-trip
compression and decompression.
"""

from pathlib import Path

import pytest

from tests.utils.asserting_utils import run_and_assert
from tests.utils.config import (
    ClpCorePathConfig,
    CompressionTestPathConfig,
    IntegrationTestLogs,
    IntegrationTestPathConfig,
)
from tests.utils.utils import (
    is_dir_tree_content_equal,
    is_json_file_structurally_equal,
    unlink,
)

pytestmark = pytest.mark.core

# Constants
CLP_S_CANONICAL_OUTPUT_FILENAME = "original"

text_datasets = pytest.mark.parametrize(
    "test_logs_fixture",
    [
        "hive_24hr",
    ],
)

json_datasets = pytest.mark.parametrize(
    "test_logs_fixture",
    [
        "postgresql",
    ],
)


@pytest.mark.clp
@text_datasets
def test_clp_identity_transform(
    request: pytest.FixtureRequest,
    clp_core_path_config: ClpCorePathConfig,
    integration_test_path_config: IntegrationTestPathConfig,
    test_logs_fixture: str,
) -> None:
    """
    Validate that compression and decompression by the core binary `clp` run successfully and are
    lossless.

    :param request:
    :param clp_core_path_config:
    :param integration_test_path_config:
    :param test_logs_fixture:
    """
    integration_test_logs: IntegrationTestLogs = request.getfixturevalue(test_logs_fixture)
    test_paths = CompressionTestPathConfig(
        test_name=f"clp-{integration_test_logs.name}",
        logs_source_dir=integration_test_logs.extraction_dir,
        integration_test_path_config=integration_test_path_config,
    )
    test_paths.clear_test_outputs()

    bin_path = str(clp_core_path_config.clp_binary_path)
    src_path = str(test_paths.logs_source_dir)
    compression_path = str(test_paths.compression_dir)
    decompression_path = str(test_paths.decompression_dir)
    # fmt: off
    compression_cmd = [
        bin_path,
        "c",
        "--progress",
        "--remove-path-prefix", src_path,
        compression_path,
        src_path,
    ]
    # fmt: on
    run_and_assert(compression_cmd)

    decompression_cmd = [bin_path, "x", compression_path, decompression_path]
    run_and_assert(decompression_cmd)

    input_path = test_paths.logs_source_dir
    output_path = test_paths.decompression_dir
    assert is_dir_tree_content_equal(
        input_path,
        output_path,
    ), f"Mismatch between clp input {input_path} and output {output_path}."

    test_paths.clear_test_outputs()


@pytest.mark.clp_s
@json_datasets
def test_clp_s_identity_transform(
    request: pytest.FixtureRequest,
    clp_core_path_config: ClpCorePathConfig,
    integration_test_path_config: IntegrationTestPathConfig,
    test_logs_fixture: str,
) -> None:
    """
    Validate that compression and decompression by the core binary `clp-s` run successfully and are
    lossless.

    :param request:
    :param clp_core_path_config:
    :param integration_test_path_config:
    :param test_logs_fixture:
    """
    integration_test_logs: IntegrationTestLogs = request.getfixturevalue(test_logs_fixture)
    test_logs_name = integration_test_logs.name

    test_paths = CompressionTestPathConfig(
        test_name=f"clp-s-{test_logs_name}",
        logs_source_dir=integration_test_logs.extraction_dir,
        integration_test_path_config=integration_test_path_config,
    )
    _clp_s_compress_and_decompress(clp_core_path_config, test_paths)

    # Recompress the decompressed output that's consolidated into a single json file, and decompress
    # it again to verify consistency. The compression input of the second iteration points to the
    # decompression output of the first.
    # TODO: Remove this check once we can directly compare decompressed logs (which would preserve
    #       the directory structure and row/key order) with the original downloaded logs.
    # See also: https://docs.yscope.com/clp/main/user-guide/core-clp-s.html#current-limitations
    consolidated_json_test_paths = CompressionTestPathConfig(
        test_name=f"clp-s-{test_logs_name}-consolidated-json",
        logs_source_dir=test_paths.decompression_dir,
        integration_test_path_config=integration_test_path_config,
    )
    _clp_s_compress_and_decompress(clp_core_path_config, consolidated_json_test_paths)

    input_path = consolidated_json_test_paths.logs_source_dir / CLP_S_CANONICAL_OUTPUT_FILENAME
    output_path = consolidated_json_test_paths.decompression_dir / CLP_S_CANONICAL_OUTPUT_FILENAME
    assert is_json_file_structurally_equal(input_path, output_path), (
        f"Mismatch between clp-s input {input_path} and output {output_path}."
    )

    test_paths.clear_test_outputs()
    consolidated_json_test_paths.clear_test_outputs()


@pytest.mark.clp_s
def test_log_converter_clp_s_identity_transform(
    core_config: CoreConfig,
    integration_test_config: IntegrationTestConfig,
) -> None:
    """
    Validate the end-to-end functionality of the `log-converter` and `clp-s` pipeline.

    This test ensures that:
    1. `log-converter` correctly transforms unstructured logs into key-value IR format.
    2. The kv-IR output can be compressed and decompressed by `clp-s` without data loss.

    :param core_config:
    :param integration_test_config:
    """
    log_converter_out_dir = integration_test_config.test_root_dir / "log-converter-outputs"
    log_converter_out_dir.mkdir(parents=True, exist_ok=True)
    log_converter_bin_path_str = str(core_config.log_converter_binary_path)

    unstructured_logs_dir = Path(__file__).resolve().parent / "data" / "unstructured-logs"
    for test_case_dir in unstructured_logs_dir.iterdir():
        if not test_case_dir.is_dir():
            continue

        test_name = test_case_dir.name
        kv_ir_out = log_converter_out_dir / test_name
        unlink(kv_ir_out)

        # fmt: off
        run_and_assert([
            log_converter_bin_path_str,
            str(test_case_dir / "raw.log"),
            "--output-dir", str(kv_ir_out),
        ])
        # fmt: on

        test_paths = CompressionTestConfig(
            test_name=f"log-converter-clp-s-{test_name}",
            logs_source_dir=kv_ir_out,
            integration_test_config=integration_test_config,
        )
        _clp_s_compress_and_decompress(core_config, test_paths)

        expected_out = test_case_dir / "converted.json"
        actual_out = test_paths.decompression_dir / CLP_S_CANONICAL_OUTPUT_FILENAME
        assert is_json_file_structurally_equal(expected_out, actual_out), (
            f"Mismatch between {expected_out}(expected) and {actual_out}(actual)."
        )

        test_paths.clear_test_outputs()


def _clp_s_compress_and_decompress(
    clp_core_path_config: ClpCorePathConfig,
    test_paths: CompressionTestPathConfig,
) -> None:
    test_paths.clear_test_outputs()
    bin_path = str(clp_core_path_config.clp_s_binary_path)
    src_path = str(test_paths.logs_source_dir)
    compression_path = str(test_paths.compression_dir)
    decompression_path = str(test_paths.decompression_dir)
    run_and_assert([bin_path, "c", compression_path, src_path])
    run_and_assert([bin_path, "x", compression_path, decompression_path])

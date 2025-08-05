from pathlib import Path

import pytest
from tests.utils.config import (
    CompressionTestConfig,
    TestConfig,
    TestLogs,
)
from tests.utils.utils import (
    is_dir_tree_content_equal,
    is_json_file_structurally_equal,
    run_and_assert,
)

pytestmark = pytest.mark.binaries

text_datasets = pytest.mark.parametrize(
    "test_logs_fixture",
    [
        "hive_24hr",
    ],
)

json_datasets = pytest.mark.parametrize(
    "test_logs_fixture",
    [
        "elasticsearch",
        "spark_event_logs",
        "postgresql",
    ],
)


@pytest.mark.clp
@text_datasets
def test_clp_identity_transform(
    request,
    test_config: TestConfig,
    test_logs_fixture: str,
) -> None:
    test_logs: TestLogs = request.getfixturevalue(test_logs_fixture)
    test_paths = CompressionTestConfig(
        test_name=f"clp-{test_logs.name}",
        logs_source_dir=test_logs.extraction_dir,
        test_config=test_config,
    )
    test_paths.clear_test_outputs()

    binary_path_str = str(test_config.get_clp_binary_path())
    # fmt: off
    compression_cmd = [
        binary_path_str,
        "c",
        "--progress",
        "--remove-path-prefix", str(test_paths.logs_source_dir),
        str(test_paths.compression_dir),
        str(test_paths.logs_source_dir),
    ]
    # fmt: on
    run_and_assert(compression_cmd)

    decompression_cmd = [
        binary_path_str,
        "x",
        str(test_paths.compression_dir),
        str(test_paths.decompression_dir),
    ]
    run_and_assert(decompression_cmd)

    input_path = test_paths.logs_source_dir
    output_path = test_paths.decompression_dir
    assert is_dir_tree_content_equal(
        input_path,
        output_path,
    ), "Mismatch between clp input {input_path} and output {output_path}."

    test_paths.clear_test_outputs()


@pytest.mark.clp_s
@json_datasets
def test_clp_s_identity_transform(
    request,
    test_config: TestConfig,
    test_logs_fixture: str,
) -> None:
    test_logs: TestLogs = request.getfixturevalue(test_logs_fixture)
    test_logs_name = test_logs.name

    test_paths = CompressionTestConfig(
        test_name=f"clp-s-{test_logs_name}",
        logs_source_dir=test_logs.extraction_dir,
        test_config=test_config,
    )
    _clp_s_compress_and_decompress(test_config, test_paths)

    # Recompress the decompressed output that's consolidated into a single json file, and decompress
    # it again to verify consistency. The compression input of the second iteration points to the
    # decompression output of the first.
    # TODO: Remove this check once we can directly compare decompressed logs (which would preserve
    #       the directory structure and row/key order) with the original downloaded logs.
    # See also: https://docs.yscope.com/clp/main/user-guide/core-clp-s.html#current-limitations
    consolidated_json_test_paths = CompressionTestConfig(
        test_name=f"clp-s-{test_logs_name}-consolidated-json",
        logs_source_dir=test_paths.decompression_dir,
        test_config=test_config,
    )
    _clp_s_compress_and_decompress(test_config, consolidated_json_test_paths)

    input_path = consolidated_json_test_paths.logs_source_dir / "original"
    output_path = consolidated_json_test_paths.decompression_dir / "original"
    assert is_json_file_structurally_equal(
        input_path, output_path
    ), f"Mismatch between clp-s input {input_path} and output {output_path}."

    test_paths.clear_test_outputs()
    consolidated_json_test_paths.clear_test_outputs()


def _clp_s_compress_and_decompress(
    test_config: TestConfig, test_paths: CompressionTestConfig
) -> None:
    test_paths.clear_test_outputs()
    bin_path = str(test_config.get_clp_s_binary_path())
    src_path = str(test_paths.logs_source_dir)
    compression_path = str(test_paths.compression_dir)
    decompression_path = str(test_paths.decompression_dir)
    run_and_assert([bin_path, "c", compression_path, src_path])
    run_and_assert([bin_path, "x", compression_path, decompression_path])

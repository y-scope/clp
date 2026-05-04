"""
Integration tests verifying that CLP core compression binaries perform lossless round-trip
compression and decompression.
"""

import pytest

from tests.utils.classes import IntegrationTestDataset, IntegrationTestPathConfig
from tests.utils.config import (
    ClpCorePathConfig,
    CompressionTestPathConfig,
)
from tests.utils.subprocess_utils import run_and_log_subprocess
from tests.utils.utils import (
    is_dir_tree_content_equal,
    is_json_file_structurally_equal,
)

pytestmark = pytest.mark.core


@pytest.mark.clp
def test_clp_identity_transform(
    clp_core_path_config: ClpCorePathConfig,
    integration_test_path_config: IntegrationTestPathConfig,
    text_multifile: IntegrationTestDataset,
) -> None:
    """
    Validate that compression and decompression by the core binary `clp` run successfully and are
    lossless.

    :param clp_core_path_config:
    :param integration_test_path_config:
    :param text_multifile:
    """
    test_paths = CompressionTestPathConfig(
        test_name=f"clp-{text_multifile.dataset_name}",
        logs_source_dir=text_multifile.logs_path,
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
    run_and_log_subprocess(compression_cmd)

    decompression_cmd = [bin_path, "x", compression_path, decompression_path]
    run_and_log_subprocess(decompression_cmd)

    input_path = test_paths.logs_source_dir
    output_path = test_paths.decompression_dir
    assert is_dir_tree_content_equal(
        input_path,
        output_path,
    ), f"Mismatch between clp input {input_path} and output {output_path}."

    test_paths.clear_test_outputs()


@pytest.mark.clp_s
def test_clp_s_identity_transform(
    clp_core_path_config: ClpCorePathConfig,
    integration_test_path_config: IntegrationTestPathConfig,
    json_multifile: IntegrationTestDataset,
) -> None:
    """
    Validate that compression and decompression by the core binary `clp-s` run successfully and are
    lossless.

    :param clp_core_path_config:
    :param integration_test_path_config:
    :param json_multifile:
    """
    dataset_name = json_multifile.dataset_name

    test_paths = CompressionTestPathConfig(
        test_name=f"clp-s-{dataset_name}",
        logs_source_dir=json_multifile.logs_path,
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
        test_name=f"clp-s-{dataset_name}-consolidated-json",
        logs_source_dir=test_paths.decompression_dir,
        integration_test_path_config=integration_test_path_config,
    )
    _clp_s_compress_and_decompress(clp_core_path_config, consolidated_json_test_paths)

    _consolidated_json_file_name = "original"
    input_path = consolidated_json_test_paths.logs_source_dir / _consolidated_json_file_name
    output_path = consolidated_json_test_paths.decompression_dir / _consolidated_json_file_name
    assert is_json_file_structurally_equal(input_path, output_path), (
        f"Mismatch between clp-s input {input_path} and output {output_path}."
    )

    test_paths.clear_test_outputs()
    consolidated_json_test_paths.clear_test_outputs()


def _clp_s_compress_and_decompress(
    clp_core_path_config: ClpCorePathConfig,
    test_paths: CompressionTestPathConfig,
) -> None:
    test_paths.clear_test_outputs()
    bin_path = str(clp_core_path_config.clp_s_binary_path)
    src_path = str(test_paths.logs_source_dir)
    compression_path = str(test_paths.compression_dir)
    decompression_path = str(test_paths.decompression_dir)
    run_and_log_subprocess([bin_path, "c", compression_path, src_path])
    run_and_log_subprocess([bin_path, "x", compression_path, decompression_path])

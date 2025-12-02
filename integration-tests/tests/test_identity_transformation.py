"""
Integration tests verifying that CLP core compression binaries perform lossless round-trip
compression and decompression.
"""

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
)

pytestmark = pytest.mark.core

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
    run_and_assert([bin_path, "c", compression_path, src_path])
    run_and_assert([bin_path, "x", compression_path, decompression_path])

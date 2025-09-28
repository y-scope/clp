"""
Integration tests verifying that CLP core compression binaries perform lossless round-trip
compression and decompression.
"""

from pathlib import Path

import pytest

from tests.utils.asserting_utils import run_and_assert
from tests.utils.config import (
    CompressionTestConfig,
    IntegrationTestConfig,
    IntegrationTestLogs,
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
    integration_test_config: IntegrationTestConfig,
    test_logs_fixture: str,
) -> None:
    """
    Validate that compression and decompression by the core binary `clp` run successfully and are
    lossless across various input archive formats.

    :param request:
    :param integration_test_config:
    :param test_logs_fixture:
    """
    integration_test_logs: IntegrationTestLogs = request.getfixturevalue(test_logs_fixture)
    logs_source_dir: Path = integration_test_logs.extraction_dir

    archives_to_test = [
        integration_test_logs.extraction_dir,
        integration_test_logs.tar_gz_path,
        integration_test_logs.tar_lz4_path,
        integration_test_logs.tar_xz_path,
        integration_test_logs.tar_zstd_path,
    ]
    for archive_path in archives_to_test:
        _test_clp_identity_transform_single_archive(
            archive_path, integration_test_config, logs_source_dir
        )


def _test_clp_identity_transform_single_archive(
    compression_input: Path,
    integration_test_config: IntegrationTestConfig,
    logs_source_dir: Path,
) -> None:
    """
    Validate that compression and decompression by the core binary `clp` run successfully and are
    lossless for a single archive input format.

    :param compression_input: Path to the archive for compression.
    :param integration_test_config: General config for the integration tests.
    :param logs_source_dir: Path to the uncompressed logs for comparison.
    """
    test_paths = CompressionTestConfig(
        test_name=f"clp-{compression_input.name}",
        compression_input=compression_input,
        integration_test_config=integration_test_config,
    )
    test_paths.clear_test_outputs()

    bin_path = str(integration_test_config.core_config.clp_binary_path)
    input_path = str(test_paths.compression_input)
    compression_path = str(test_paths.compression_dir)
    decompression_path = str(test_paths.decompression_dir)
    path_prefix_to_remove = (
        input_path
        if test_paths.compression_input.is_dir()
        else str(test_paths.compression_input.parent)
    )

    # fmt: off
    compression_cmd = [
        bin_path,
        "c",
        "--progress",
        "--remove-path-prefix", path_prefix_to_remove,
        compression_path,
        input_path,
    ]
    # fmt: on
    run_and_assert(compression_cmd)

    decompression_cmd = [bin_path, "x", compression_path, decompression_path]
    run_and_assert(decompression_cmd)

    decompressed_logs_path = test_paths.decompression_dir
    assert is_dir_tree_content_equal(
        logs_source_dir,
        decompressed_logs_path,
    ), f"Mismatch between source {logs_source_dir} and `clp` final output {decompressed_logs_path}."
    test_paths.clear_test_outputs()


@pytest.mark.clp_s
@json_datasets
def test_clp_s_identity_transform(
    request: pytest.FixtureRequest,
    integration_test_config: IntegrationTestConfig,
    test_logs_fixture: str,
) -> None:
    """
    Validate that compression and decompression by the core binary `clp-s` run successfully and are
    lossless.

    :param request:
    :param integration_test_config:
    :param test_logs_fixture:
    """
    integration_test_logs: IntegrationTestLogs = request.getfixturevalue(test_logs_fixture)
    test_logs_name = integration_test_logs.name

    test_paths = CompressionTestConfig(
        test_name=f"clp-s-{test_logs_name}",
        compression_input=integration_test_logs.extraction_dir,
        integration_test_config=integration_test_config,
    )
    _clp_s_compress_and_decompress(integration_test_config, test_paths)

    # Recompress the decompressed output that's consolidated into a single json file, and decompress
    # it again to verify consistency. The compression input of the second iteration points to the
    # decompression output of the first.
    # TODO: Remove this check once we can directly compare decompressed logs (which would preserve
    #       the directory structure and row/key order) with the original downloaded logs.
    # See also: https://docs.yscope.com/clp/main/user-guide/core-clp-s.html#current-limitations
    consolidated_json_test_paths = CompressionTestConfig(
        test_name=f"clp-s-{test_logs_name}-consolidated-json",
        compression_input=test_paths.decompression_dir,
        integration_test_config=integration_test_config,
    )
    _clp_s_compress_and_decompress(integration_test_config, consolidated_json_test_paths)

    _consolidated_json_file_name = "original"
    input_path = consolidated_json_test_paths.compression_input / _consolidated_json_file_name
    output_path = consolidated_json_test_paths.decompression_dir / _consolidated_json_file_name
    assert is_json_file_structurally_equal(input_path, output_path), (
        f"Mismatch between clp-s input {input_path} and output {output_path}."
    )

    test_paths.clear_test_outputs()
    consolidated_json_test_paths.clear_test_outputs()


def _clp_s_compress_and_decompress(
    integration_test_config: IntegrationTestConfig, test_paths: CompressionTestConfig
) -> None:
    test_paths.clear_test_outputs()
    bin_path = str(integration_test_config.core_config.clp_s_binary_path)
    src_path = str(test_paths.compression_input)
    compression_path = str(test_paths.compression_dir)
    decompression_path = str(test_paths.decompression_dir)
    run_and_assert([bin_path, "c", compression_path, src_path])
    run_and_assert([bin_path, "x", compression_path, decompression_path])

"""Integration tests verifying the correctness the CLP core python binding library."""

import pytest

from tests.utils.asserting_utils import run_and_assert
from tests.utils.config import (
    ClpCorePathConfig,
    CompressionTestPathConfig,
    IntegrationTestLogs,
    IntegrationTestPathConfig,
)
from tests.utils.utils import is_json_file_structurally_equal
from yscope_clp_core import open_archive

pytestmark = pytest.mark.yscope_clp_core


def test_archive_writer(
    clp_core_path_config: ClpCorePathConfig,
    postgresql: IntegrationTestLogs,
    integration_test_path_config: IntegrationTestPathConfig,
) -> None:
    """
    Test the archive writer mode of yscope_clp_core.open_archive.

    This is a lightweight sanity test intended only to verify that yscope_clp_core can be installed
    into python envrionment and that its basic functionality works. This test is coarsly written and
    will be refined in future updates.
    Right now, we simply checks that an archive produced by the writer can be successfully
    decompressed by clp-s with structurally identical output.

    :param clp_core_path_config:
    :param postgresql:
    :param integration_test_path_config:
    """
    test_paths = CompressionTestPathConfig(
        test_name="yscope-clp-core-archive-writer",
        logs_source_dir=postgresql.extraction_dir,
        integration_test_path_config=integration_test_path_config,
    )

    with open_archive(test_paths.compression_dir, mode="w") as writer:
        writer.add(test_paths.logs_source_dir)

    decompress_cmd = [
        str(clp_core_path_config.clp_s_binary_path),
        "x",
        str(test_paths.compression_dir),
        str(test_paths.decompression_dir),
    ]
    run_and_assert(decompress_cmd)

    input_path = postgresql.extraction_dir / "postgresql" / "postgresql.log"
    output_path = test_paths.decompression_dir / "original"
    assert is_json_file_structurally_equal(input_path, output_path), (
        f"Mismatch between clp-s input {input_path} and output {output_path}."
    )

    test_paths.clear_test_outputs()

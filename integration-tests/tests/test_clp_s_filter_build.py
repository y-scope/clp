"""Integration tests verifying clp-s builds variable dictionary filters."""

from pathlib import Path

import pytest

from tests.utils.asserting_utils import run_and_assert
from tests.utils.config import (
    ClpCorePathConfig,
    CompressionTestPathConfig,
    IntegrationTestLogs,
    IntegrationTestPathConfig,
)
from tests.utils.utils import unlink

pytestmark = pytest.mark.core

json_datasets = pytest.mark.parametrize(
    "test_logs_fixture",
    [
        "postgresql",
    ],
)


@pytest.mark.clp_s
@json_datasets
def test_clp_s_builds_var_dict_filters(
    request: pytest.FixtureRequest,
    clp_core_path_config: ClpCorePathConfig,
    integration_test_path_config: IntegrationTestPathConfig,
    test_logs_fixture: str,
) -> None:
    """
    Validate that clp-s builds variable dictionary filters when enabled.

    :param request:
    :param clp_core_path_config:
    :param integration_test_path_config:
    :param test_logs_fixture:
    """
    integration_test_logs: IntegrationTestLogs = request.getfixturevalue(test_logs_fixture)

    test_paths = CompressionTestPathConfig(
        test_name=f"clp-s-{integration_test_logs.name}-filters",
        logs_source_dir=integration_test_logs.extraction_dir,
        integration_test_path_config=integration_test_path_config,
    )
    test_paths.clear_test_outputs()

    filters_dir = (
        integration_test_path_config.test_root_dir
        / f"clp-s-{integration_test_logs.name}-filters-output"
    )
    unlink(filters_dir)
    filters_dir.mkdir(parents=True, exist_ok=True)

    bin_path = str(clp_core_path_config.clp_s_binary_path)
    src_path = str(test_paths.logs_source_dir)
    compression_path = str(test_paths.compression_dir)

    # fmt: off
    compression_cmd = [
        bin_path,
        "c",
        "--var-filter-type", "bloom",
        "--var-filter-fpr", "0.001",
        "--var-filter-output-dir", str(filters_dir),
        compression_path,
        src_path,
    ]
    # fmt: on
    run_and_assert(compression_cmd)

    archive_dirs = [p for p in Path(compression_path).iterdir() if p.is_dir()]
    assert archive_dirs, f"No archives created in {compression_path}"

    for archive_dir in archive_dirs:
        expected_filter = filters_dir / f"{archive_dir.name}.var.dict.filter"
        assert expected_filter.is_file(), f"Missing filter: {expected_filter}"

    unlink(filters_dir)
    test_paths.clear_test_outputs()

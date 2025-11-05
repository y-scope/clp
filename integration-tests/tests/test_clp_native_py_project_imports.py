"""Smoke test ensuring CLP native Python projects import without errors."""

from clp_mcp_server.constants import QueryJobType
from clp_package_utils.general import JobType
from clp_py_utils.clp_config import StorageEngine
from job_orchestration.scheduler.constants import CompressionJobStatus


def test_clp_native_py_project_enum_classes() -> None:
    """
    Verifies that the following CLP native Python projects import successfully:
      - `clp-mcp-server`
      - `clp-package-utils`
      - `clp-py-utils`
      - `job-orchestration`
    by testing conversions between their representative enum classes and literal values.
    """
    assert QueryJobType.SEARCH_OR_AGGREGATION == QueryJobType(0)
    assert JobType.COMPRESSION == JobType("compression")
    assert StorageEngine.CLP == StorageEngine("clp")
    assert CompressionJobStatus.PENDING == CompressionJobStatus(0)

"""Smoke tests to validate that CLP Python projects can be imported without errors."""

from pathlib import Path

from clp_mcp_server.constants import QueryJobType
from clp_package_utils.general import JobType
from clp_py_utils.clp_config import StorageEngine
from clp_py_utils.core import FileMetadata
from job_orchestration.scheduler.constants import CompressionJobStatus


def test_clp_native_py_project_enum_classes() -> None:
    """
    Verifies that the following CLP Python projects can be imported successfully by testing
    conversions between their representative enum classes and literal values:

    - clp-mcp-server
    - clp-package-utils
    - clp-py-utils
    - job-orchestration
    """
    assert QueryJobType.SEARCH_OR_AGGREGATION == QueryJobType(0)
    assert JobType.COMPRESSION == JobType("compression")
    assert StorageEngine.CLP == StorageEngine("clp")
    assert CompressionJobStatus.PENDING == CompressionJobStatus(0)


def test_file_metadata_estimates_zstandard_file_sizes() -> None:
    """Tests case-insensitive recognition of the standard Zstandard file extension."""
    file_size = 100

    for path in [
        Path("app.log.zst"),
        Path("app.log.clp.zst"),
        Path("app.log.tar.zst"),
        Path("app.log.ZST"),
    ]:
        assert file_size * 8 == FileMetadata(path, file_size).estimated_uncompressed_size


def test_file_metadata_estimates_gzip_file_sizes() -> None:
    """Tests case-insensitive recognition of the supported gzip file extensions."""
    file_size = 100

    for path in [
        Path("app.log.gz"),
        Path("app.log.GZ"),
        Path("app.log.gzip"),
        Path("app.log.GZIP"),
        Path("app.log.tgz"),
        Path("app.log.TGZ"),
        Path("app.log.tar.gz"),
        Path("app.log.TAR.GZ"),
    ]:
        assert file_size * 13 == FileMetadata(path, file_size).estimated_uncompressed_size

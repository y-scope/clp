"""Integration tests verifying that CLP package startup and tear down processes are successful."""
# TODO: poluate this file with actual tests and leverage `clp_py_utils`.

import pytest
from clp_py_utils.clp_config import StorageEngine

pytestmark = pytest.mark.package


def test_clp_py_utils_import() -> None:
    """Tests that `clp_py_utils` is successfully imported."""
    assert StorageEngine.CLP == StorageEngine("clp")

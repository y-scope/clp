"""Integration tests for the CLP core python binding library."""

import inspect

from yscope_clp_core import clp_s


def test_docstring_access() -> None:
    """Verify that `yscope_clp_core` is accessible by inspecting its API docstring."""
    doc = inspect.getdoc(clp_s)
    assert doc is not None

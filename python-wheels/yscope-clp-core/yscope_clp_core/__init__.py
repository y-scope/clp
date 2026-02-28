"""Top-level namespace for the yscope_clp_core package."""

from yscope_clp_core._api import open_archive
from yscope_clp_core._bin import clp_s

__all__ = [
    "clp_s",
    "open_archive",
]

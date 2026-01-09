"""Public entry points for the clp_core Python package."""

from clp_core._api import compress, decompress, search
from clp_core._bin import clp_s
from clp_core._except import (
    BadArchiveSourceError,
    BadCompressionInputError,
    ClpCoreError,
    ClpCoreRuntimeError,
)
from clp_core._open_archive import open_archive

__all__ = [
    "BadArchiveSourceError",
    "BadCompressionInputError",
    "ClpCoreError",
    "ClpCoreRuntimeError",
    "clp_s",
    "compress",
    "decompress",
    "open_archive",
    "search",
]

"""Public entry points for the clp_core Python package."""

from clp_core._api import compress, decompress, search
from clp_core._bin import clp_s
from clp_core._except import BadCompressionInputError, ClpCoreError, ClpCoreRuntimeError

__all__ = [
    "BadCompressionInputError",
    "ClpCoreError",
    "ClpCoreRuntimeError",
    "clp_s",
    "compress",
    "decompress",
    "search",
]

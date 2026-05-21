"""Public entry points for the yscope_clp_core Python package."""

from yscope_clp_core._api import (
    CLP_SFA_MAGIC_BYTES,
    is_clp_json_single_file_archive,
    open_archive,
    search_archive,
)
from yscope_clp_core._archive_io import (
    ClpArchiveReader,
    ClpArchiveWriter,
    ClpSearchResults,
)
from yscope_clp_core._bin import clp_s
from yscope_clp_core._config import (
    ClpQuery,
    CompressionKwargs,
    DecompressionKwargs,
    KqlQuery,
    LogEvent,
    SearchKwargs,
)
from yscope_clp_core._except import (
    ArchiveClosedError,
    BadArchiveSourceError,
    BadCompressionInputError,
    ClpCoreError,
    ClpCoreRuntimeError,
)

__all__ = [
    "CLP_SFA_MAGIC_BYTES",
    "ArchiveClosedError",
    "BadArchiveSourceError",
    "BadCompressionInputError",
    "ClpArchiveReader",
    "ClpArchiveWriter",
    "ClpCoreError",
    "ClpCoreRuntimeError",
    "ClpQuery",
    "ClpSearchResults",
    "CompressionKwargs",
    "DecompressionKwargs",
    "KqlQuery",
    "LogEvent",
    "SearchKwargs",
    "clp_s",
    "is_clp_json_single_file_archive",
    "open_archive",
    "search_archive",
]

"""Public entry points for the clp_core Python package."""

from clp_core._api import (
    open_archive,
    search_archive,
)
from clp_core._archive_io import (
    ClpArchiveReader,
    ClpArchiveWriter,
    ClpSearchResults,
)
from clp_core._bin import clp_s
from clp_core._config import (
    ClpQuery,
    CompressionKwargs,
    DecompressionKwargs,
    KqlQuery,
    LogEvent,
    SearchKwargs,
)
from clp_core._except import (
    BadArchiveSourceError,
    BadCompressionInputError,
    ClpCoreError,
    ClpCoreRuntimeError,
)

__all__ = [
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
    "open_archive",
    "search_archive",
]

"""Public APIs for the clp_core python package."""

from typing import (
    Any,
    Literal,
    overload,
)

from typing_extensions import Unpack

from clp_core._archive_io import (
    ClpArchiveReader,
    ClpArchiveWriter,
    ClpSearchResults,
)
from clp_core._config import (
    ArchiveInputSource,
    ClpQuery,
    CompressionKwargs,
    DecompressionKwargs,
    SearchKwargs,
)
from clp_core._except import ClpCoreRuntimeError


@overload
def open_archive(
    file: ArchiveInputSource,
    mode: Literal["wb"],
    **kwargs: Unpack[CompressionKwargs],
) -> ClpArchiveWriter: ...


@overload
def open_archive(
    file: ArchiveInputSource,
    mode: Literal["r"],
    **kwargs: Unpack[DecompressionKwargs],
) -> ClpArchiveReader: ...


def open_archive(
    file: ArchiveInputSource,
    mode: Literal["r", "wb"] = "r",
    **kwargs: Any,
) -> ClpArchiveWriter | ClpArchiveReader:
    """
    Open a CLP archive for reading or writing.

    This function returns an archive handler that provides access to the underlying CLP archive.
    The exact handler type and supported options depend on the selected open mode.

    Temporary files or directories may be created during archive operations. Call close() on the
    returned object or use a with statement to ensure proper cleanup.

    :param file: Archive sink or source. Must be either a filesystem path or a binary I/O object.
        When a binary stream is provided, seeking must be performed before creating the archive
        handler. The stream is then used directly without further seek operations.
    :param mode: Archive open mode. Use "wb" to create a new archive or overwrite an existing one,
        or "r" to open an existing archive for reading.
    :param kwargs: Mode dependent keyword arguments for archive handler operations. For "wb", see
        `CompressionKwargs`. For "r", see `DecompressionKwargs`.
    :return: An archive handler instance. For "wb", returns a `ClpArchiveWriter`, which controls how
        and which log data is written to the archive. For "r", returns a `ClpArchiveReader`, which
        allows iteration over log events in their original order.
    :raise BadArchiveSourceError: If the archive source type is not supported.
    :raise ClpCoreRuntimeError: If the archive handler constructor failed or the archive open mode
        is unsupported.
    """
    if mode == "r":
        return ClpArchiveReader(file, **kwargs)
    if mode == "wb":
        return ClpArchiveWriter(file, **kwargs)
    err_msg = f"Unsupported archive open mode `{mode}`."
    raise ClpCoreRuntimeError(err_msg)


def search_archive(
    file: ArchiveInputSource,
    query: ClpQuery,
    **kwargs: Unpack[SearchKwargs],
) -> ClpSearchResults:
    """
    Execute a search query against a CLP archive.

    Temporary files or directories may be created during archive operations. Call close() on the
    returned object or use a with statement to ensure proper cleanup.

    :param file: Archive source. Must be either a filesystem path or a binary I/O object. When a
        binary stream is provided, seeking must be performed before creating the archive handler.
        The stream is then used directly without further seek operations.
    :param query: Search query to evaluate against the archive. For supported query types, see
        `ClpQuery`.
    :param kwargs: Search specific keyword arguments controlling query execution and result
        behavior. See `SearchKwargs` for supported options.
    :return: A `ClpSearchResults` handler providing an iterable view of query-matching log events.
    :raise BadArchiveSourceError: If the archive source type is not supported.
    :raise ClpCoreRuntimeError: If the archive handler constructor failed.
    """
    return ClpSearchResults(file, query, **kwargs)

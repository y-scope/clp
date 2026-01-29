"""Core public interfact containing functions intended for external use."""

from typing import Literal

from typing_extensions import Unpack

from yscope_clp_core._archive_io import ClpArchiveWriter
from yscope_clp_core._except import ClpCoreRuntimeError
from yscope_clp_core._types import ArchiveSource, CompressionKwargs


def open_archive(
    file: ArchiveSource,
    mode: Literal["w"],
    **kwargs: Unpack[CompressionKwargs],
) -> ClpArchiveWriter:
    """
    Open a CLP archive for writing.

    This function returns an archive handler that provides access to the underlying CLP archive.
    The exact handler type and supported options depend on the selected open mode.

    Temporary files or directories may be created during archive operations. Call close() on the
    returned object or use a with statement to ensure proper cleanup.

    :param file: Archive source. Must be either a filesystem path or a binary IO stream. If a stream
        is provided, any necessary seeking must be performed before creating the archive handler.
        The stream is then used directly without further seeking opeartions.
    :param mode: Archive open mode. For "w", create a new archive or overwrite an existing one.
    :param kwargs: Mode dependent keyword arguments for archive handler operations. For "w", see
        `CompressionKwargs`.
    :return: An archive handler instance. For "w", returns a `ClpArchiveWriter` that controls how
        and which log data is written to the archive.
    :raise ClpCoreRuntimeError: If the archive handler construction fails or the archive open mode
        is unsupported.
    """
    if mode != "w":
        err_msg = f"Unsupported archive open mode `{mode}`. Please choose from [`w`]."
        raise ClpCoreRuntimeError(err_msg)
    return ClpArchiveWriter(file, **kwargs)

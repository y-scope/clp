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
    CompressionKwargs,
    DecompressionKwargs,
    SearchKwargs,
)


@overload
def open_archive(
    file: ArchiveInputSource,
    mode: Literal["w"],
    **kwargs: Unpack[CompressionKwargs],
) -> ClpArchiveWriter:
    pass


@overload
def open_archive(
    file: ArchiveInputSource,
    mode: Literal["r"],
    **kwargs: Unpack[DecompressionKwargs],
) -> ClpArchiveReader:
    pass


def open_archive(
    file: ArchiveInputSource,
    mode: Literal["r", "w"] = "r",
    **kwargs: Any,
) -> ClpArchiveWriter | ClpArchiveReader:
    if "w" == mode:
        return ClpArchiveWriter(file, **kwargs)
    return ClpArchiveReader(file, **kwargs)


def search_archive(
    file: ArchiveInputSource,
    **kwargs: Unpack[SearchKwargs],
) -> ClpSearchResults:
    return ClpSearchResults(file, **kwargs)

from typing import (
    Any,
    Literal,
    overload,
)

from typing_extensions import Unpack

from clp_core._archive_io import (
    ClpArchiveReader,
    ClpArchiveWriter,
)
from clp_core._config import (
    CompressionKwargs,
    DecompressionKwargs,
    InputSource,
)


@overload
def open_archive(
    file: InputSource,
    mode: Literal["w"],
    **kwargs: Unpack[CompressionKwargs],
) -> ClpArchiveWriter:
    pass


@overload
def open_archive(
    file: InputSource,
    mode: Literal["r"],
    **kwargs: Unpack[DecompressionKwargs],
) -> ClpArchiveReader:
    pass


def open_archive(
    file: InputSource,
    mode: Literal["r", "w"] = "r",
    **kwargs: Any,
) -> ClpArchiveWriter | ClpArchiveReader:
    if "w" == mode:
        return ClpArchiveWriter(file, **kwargs)
    return ClpArchiveReader(file, **kwargs)

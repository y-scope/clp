import io
import os
from typing import Annotated, Any, cast, Final, IO, TypedDict

from pydantic import AfterValidator, ConfigDict, TypeAdapter

from yscope_clp_core._except import ClpCoreTypeError


class ArchiveSourceValidator:
    @staticmethod
    def validate(value: Any) -> str | os.PathLike[str] | IO[bytes]:
        """
        Type check the given archive source for a supported type.

        :param value: Archive source.
        :raise ClpCoreTypeError: If the archive source type is not supported.
        """
        if isinstance(value, (str, os.PathLike)):
            return value
        if isinstance(value, io.IOBase):
            if isinstance(value, io.TextIOBase):
                err_msg = "Archive source is a stream but it is in the unsupported text format."
                raise ClpCoreTypeError(err_msg)
            return cast("IO[bytes]", value)
        err_msg = "Expected archive source of type str | os.PathLike | IO[bytes]."
        raise ClpCoreTypeError(err_msg)


ArchiveSource = Annotated[
    str | os.PathLike[str] | IO[bytes],
    AfterValidator(ArchiveSourceValidator.validate),
]

ArchiveSourceAdapter: Final[TypeAdapter[ArchiveSource]] = TypeAdapter(
    ArchiveSource,
    config=ConfigDict(arbitrary_types_allowed=True),
)


class CompressionKwargs(TypedDict, total=False):
    """
    Keyword arguments use by `ClpArchiveWriter` to control archive compression behavior.
    :param compression_level: An integer from 1 to 19 controlling the level of compression; `1` is
        fastest and produces the least compression, and `19` is the slowest and produces the most
        compression. The default is `3`.
    :param timestamp_key: Path (e.g. x.y) for the field containing the log event's timestamp. If
        None, no timestamp field is assumed.
    """

    compression_level: int
    timestamp_key: str | None

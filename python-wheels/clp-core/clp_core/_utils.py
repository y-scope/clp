"""Utilities used by the clp_core Python package."""

import io
import os
import shutil
import tempfile
from pathlib import Path
from typing import BinaryIO, TextIO

from clp_core._except import (
    BadArchiveSourceError,
    ClpCoreRuntimeError,
)


def _validate_archive_source(src: object) -> None:
    """
    Type check the given archive source for a supported type.

    TODO: Consider replacing this with a Pydantic field validator for runtime validation.

    :param src:
    :raise BadArchiveSourceError: If the source is not a str, os.PathLike, or BinaryIO.
    """
    if isinstance(src, (str, os.PathLike)):
        return
    if isinstance(src, io.IOBase) and not isinstance(src, io.TextIOBase):
        return
    err_msg = "Expected archive source of type str | os.PathLike | BinaryIO."
    raise BadArchiveSourceError(err_msg)


def _get_single_file_in_dir(dir_path: Path) -> Path:
    """
    Validate that a directory contains exactly one regular file and return its path.

    This helper is useful for determining the exact path of compression and decompression outputs,
    whether they are a single file archive or a single JSON Lines output.
    :param dir_path:
    :return: Path to the single file contained in the directory.
    :raise ClpCoreRuntimeError:
    """
    if not dir_path.is_dir():
        err_msg = f"{dir_path} is not a directory."
        raise ClpCoreRuntimeError(err_msg)

    files = list(dir_path.iterdir())
    if len(files) != 1 or not files[0].is_file():
        err_msg = f"Expected exactly one file in {dir_path}."
        raise ClpCoreRuntimeError(err_msg)

    return files[0]


def _write_stream_to_temp_file(
    stream: BinaryIO | TextIO,
    suffix: str | None = None,
) -> str:
    """
    Write the contents of a readable text or binary stream to a temporary file and return its path.

    This helper materializes an input stream into a temporary file on disk for utilities that
    require a filesystem path. The caller assumes responsibility for deleting the file when it is no
    longer required. On write failure, the file is closed and removed before propagating the error.
    :param stream: Readable text or binary stream whose contents will be written to disk.
    :param suffix: Optional filename suffix for the temporary file.
    :return: Path string to the temporary file containing the stream contents.
    :raise ClpCoreRuntimeError: If an error occurs while writing the stream to disk.
    """
    if not stream.readable():
        err_msg = "Input stream is not readable."
        raise ClpCoreRuntimeError(err_msg)

    is_text = isinstance(stream, io.TextIOBase)
    mode = "w" if is_text else "wb"
    encoding = getattr(stream, "encoding", None)
    errors = getattr(stream, "errors", None)

    # Context manager for temporary files is used in parent function.
    temp_file = tempfile.NamedTemporaryFile(  # noqa: SIM115
        mode=mode,
        encoding=encoding,
        errors=errors,
        delete=False,
        suffix=suffix,
    )

    temp_file_path_str = temp_file.name
    try:
        shutil.copyfileobj(stream, temp_file, length=1024 * 1024)
    except Exception as e:
        temp_file.close()
        Path(temp_file_path_str).unlink(missing_ok=True)
        err_msg = f"Failed to write stream to be compressed at `{temp_file_path_str}`."
        raise ClpCoreRuntimeError(err_msg) from e
    else:
        return temp_file_path_str
    finally:
        temp_file.close()

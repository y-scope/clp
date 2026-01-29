"""Shared package utilities."""

import io
import shutil
import tempfile
from pathlib import Path
from typing import IO

from yscope_clp_core._constants import FILE_OBJ_COPY_CHUNK_SIZE
from yscope_clp_core._except import ClpCoreRuntimeError


def _get_single_file_in_dir(dir_path: Path) -> Path:
    """
    Validate that a directory contains exactly one regular file and return its path.

    Use this helper to resolve the output path when a process produces an anonymous single-file
    result, such as:
        * Compression that outputs a single file archive.
        * Decompression that outputs a single JSON Lines output.
    :param dir_path:
    :return: Path to the single file contained in the directory.
    :raise ClpCoreRuntimeError: If the supplied path is not a directory or does not contain exactly
        one regular file.
    """
    if not dir_path.is_dir():
        err_msg = f"{dir_path} is not a directory."
        raise ClpCoreRuntimeError(err_msg)

    items = list(dir_path.iterdir())

    if len(items) != 1:
        err_msg = f"Expected exactly one file in {dir_path}, but found {len(items)} items."
        raise ClpCoreRuntimeError(err_msg)

    if not items[0].is_file():
        err_msg = f"Expected a regular file in {dir_path}, but the item is not a file."
        raise ClpCoreRuntimeError(err_msg)

    return items[0]


def _write_stream_to_temp_file(
    stream: IO[bytes] | IO[str],
    suffix: str | None = None,
) -> Path:
    """
    Write the contents of a readable text or binary stream to a temporary file and return its path.

    This helper materializes an input stream into a temporary file on disk for utilities that
    require a filesystem path. The caller assumes responsibility for deleting the file when it is no
    longer required. On write failure, the file is closed and removed before raising an exception.
    :param stream: Readable text or binary stream whose contents will be written to disk.
    :param suffix: Optional filename suffix for the temporary file.
    :return: Path to the temporary file containing the stream contents.
    :raise ClpCoreRuntimeError: If the input stream is not readable, or an error occurs while
        writing the stream to disk.
    """
    if not stream.readable():
        err_msg = "Input stream is not readable."
        raise ClpCoreRuntimeError(err_msg)

    is_text = isinstance(stream, io.TextIOBase)
    mode = "w" if is_text else "wb"
    encoding = getattr(stream, "encoding", None)
    errors = getattr(stream, "errors", None)

    temp_file_path: Path | None
    try:
        with tempfile.NamedTemporaryFile(
            mode=mode,
            encoding=encoding,
            errors=errors,
            delete=False,
            suffix=suffix,
        ) as temp_file:
            temp_file_path = Path(temp_file.name)
            shutil.copyfileobj(stream, temp_file, length=FILE_OBJ_COPY_CHUNK_SIZE)
            temp_file.flush()
    except Exception as e:
        if temp_file_path is not None:
            temp_file_path.unlink(missing_ok=True)
            err_msg = f"Failed to write input stream to temporary file at '{temp_file_path}'."
        else:
            err_msg = "Could not initialize temporary file for writing input stream."
        raise ClpCoreRuntimeError(err_msg) from e

    return temp_file_path

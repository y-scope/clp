"""Public APIs for the clp_core python package."""

import io
import os
import subprocess
import tempfile
from collections.abc import Iterator
from contextlib import contextmanager, suppress
from pathlib import Path

from clp_core._bin import _get_executable
from clp_core._except import BadCompressionInputError, ClpCoreRuntimeError

CLP_S_BIN = _get_executable("clp-s")


def _unlink(file_path: str | Path) -> None:
    with suppress(Exception):
        Path(file_path).unlink(missing_ok=True)


def _write_stream_to_temp_file(stream: io.IOBase) -> str:
    if not stream.readable():
        err_msg = "Compression input stream is not readable."
        raise BadCompressionInputError(err_msg)

    is_text = isinstance(stream, io.TextIOBase)
    mode = "w" if is_text else "wb"
    encoding = "utf-8" if is_text else None

    # Context manager for temporary files is used in parent function.
    tmp_file = tempfile.NamedTemporaryFile(  # noqa: SIM115
        mode=mode,
        encoding=encoding,
        delete=False,
        suffix=".jsonl",
    )
    tmp_file_path_str = tmp_file.name
    try:
        tmp_file.write(stream.read())
    except Exception as e:
        _unlink(tmp_file_path_str)
        err_msg = f"Cannot write compression input to temporary file at `{tmp_file_path_str}`."
        raise ClpCoreRuntimeError(err_msg) from e
    else:
        return tmp_file_path_str
    finally:
        tmp_file.close()


@contextmanager
def _input_source_path_ctx(input_source: os.PathLike[str] | io.IOBase | str) -> Iterator[str]:
    if isinstance(input_source, os.PathLike):
        yield os.fspath(input_source)
        return

    if isinstance(input_source, io.IOBase):
        tmp_path: str | None = None
        try:
            tmp_path = _write_stream_to_temp_file(input_source)
            yield tmp_path
        finally:
            if tmp_path is not None:
                _unlink(tmp_path)
        return

    if isinstance(input_source, str):
        yield input_source
        return

    err_msg = "Unsupported compression input type."
    raise BadCompressionInputError(err_msg)


def compress(input_source: os.PathLike[str] | io.IOBase | str, archive_path: str | Path) -> None:
    input_file_path = _input_source_path_ctx(input_source)
    compress_cmd = [str(CLP_S_BIN), "c", str(archive_path), str(input_file_path)]

    try:
        subprocess.run(compress_cmd, check=True)
    except Exception as e:
        err_msg = "Compression failed."
        raise ClpCoreRuntimeError(err_msg) from e


def decompress() -> None:
    pass


def search() -> None:
    pass

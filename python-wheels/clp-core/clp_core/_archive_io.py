import io
import os
import shutil
import subprocess
from contextlib import AbstractContextManager
from pathlib import Path
from tempfile import TemporaryDirectory
from types import TracebackType
from typing import BinaryIO, cast

from typing_extensions import Unpack

from clp_core._bin import _get_executable
from clp_core._config import (
    CompressionKwargs,
    DecompressionKwargs,
    InputSource,
)
from clp_core._except import (
    BadCompressionInputError,
    ClpCoreRuntimeError,
)
from clp_core._utils import (
    _get_single_file_in_dir,
    _validate_archive_source,
    _write_stream_to_temp_file,
)

CLP_S_BIN = _get_executable("clp-s")


class ClpArchiveWriter(AbstractContextManager["ClpArchiveWriter", None]):
    def __init__(self, file: InputSource, **kwargs: Unpack[CompressionKwargs]) -> None:
        self._file: InputSource = file
        self._compression_level: int | None = kwargs.get("compression_level")
        self._timestamp_key: str | None = kwargs.get("timestamp_key")

        self._files_to_compress: list[Path] = []
        self._temp_files_to_compress: list[Path] = []

        _validate_archive_source(self._file)

        self._archive_dir: Path
        self._temp_archive_dir: TemporaryDirectory[str] | None = None
        if isinstance(file, (str, os.PathLike)):
            self._archive_dir = Path(file)
            try:
                if self._archive_dir.is_dir():
                    shutil.rmtree(self._archive_dir)
                elif self._archive_dir.is_file():
                    self._archive_dir.unlink()
            except Exception as e:
                err_msg = f"Failed to overwrite archive at {self._archive_dir}."
                raise ClpCoreRuntimeError(err_msg) from e
        else:
            # Create a temporary directory for the archive to live in, and feed
            # the archive into the BinaryIO stream.
            self._temp_archive_dir = TemporaryDirectory(dir="/tmp/bill")
            self._archive_dir = Path(self._temp_archive_dir.name)

    def add(self, source: str | os.PathLike[str] | io.IOBase) -> None:
        if isinstance(source, (str, os.PathLike)):
            self._files_to_compress.append(Path(source))
        elif isinstance(source, io.IOBase):
            temp_file_path = Path(_write_stream_to_temp_file(source))
            self._files_to_compress.append(temp_file_path)
            self._temp_files_to_compress.append(temp_file_path)
        else:
            err_msg = f"Invalid compression input type for archive {self._archive_dir}."
            raise BadCompressionInputError(err_msg)

    def _compress(self) -> None:
        if 0 == len(self._files_to_compress):
            err_msg = f"Nothing to compress for archive {self._archive_dir}."
            raise BadCompressionInputError(err_msg)

        compress_cmd = [str(CLP_S_BIN), "c", "--single-file-archive"]
        if self._compression_level is not None:
            compress_cmd.extend(["--compression-level", str(self._compression_level)])
        if self._timestamp_key is not None:
            compress_cmd.extend(["--timestamp-key", self._timestamp_key])
        compress_cmd.append(str(self._archive_dir))
        compress_cmd.extend(str(p) for p in self._files_to_compress)
        subprocess.run(compress_cmd, check=True)

        if self._temp_archive_dir is not None:
            with _get_single_file_in_dir(self._archive_dir).open("rb") as archive_file:
                shutil.copyfileobj(archive_file, cast("BinaryIO", self._file), length=1024 * 1024)

    def close(self) -> None:
        for p in self._temp_files_to_compress:
            p.unlink()
        if self._temp_archive_dir is not None:
            self._temp_archive_dir.cleanup()
        self._temp_archive_dir = None

    def __enter__(self) -> "ClpArchiveWriter":
        return self

    def __exit__(
        self,
        exc_type: type[BaseException] | None,
        exc_val: BaseException | None,
        exc_tb: TracebackType | None,
    ) -> None:
        try:
            if exc_type is None:
                self._compress()
        except Exception as e:
            err_msg = f"Failed to compress archive {self._archive_dir}."
            raise ClpCoreRuntimeError(err_msg) from e
        finally:
            self.close()


class ClpArchiveReader:
    def __init__(self, file: InputSource, **kwargs: Unpack[DecompressionKwargs]) -> None:
        pass

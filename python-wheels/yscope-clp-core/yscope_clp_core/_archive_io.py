"""Classes for handling archive operations including compression, decompression and search."""

import io
import os
import shutil
import subprocess
import sys
from contextlib import AbstractContextManager, suppress
from pathlib import Path
from tempfile import TemporaryDirectory
from types import TracebackType
from typing import cast, IO

from typing_extensions import Unpack

from yscope_clp_core._bin import _get_clp_exe
from yscope_clp_core._constants import FILE_OBJ_COPY_CHUNK_SIZE
from yscope_clp_core._except import ClpCoreRuntimeError, ClpCoreTypeError, ClpCoreValueError
from yscope_clp_core._types import ArchiveSource, ArchiveSourceAdapter, CompressionKwargs
from yscope_clp_core._utils import _get_single_file_in_dir, _write_stream_to_temp_file

if sys.version_info >= (3, 11):
    from typing import Self
else:
    from typing_extensions import Self


class ClpArchiveWriter(AbstractContextManager["ClpArchiveWriter", None]):
    """
    Archive handler for creating and writing to a single CLP archive.

    This class collects one or more compression input sources and produces a single CLP archive.
    Instances should be closed explicitly, as the writer finalizes compression and releases all
    associated resources at that point.
    """

    def __init__(self, file: ArchiveSource, **kwargs: Unpack[CompressionKwargs]) -> None:
        """
        Initialize a `ClpArchiveWriter` instance.

        If the provided archive source is a binary stream, a temporary archive is created on disk,
        and its contents are written to the stream when the writer is closed.
        :param file: Archive source.
        :param kwargs: Compression-specific keyword arguments. See `CompressionKwargs`.
        :raise ClpCoreRuntimeError: if initialization fails.
        :raise: Propagates `ArchiveSourceAdapter`'s exceptions.
        """
        self._file: ArchiveSource = ArchiveSourceAdapter.validate_python(file)
        self._compression_level: int | None = kwargs.get("compression_level")
        self._timestamp_key: str | None = kwargs.get("timestamp_key")

        self._files_to_compress: list[Path] = []
        self._temp_files_to_compress: list[Path] = []

        self._archive_dir: Path
        self._temp_archive_dir: TemporaryDirectory[str] | None = None
        if isinstance(self._file, (str, os.PathLike)):
            self._archive_dir = Path(self._file)
            try:
                if self._archive_dir.is_dir():
                    shutil.rmtree(self._archive_dir)
                elif self._archive_dir.is_file():
                    self._archive_dir.unlink()
            except Exception as e:
                err_msg = f"Failed to overwrite existing archive at {self._archive_dir}."
                raise ClpCoreRuntimeError(err_msg) from e
        else:
            # Create a temporary directory for the archive to live in, and feed the archive into the
            # binary stream when done.
            self._temp_archive_dir = TemporaryDirectory()
            self._archive_dir = Path(self._temp_archive_dir.name)

        self._open: bool = True

    def add(self, source: str | os.PathLike[str] | IO[bytes] | IO[str]) -> None:
        """
        Add an input source to be included in the compressed archive.

        :param source: Input source to add. Must be a filesystem path or an I/O stream. Filesystem
            paths are added directly, while stream inputs are first materialized into temporary
            files, as the CLP compression pipeline currently does not support operating on stdin.
        :raise ClpCoreRuntimeError: If the archive writer is already closed.
        :raise ClpCoreTypeError: If the input source type is not supported.
        :raise: Propagates `_write_stream_to_temp_file`'s exceptions.
        """
        if not self._open:
            err_msg = "Writer is already closed."
            raise ClpCoreRuntimeError(err_msg)

        if isinstance(source, (str, os.PathLike)):
            self._files_to_compress.append(Path(source))
        elif isinstance(source, io.IOBase):
            temp_file_path = _write_stream_to_temp_file(source, suffix=".log")
            self._files_to_compress.append(temp_file_path)
            self._temp_files_to_compress.append(temp_file_path)
        else:
            err_msg = f"Compression input `{source}` has an invalid type."
            raise ClpCoreTypeError(err_msg)

    def compress(self) -> None:
        """
        Perform compression and generate archive. If a binary I/O stream was provided at
        initialization, write the compressed contents to that stream instead.

        This function is automatically invoked when the writer is closed or upon exiting its
        context. If compression has already been performed, this function becomes a no-op.
        :raise ClpCoreRuntimeError: If the archive compression fails.
        :raise ClpCoreRuntimeError: If the compressed data cannot be written to archive stream.
        :raise ClpCoreValueError: If there is nothing to compress.
        """
        if not self._open:
            return

        if 0 == len(self._files_to_compress):
            err_msg = f"Nothing to compress for archive {self._archive_dir}."
            raise ClpCoreValueError(err_msg)

        clp_s_bin_path = _get_clp_exe("clp-s")
        compress_cmd = [str(clp_s_bin_path), "c", "--single-file-archive"]
        if self._compression_level is not None:
            compress_cmd.extend(["--compression-level", str(self._compression_level)])
        if self._timestamp_key is not None:
            compress_cmd.extend(["--timestamp-key", self._timestamp_key])
        compress_cmd.append(str(self._archive_dir))
        compress_cmd.extend(str(p) for p in self._files_to_compress)

        try:
            subprocess.run(compress_cmd, check=True, stderr=subprocess.PIPE, text=True)
        except subprocess.CalledProcessError as e:
            err_msg = f"Compression for archive {self._archive_dir} failed: {e.stderr.strip()}"
            raise ClpCoreRuntimeError(err_msg) from e

        try:
            if self._temp_archive_dir is not None:
                with _get_single_file_in_dir(self._archive_dir).open("rb") as archive_file:
                    shutil.copyfileobj(
                        archive_file, cast("IO[bytes]", self._file), length=FILE_OBJ_COPY_CHUNK_SIZE
                    )
        except Exception as e:
            err_msg = "Failed to write compressed contents to the provided archive stream."
            raise ClpCoreRuntimeError(err_msg) from e

        self._open = False
        self.close()

    def close(self) -> None:
        """
        Close the writer and clean up temporary resources.

        If the archive is still open, this method first performs compression before marking the
        writer as closed and performing cleanup. This method is idempotent and safe to call multiple
        times.
        :raise: Propagates `compress()`'s exceptions.
        """
        if self._open:
            self.compress()

        self._open = False

        for p in self._temp_files_to_compress:
            with suppress(OSError):
                p.unlink()

        if self._temp_archive_dir is not None:
            with suppress(OSError):
                self._temp_archive_dir.cleanup()
            self._temp_archive_dir = None

    def __enter__(self) -> Self:
        return self

    def __exit__(
        self,
        exc_type: type[BaseException] | None,
        exc_val: BaseException | None,
        exc_tb: TracebackType | None,
    ) -> None:
        """
        Finalize archive creation and release associated resources.

        Compression is performed only if no exception was raised while adding inputs. All temporary
        resources are released before returning, regardless of whether compression succeeds.
        :param exc_type:
        :param exc_val:
        :param exc_tb:
        :raise: Propagates `compress()`'s exceptions.
        """
        try:
            if exc_type is None:
                self.compress()
        except Exception as e:
            err_msg = f"Failed to compress archive {self._archive_dir}."
            raise ClpCoreRuntimeError(err_msg) from e
        finally:
            self._open = False
            self.close()

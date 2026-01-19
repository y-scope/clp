import io
import json
import os
import shutil
import subprocess
from collections.abc import Iterator
from contextlib import AbstractContextManager, suppress
from pathlib import Path
from tempfile import TemporaryDirectory
from types import TracebackType
from typing import BinaryIO, cast, IO, TextIO

from typing_extensions import Unpack

from clp_core._bin import _get_clp_exe
from clp_core._config import (
    ArchiveInputSource,
    ClpQuery,
    CompressionKwargs,
    DecompressionKwargs,
    LogEvent,
    SearchKwargs,
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


class ClpArchiveWriter(AbstractContextManager["ClpArchiveWriter", None]):
    """
    Archive handler for creating and writing a CLP archive.

    This class collects one or more input sources and produces a single CLP archive. Instances
    should be closed explicitly, as the writer finalizes compression and releases all associated
    resources at that point.
    """

    def __init__(self, file: ArchiveInputSource, **kwargs: Unpack[CompressionKwargs]) -> None:
        """
        Initialize a `ClpArchiveWriter` instance.

        If the provided archive is a binary stream sink, a temporary archive is created, and its
        contents are written to the stream when the writer is closed.
        :param file: Archive sink. Must be either a filesystem path or a binary I/O object.
        :param kwargs: Compression specific keyword arguments. See `CompressionKwargs`.
        :raise ClpCoreRuntimeError: if initialization fails.
        """
        self._file: ArchiveInputSource = file
        self._compression_level: int | None = kwargs.get("compression_level")
        self._timestamp_key: str | None = kwargs.get("timestamp_key")

        self._files_to_compress: list[Path] = []
        self._temp_files_to_compress: list[Path] = []

        _validate_archive_source(self._file)

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
                err_msg = f"Failed to overwrite archive at {self._archive_dir}."
                raise ClpCoreRuntimeError(err_msg) from e
        else:
            # Create a temporary directory for the archive to live in, and feed
            # the archive into the BinaryIO stream.
            self._temp_archive_dir = TemporaryDirectory()
            self._archive_dir = Path(self._temp_archive_dir.name)

    def add(self, source: str | os.PathLike[str] | BinaryIO | TextIO) -> None:
        """
        Add an input source to be included in the compressed archive.

        :param source: Input source to add. Must be a filesystem path or an I/O stream. Filesystem
            paths are added directly. Stream inputs are first materialized into temporary files, as
            the CLP compression pipeline currently does not support operating on stdin.
        :raise BadCompressionInputError: If the input source type is not supported.
        :raise ClpCoreRuntimeError: If the input source cannot be processed successfully.
        """
        if isinstance(source, (str, os.PathLike)):
            self._files_to_compress.append(Path(source))
        elif isinstance(source, io.IOBase):
            temp_file_path = Path(_write_stream_to_temp_file(source, suffix=".log"))
            self._files_to_compress.append(temp_file_path)
            self._temp_files_to_compress.append(temp_file_path)
        else:
            err_msg = f"Invalid compression input type for archive {self._archive_dir}."
            raise BadCompressionInputError(err_msg)

    def _compress(self) -> None:
        """
        Perform compression and, if a binary I/O stream was provided at initialization, write the
        archive contents to that stream. Should not be called by the class user.

        :raise subprocess.CalledProcessError: If the archive compression fails.
        """
        if 0 == len(self._files_to_compress):
            err_msg = f"Nothing to compress for archive {self._archive_dir}."
            raise BadCompressionInputError(err_msg)

        clp_s_bin_path = _get_clp_exe("clp-s")
        compress_cmd = [str(clp_s_bin_path), "c", "--single-file-archive"]
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
        """Cleans up all temporary resources used by the archive writer."""
        for p in self._temp_files_to_compress:
            with suppress(OSError):
                p.unlink()

        if self._temp_archive_dir is not None:
            with suppress(OSError):
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
        """
        Finalize archive creation and release associated resources.

        Compression is performed only if no exception was raised while adding inputs. All temporary
        resources are released before returning, regardless of whether compression succeeds.
        :param exc_type:
        :param exc_val:
        :param exc_tb:
        :raise ClpCoreRuntimeError: If compression fails.
        """
        try:
            if exc_type is None:
                self._compress()
        except Exception as e:
            err_msg = f"Failed to compress archive {self._archive_dir}."
            raise ClpCoreRuntimeError(err_msg) from e
        finally:
            self.close()


class ClpArchiveReader(AbstractContextManager["ClpArchiveReader", None], Iterator[LogEvent]):
    """
    Archive handler for decompressing and reading a CLP archive.

    This reader provides sequential access to log events in their original ingestion order. The
    underlying reader stream is closed automatically after the final log event has been consumed.
    """

    def __init__(self, file: ArchiveInputSource, **kwargs: Unpack[DecompressionKwargs]) -> None:
        """
        Initialize a `ClpArchiveReader` instance.

        A temporary directory is created to hold the decompressed output and is used for reading log
        events.

        :param file: Archive source. Must be either a filesystem path or a binary I/O object. For a
            binary stream, its contents are written into a temporary archive file for decompression.
        :param kwargs: Decompression specific keyword arguments. See `DecompressionKwargs`.
        :raise ClpCoreRuntimeError: if the decompression handler fails.
        :raise subprocess.CalledProcessError: If the archive decompression fails.
        """
        self._file: ArchiveInputSource = file
        self._encoding: str | None = kwargs.get("encoding")
        self._errors: str | None = kwargs.get("errors")

        _validate_archive_source(self._file)

        self._archive_path: Path
        self._archive_is_temp: bool = False
        if isinstance(self._file, (str, os.PathLike)):
            self._archive_path = Path(self._file)
        else:
            self._archive_path = Path(_write_stream_to_temp_file(self._file, suffix=".clp"))
            self._archive_is_temp = True

        self._decompression_temp_dir: TemporaryDirectory[str] = TemporaryDirectory()
        clp_s_bin_path = _get_clp_exe("clp-s")
        decompress_cmd = [
            str(clp_s_bin_path),
            "x",
            "--ordered",
            str(self._archive_path),
            self._decompression_temp_dir.name,
        ]
        subprocess.run(decompress_cmd, check=True)

        self._decomp_fp: TextIO | None
        self._decomp_fp = _get_single_file_in_dir(Path(self._decompression_temp_dir.name)).open(
            mode="r",
            encoding=self._encoding,
            errors=self._errors,
        )

    def get_next_log_event(self) -> LogEvent:
        """
        Return the next log event from the archive.

        :raise StopIteration: When no more log events are available.
        :raise ClpCoreRuntimeError: If the reader has already been closed.
        """
        if self._decomp_fp is None:
            err_msg = f"ClpArchiveReader for archive at {self._archive_path} is closed."
            raise ClpCoreRuntimeError(err_msg)

        line = self._decomp_fp.readline()
        if 0 == len(line):
            raise StopIteration

        return LogEvent(kv_pairs=json.loads(line.rstrip("\n")))

    def close(self) -> None:
        """Cleans up all temporary resources used by the archive reader."""
        if self._archive_is_temp:
            with suppress(OSError):
                self._archive_path.unlink()

        if self._decomp_fp is not None:
            with suppress(OSError):
                self._decomp_fp.close()
            self._decomp_fp = None

        with suppress(OSError):
            self._decompression_temp_dir.cleanup()

    def __next__(self) -> LogEvent:
        return self.get_next_log_event()

    def __iter__(self) -> "ClpArchiveReader":
        return self

    def __enter__(self) -> "ClpArchiveReader":
        return self

    def __exit__(
        self,
        exc_type: type[BaseException] | None,
        exc_val: BaseException | None,
        exc_tb: TracebackType | None,
    ) -> None:
        self.close()


class ClpSearchResults(AbstractContextManager["ClpSearchResults", None], Iterator[LogEvent]):
    """
    Handler for executing a search query against a CLP archive.

    This class streams search results from the archive and provides an iterator interface over
    matching log events.
    """

    def __init__(
        self, file: ArchiveInputSource, query: ClpQuery, **kwargs: Unpack[SearchKwargs]
    ) -> None:
        """
        Initialize a `ClpSearchResults` instance.

        :param file: Archive source. Must be either a filesystem path or a binary I/O object. For a
            binary stream, its contents are written into a temporary archive file for decompression.
        :param query: Search query to evaluate against the archive.
        :param kwargs: Search specific keyword arguments. See `SearchKwargs`.
        :raise ClpCoreRuntimeError: If the archive search handler or execution fails.
        """
        self._file: ArchiveInputSource = file
        self._query: ClpQuery = query
        self._encoding: str | None = kwargs.get("encoding")
        self._errors: str | None = kwargs.get("errors")

        _validate_archive_source(self._file)

        self._archive_path: Path
        self._archive_is_temp: bool = False
        if isinstance(self._file, (str, os.PathLike)):
            self._archive_path = Path(self._file)
        else:
            self._archive_path = Path(_write_stream_to_temp_file(self._file, suffix=".clp"))
            self._archive_is_temp = True

        clp_s_bin_path = _get_clp_exe("clp-s")
        search_cmd = [
            str(clp_s_bin_path),
            "s",
            str(self._archive_path),
            self._query.get_query_string(),
        ]

        self._search_proc: subprocess.Popen[str] | None = None
        self._search_fp: IO[str] | None = None
        try:
            self._search_proc = subprocess.Popen(
                search_cmd,
                stdout=subprocess.PIPE,
                text=True,
                encoding=self._encoding,
                errors=self._errors,
                bufsize=1,  # flushes each line
            )
        except Exception as e:
            err_msg = f"Failed to query archive at {self._archive_path}."
            raise ClpCoreRuntimeError(err_msg) from e

        self._search_fp = self._search_proc.stdout
        if self._search_fp is None:
            err_msg = "Cannot read the search results stream."
            raise ClpCoreRuntimeError(err_msg)

    def get_next_log_event(self) -> LogEvent:
        """
        Return the next log event produced by the search query.

        :raise StopIteration: When no more matching log events are available.
        :raise ClpCoreRuntimeError: If the search results stream has been closed.
        :raise subprocess.CalledProcessError: If the search process exits with a non-zero status.
        """
        if self._search_fp is None or self._search_proc is None:
            err_msg = f"ClpSearchResults streaming for archive at {self._archive_path} is closed."
            raise ClpCoreRuntimeError(err_msg)

        line = self._search_fp.readline()
        if 0 == len(line):
            # Check if the search hit EOF or erred midway
            rc = self._search_proc.wait()
            if rc != 0:
                raise subprocess.CalledProcessError(rc, self._search_proc.args)
            raise StopIteration

        return LogEvent(kv_pairs=json.loads(line.rstrip("\n")))

    def close(self) -> None:
        """Cleans up all temporary resources used by the archive search handler."""
        if self._archive_is_temp:
            with suppress(OSError):
                self._archive_path.unlink()

        if self._search_fp is not None:
            with suppress(OSError):
                self._search_fp.close()
            self._search_fp = None

        if self._search_proc is not None:
            with suppress(OSError):
                self._search_proc.wait()
            self._search_proc = None

    def __next__(self) -> LogEvent:
        return self.get_next_log_event()

    def __iter__(self) -> "ClpSearchResults":
        return self

    def __enter__(self) -> "ClpSearchResults":
        return self

    def __exit__(
        self,
        exc_type: type[BaseException] | None,
        exc_val: BaseException | None,
        exc_tb: TracebackType | None,
    ) -> None:
        self.close()

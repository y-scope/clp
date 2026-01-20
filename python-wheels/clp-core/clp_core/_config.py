import os
from abc import ABC, abstractmethod
from dataclasses import dataclass
from typing import Any, IO

from typing_extensions import TypedDict

ArchiveInputSource = str | os.PathLike[str] | IO[bytes]
FILE_OBJ_COPY_CHUNK_SIZE = 1024 * 1024  # 1 MB


@dataclass
class LogEvent:
    """Single log event returned by a CLP Archive search."""

    kv_pairs: dict[str, Any]

    def get_kv_pairs(self) -> dict[str, Any]:
        """:return: the key value pairs associated with this log event."""
        return self.kv_pairs


class ClpQuery(ABC):
    """
    Abstract base class for CLP Archive search queries.

    Currently support `KqlQuery`.
    """

    @property
    @abstractmethod
    def query_language(self) -> str:
        """:return: the query language identifier."""

    @abstractmethod
    def get_query_string(self) -> str:
        """:return: the query string."""


class KqlQuery(ClpQuery):
    """
    Kibana Query Language (KQL) query.

    >>> query = KqlQuery('level: ERROR AND attr.ctx: "*conn1*"')
    >>> query.get_query_string()
    'level: ERROR AND attr.ctx: "*conn1*"'
    """

    query_language: str = "kql"

    def __init__(self, query: str) -> None:
        self._query_string: str = query

    def get_query_string(self) -> str:
        """:return: the query string."""
        return self._query_string


class CompressionKwargs(TypedDict, total=False):
    """
    Keyword arguments controlling archive compression behavior.

    :param compression_level: An integer from 1 to 19 controlling the level of compression; `1` is
        fastest and produces the least compression, and `19` is the slowest and produces the most
        compression. The default is `3`.
    :param timestamp_key: Path (e.g. x.y) for the field containing the log event's timestamp.
    """

    compression_level: int
    timestamp_key: str | None


class DecompressionKwargs(TypedDict, total=False):
    """
    Keyword arguments controlling archive decompression behavior.

    :param encoding: Same as the argument in the built-in `open()` function. The name of encoding
        used to decode or encode. the file.
    :param errors: Same as the argument in the built-in `open()` function. An optional string that
        specifies how encoding and decoding errors are to be handled.
    """

    encoding: str
    errors: str


class SearchKwargs(TypedDict, total=False):
    """
    Keyword arguments controlling archive search behavior.

    :param encoding: Same as the argument in the built-in `open()` function. The name of encoding
        used to decode or encode. the file.
    :param errors: Same as the argument in the built-in `open()` function. An optional string that
        specifies how encoding and decoding errors are to be handled.
    """

    encoding: str
    errors: str

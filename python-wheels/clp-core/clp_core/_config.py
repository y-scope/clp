import os
from abc import ABC, abstractmethod
from dataclasses import dataclass
from typing import Any, BinaryIO

from typing_extensions import TypedDict

ArchiveInputSource = str | os.PathLike[str] | BinaryIO


@dataclass
class LogEvent:
    kv_pairs: dict[str, Any]

    def get_kv_pairs(self) -> dict[str, Any]:
        return self.kv_pairs


class ClpQuery(ABC):
    """Abstract base class for search queries."""

    @property
    @abstractmethod
    def query_language(self) -> str:
        """Returns the query language identifier."""

    @abstractmethod
    def get_query_string(self) -> str:
        """Returns the query string."""


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
        return self._query_string


class CompressionKwargs(TypedDict, total=False):
    compression_level: int
    timestamp_key: str | None


class DecompressionKwargs(TypedDict, total=False):
    encoding: str
    errors: str


class SearchKwargs(TypedDict, total=False):
    encoding: str
    errors: str
    query: ClpQuery

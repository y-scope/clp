import os
from dataclasses import dataclass
from typing import Any, BinaryIO

from typing_extensions import TypedDict

ArchiveInputSource = str | os.PathLike[str] | BinaryIO


class CompressionKwargs(TypedDict, total=False):
    compression_level: int
    timestamp_key: str | None


class DecompressionKwargs(TypedDict, total=False):
    encoding: str
    errors: str


@dataclass
class LogEvent:
    kv_pairs: dict[str, Any]

    def get_kv_pairs(self) -> dict[str, Any]:
        return self.kv_pairs

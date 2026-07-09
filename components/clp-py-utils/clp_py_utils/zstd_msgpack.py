"""Utilities for serializing data as zstd-compressed MessagePack."""

from threading import local
from typing import Any, Protocol

import msgpack
import zstandard as zstd

ZSTD_COMPRESSION_LEVEL = 3


class _Compressor(Protocol):
    def compress(self, data: bytes) -> bytes:
        """
        Compresses bytes.

        :param data:
        :return: The compressed bytes.
        """
        ...


class _Decompressor(Protocol):
    def decompress(self, data: bytes) -> bytes:
        """
        Decompresses bytes.

        :param data:
        :return: The decompressed bytes.
        """
        ...


class _ZstdMsgpackContext(local):
    compressor: _Compressor | None = None
    decompressor: _Decompressor | None = None


_context = _ZstdMsgpackContext()


def _get_compressor() -> _Compressor:
    compressor = _context.compressor
    if compressor is None:
        compressor = zstd.ZstdCompressor(level=ZSTD_COMPRESSION_LEVEL)
        _context.compressor = compressor
    return compressor


def _get_decompressor() -> _Decompressor:
    decompressor = _context.decompressor
    if decompressor is None:
        decompressor = zstd.ZstdDecompressor()
        _context.decompressor = decompressor
    return decompressor


def serialize(data: Any) -> bytes:
    """
    Serializes data as zstd-compressed MessagePack.

    :param data:
    :return: The serialized bytes.
    """
    packed = msgpack.packb(data)
    return _get_compressor().compress(packed)


def deserialize(data: bytes) -> Any:
    """
    Deserializes zstd-compressed MessagePack data.

    :param data:
    :return: The deserialized data.
    """
    decompressed = _get_decompressor().decompress(data)
    return msgpack.unpackb(decompressed)

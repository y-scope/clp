"""Utility functions for CLP MCP server."""

import logging
from datetime import datetime, timezone
from typing import Any

from .constants import TIMESTAMP_NOT_AVAILABLE

logger = logging.getLogger(__name__)


def convert_epoch_to_date_string(epoch_ts: int) -> str:
    """
    :param epoch_ts: Unix epoch timestamp in milliseconds.
    :return: ISO 8601 formatted date string with millisecond precision (YYYY-MM-DDTHH:mm:ss.fffZ).
    :raise: ValueError if `epoch_ts` cannot be converted to a valid date string.
    """
    try:
        epoch_seconds = epoch_ts / 1000.0
        dt = datetime.fromtimestamp(epoch_seconds, tz=timezone.utc)
        return dt.isoformat(timespec="milliseconds").replace("+00:00", "Z")
    except (ValueError, OSError, OverflowError) as e:
        err_msg = f"Invalid timestamp {epoch_ts}: {e}."
        raise ValueError(err_msg) from e


def format_query_results(query_results: list[dict[str, Any]]) -> list[str]:
    """
    Formats the query results. For a log event to be formatted, it must contain the following
    kv-pairs:
    - "timestamp": An integer representing the epoch timestamp in milliseconds.
    - "message": A string representing the log message.

    The message will be formatted as `timestamp: <date string>, message: <message>`:

    :param query_results: A list of dictionaries representing kv-pair log events.
    :return: A list of strings representing formatted log events.
    """
    filtered = []
    for obj in query_results:
        epoch = obj.get("timestamp")
        timestamp_str = TIMESTAMP_NOT_AVAILABLE

        if isinstance(epoch, int):
            try:
                timestamp_str = convert_epoch_to_date_string(epoch)
            except (TypeError, ValueError) as e:
                logger.warning("Failed to convert epoch timestamp=%s to date string: %s.", epoch, e)

        message = obj.get("message", "")
        filtered.append(f"timestamp: {timestamp_str}, message: {message}")

    return filtered


def sort_by_timestamp(query_results: list[dict[str, Any]]) -> list[dict[str, Any]]:
    """
    Sorts the query results in-place by timestamp in descending order (latest to oldest).

    NOTE:
    - Timestamp is expected to be an integer representing the epoch timestamp in milliseconds,
      stored under the `timestamp` key.
    - If `timestamp` is missing or not an integer, it is treated as the oldest possible timestamp.

    :param query_results: A list of dictionaries representing kv-pair log events.
    :return: The input list sorted in-place.
    """

    def _key(log_entry: dict[str, Any]) -> int:
        ts = log_entry.get("timestamp")
        return ts if isinstance(ts, int) else -1

    query_results.sort(key=_key, reverse=True)

    return query_results

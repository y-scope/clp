"""Utility functions for CLP MCP server."""

import logging
from datetime import datetime, timezone
from typing import Any

from clp_mcp_server.server.constants import TIMESTAMP_NOT_AVAILABLE

logger = logging.getLogger(__name__)


def convert_date_string_to_epoch(date_string: str) -> int:
    """
    :param date_string: ISO 8601 formatted date string with millisecond precision
        (YYYY-MM-DDTHH:mm:ss.fffZ).
    :return: Unix epoch timestamp in milliseconds.
    :raise: ValueError if `date_string` cannot be parsed or converted to a valid Unix epoch.
    """
    try:
        cleaned_string = date_string.rstrip("Z")
        dt = datetime.fromisoformat(cleaned_string).replace(tzinfo=timezone.utc)

        epoch_seconds = dt.timestamp()
        return int(epoch_seconds * 1000)

    except (ValueError, AttributeError) as e:
        err_msg = f"Invalid date string {date_string}: {e}."
        raise ValueError(err_msg) from e


def parse_timestamp_range(
    formatted_begin_timestamp: str, formatted_end_timestamp: str
) -> tuple[int, int]:
    """
    :param formatted_begin_timestamp:
    :param formatted_end_timestamp:
    :return: A tuple containing:
        - The begin timestamp in Unix epoch milliseconds.
        - The end timestamp in Unix epoch milliseconds.
    :raise: ValueError if `formatted_end_timestamp` is earlier than `formatted_begin_timestamp`.
    :raise: ValueError if `formatted_begin_timestamp` or `formatted_end_timestamp` doesn't indicate
        UTC (missing trailing `Z`).
    :raise: Propagates `convert_date_string_to_epoch`'s exceptions.
    """
    if not formatted_begin_timestamp.endswith("Z") or not formatted_end_timestamp.endswith("Z"):
        err_msg = (
            f"Timestamp must end with 'Z' to indicate UTC. Got: {formatted_begin_timestamp} and"
            f" {formatted_end_timestamp} instead."
        )
        raise ValueError(err_msg)

    begin_ts = convert_date_string_to_epoch(formatted_begin_timestamp)
    end_ts = convert_date_string_to_epoch(formatted_end_timestamp)

    if end_ts < begin_ts:
        err_msg = (
            f"`formatted_end_timestamp` {formatted_end_timestamp} is earlier than"
            f" `formatted_begin_timestamp` {formatted_begin_timestamp}."
        )
        raise ValueError(err_msg)

    return begin_ts, end_ts


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
    - "link": A string representing the link to open the log viewer displaying the message.

    The message will be formatted as `timestamp: <date string>, message: <message>, link: <link>`.

    :param query_results: A list of dictionaries representing kv-pair log events.
    :return: A list of strings representing formatted log events.
    """
    formatted_log_events = []
    for obj in query_results:
        epoch = obj.get("timestamp")
        timestamp_str = TIMESTAMP_NOT_AVAILABLE

        if isinstance(epoch, int):
            try:
                timestamp_str = convert_epoch_to_date_string(epoch)
            except (TypeError, ValueError) as e:
                logger.warning("Failed to convert epoch timestamp=%s to date string: %s.", epoch, e)

        message = obj.get("message", "")
        if not message:
            logger.warning("Empty message attached to a log event: %s.", obj)
            continue

        link = obj["link"]

        formatted_log_events.append(f"timestamp: {timestamp_str}, message: {message}, link: {link}")

    return formatted_log_events


def sort_by_timestamp(query_results: list[dict[str, Any]]) -> list[dict[str, Any]]:
    """
    Sorts the query results in-place by timestamp in descending order (latest to oldest).

    Note:
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

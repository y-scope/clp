"""Utility functions for CLP MCP server."""

import logging
from datetime import datetime, timezone

logger = logging.getLogger(__name__)


def convert_date_string_to_epoch(date_string: str) -> int:
    """
    :param date_string: ISO 8601 formatted date string with millisecond precision
        (YYYY-MM-DDTHH:mm:ss.fffZ).
    :return: Unix epoch timestamp in milliseconds.
    :raise ValueError if date_string cannot be parsed or be converted to a valid Unix epoch.
    """
    try:
        cleaned_string = date_string.rstrip("Z")
        dt = datetime.fromisoformat(cleaned_string)
        if dt.tzinfo is None:
            dt = dt.replace(tzinfo=timezone.utc)

        # Convert to milliseconds
        epoch_seconds = dt.timestamp()
        return int(epoch_seconds * 1000)

    except (ValueError, AttributeError) as e:
        err_msg = f"Invalid date string {date_string}: {e}."
        raise ValueError(err_msg) from e


def parse_timestamp_range(begin_timestamp: str, end_timestamp: str) -> tuple[int, int]:
    """
    :param begin_timestamp:
    :param end_timestamp:
    :return: begin and end timestamp in Unix epoch timestamp with milliseconds precision.
    :raise Propagates `convert_date_string_to_epoch`'s exceptions.
    :raise ValueError if `end_timestamp` is earlier than `begin_timestamp`.
    :raise TypeError if `begin_timestamp` or `end_timestamp` is not a `str` type.
    """
    if not isinstance(begin_timestamp, str):
        err_msg = f"Object `{type(begin_timestamp).__name__}` is not of type `str`."
        raise TypeError(err_msg)

    if not isinstance(end_timestamp, str):
        err_msg = f"Object `{type(end_timestamp).__name__}` is not of type `str`."
        raise TypeError(err_msg)

    begin_epoch = convert_date_string_to_epoch(begin_timestamp)
    end_epoch = convert_date_string_to_epoch(end_timestamp)

    if end_epoch < begin_epoch:
        err_msg = (
            f"end_timestamp {end_timestamp} is earlier than begin_timestamp {begin_timestamp}."
        )
        raise ValueError(err_msg)

    return begin_epoch, end_epoch


def convert_epoch_to_date_string(epoch_ts: int) -> str:
    """
    :param epoch_ts: Unix epoch timestamp in milliseconds.
    :return: ISO 8601 formatted date string with millisecond precision (YYYY-MM-DDTHH:mm:ss.fffZ).
    :raise TypeError if `epoch_ts` is None or not an int type.
    :raise ValueError if `epoch_ts` cannot be converted to a valid date string.
    """
    if epoch_ts is None:
        err_msg = "Epoch timestamp cannot be None."
        raise TypeError(err_msg)

    if not isinstance(epoch_ts, int):
        err_msg = f"Object {type(epoch_ts).__name__} is not of type int."
        raise TypeError(err_msg)

    try:
        epoch_seconds = epoch_ts / 1000.0
        dt = datetime.fromtimestamp(epoch_seconds, tz=timezone.utc)
        return dt.strftime("%Y-%m-%dT%H:%M:%S.%f")[:-3] + "Z"
    except (ValueError, OSError, OverflowError) as e:
        err_msg = f"Invalid timestamp {epoch_ts}: {e}."
        raise ValueError(err_msg) from e


def filter_query_results(query_results: list[dict]) -> list[str]:
    """
    :param query_results: A list of dictionary containing log entries with its metadata.
    :return: A list of strings containing only the `timestamp` (as a date string) and `message` of
    a log entry.
    """
    filtered = []
    for obj in query_results:
        epoch = obj.get("timestamp")
        try:
            timestamp_str = convert_epoch_to_date_string(epoch)
        except (TypeError, ValueError) as e:
            logger.warning("Failed to convert epoch timestamp=%s to date string: %s", epoch, e)
            timestamp_str = "N/A"

        message = obj.get("message", "")
        filtered.append(f"timestamp: {timestamp_str}, message: {message}")

    return filtered


def sort_query_results(query_results: list[dict]) -> list[dict]:
    """
    :param query_results: A list of dictionary containing log entries with its metadata read from
    MongoDB.
    :return: A sorted list of dictionary containing log entries with its metadata, ordered by epoch
    from latest to oldest.
    """
    return sorted(query_results, key=lambda log_entry: log_entry.get("timestamp", 0), reverse=True)

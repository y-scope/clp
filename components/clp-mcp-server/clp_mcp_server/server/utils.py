"""Utility functions for CLP MCP server."""

import logging
from datetime import datetime, timezone

logger = logging.getLogger(__name__)


def convert_epoch_to_date_string(epoch_ts: int) -> str:
    """
    :param epoch_ts: Unix epoch timestamp in milliseconds.
    :return: ISO 8601 formatted date string with millisecond precision (YYYY-MM-DDTHH:mm:ss.fffZ).
    :raise TypeError if `epoch_ts` is None or not convertible to an int.
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
    :param query_results: A list of log entries with metadata read from MongoDB.
    :return: A list of strings encoded with date string timestamp and log entry message.
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

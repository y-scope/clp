"""Utility functions for CLP MCP server."""

from datetime import datetime, timezone


def _convert_epoch_to_date_string(epoch_ts: int) -> str:
    """
    :param epoch_ts: Unix epoch timestamp in milliseconds
    :return: ISO 8601 formatted date string with millisecond precision (YYYY-MM-DDTHH:mm:ss.fffZ)
    :raise TypeError: If epoch_ts is None or not an integer
    :raise ValueError: If epoch_ts is out of valid range
    :raise OSError: If the timestamp cannot be converted (platform-specific limits)
    """
    if epoch_ts is None:
        err_msg = "Timestamp cannot be None"
        raise TypeError(err_msg)

    if not isinstance(epoch_ts, int):
        err_msg = f"Timestamp must be int, got {type(epoch_ts).__name__}"
        raise TypeError(err_msg)

    try:
        epoch_seconds = epoch_ts / 1000.0
        dt = datetime.fromtimestamp(epoch_seconds, tz=timezone.utc)
        return dt.strftime("%Y-%m-%dT%H:%M:%S.%f")[:-3] + "Z"
    except (ValueError, OSError, OverflowError) as e:
        raise ValueError(f"Invalid timestamp {epoch_ts}: {e}") from e


def clean_query_results(results: list[dict]) -> list[str]:
    """
    Clean query results by keeping only timestamp and message fields.

    :param results: List of result dictionaries from the database
    :return: List of formatted strings with only timestamp and message
    :raise TypeError: If timestamp is invalid type
    :raise ValueError: If timestamp is out of valid range
    """
    cleaned = []
    try:
        for obj in results:
            timestamp_str = _convert_epoch_to_date_string(obj.get("timestamp"))
            message = obj.get("message", "")
            cleaned.append(f"timestamp: {timestamp_str}, message: {message}")
    except (TypeError, ValueError) as e:
        # Re-raise with context about which entry failed
        raise type(e)(f"Failed to clean result entry: {e}") from e

    return cleaned


def validate_date_string(date_string: str) -> bool:
    """
    Validates if a string is in ISO 8601 format (YYYY-MM-DDTHH:mm:ss.fffZ)

    :param date_string: Date string to validate
    :return: True if valid ISO 8601 format, False otherwise
    """
    try:
        # Try parsing the date string with milliseconds
        datetime.strptime(date_string, "%Y-%m-%dT%H:%M:%S.%fZ")
        return True
    except ValueError:
        return False

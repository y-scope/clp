"""Unit tests for CLP MCP server's utility functions."""

import pytest

from clp_mcp_server.server.utils import (
    convert_date_string_to_epoch,
    format_query_results,
    parse_timestamp_range,
    sort_by_timestamp,
)


class TestUtils:
    """Test suite for utility functions."""

    LINK = "http://localhost:4000/"

    # Error Messages:
    INVALID_DATE_STRING_ERROR = "Invalid date string"
    INVALID_DATE_STRING_FORMAT_ERROR = "Timestamp must end with 'Z' to indicate UTC."
    INVALID_DATE_STRING_VALUE_ERROR = "is earlier than `formatted_begin_timestamp`"

    # Test case: invalid timestamp types.
    INVALID_TYPE_ENTRIES = [
        {"timestamp": None, "message": '{"message":"Log with None timestamp"}\n', "link": LINK},
        {
            "timestamp": "1729267200000",  # str instead of int
            "message": '{"message":"Log with str timestamp"}\n',
            "link": LINK,
        },
        {
            "timestamp": 1729267200000.0,  # float instead of int
            "message": '{"message":"Log with float timestamp"}\n',
            "link": LINK,
        },
    ]
    EXPECTED_INVALID_TYPE = [
        f'timestamp: N/A, message: {{"message":"Log with None timestamp"}}\n, link: {LINK}',
        f'timestamp: N/A, message: {{"message":"Log with str timestamp"}}\n, link: {LINK}',
        f'timestamp: N/A, message: {{"message":"Log with float timestamp"}}\n, link: {LINK}',
    ]

    # Test case: invalid timestamp values.
    INVALID_VALUE_ENTRIES = [
        {
            "timestamp": 9999999999999999,
            "message": '{"message":"Log with overflow timestamp"}\n',
            "link": LINK,
        },
        {
            "timestamp": -9999999999999999,
            "message": '{"message":"Log with negative overflow timestamp"}\n',
            "link": LINK,
        },
    ]
    EXPECTED_INVALID_VALUE = [
        (f'timestamp: N/A, message: {{"message":"Log with overflow timestamp"}}\n, link: {LINK}'),
        (
            f'timestamp: N/A, message: {{"message":"Log with negative overflow timestamp"}}\n,'
            f" link: {LINK}"
        ),
    ]

    # Test case: missing timestamp and message fields.
    MISSING_TIMESTAMP_AND_MESSAGE_ENTRY = [
        {"_id": "test001", "link": LINK},
        {"_id": "test002", "message": '{"message":"Log with no timestamp"}\n', "link": LINK},
        {"_id": "test003", "timestamp": 0, "link": LINK},
    ]
    EXPECTED_MISSING_TIMESTAMP_AND_MESSAGE = [
        f'timestamp: N/A, message: {{"message":"Log with no timestamp"}}\n, link: {LINK}',
    ]

    # Testing basic functionality.
    RAW_LOG_ENTRIES = [
        {
            "_id": "test000",
            "timestamp": None,
            "message": '{"pid":null,"tid":null,"message":"Log at epoch none"}\n',
            "orig_file_path": "/var/log/app.log",
            "archive_id": "abc123",
            "log_event_ix": 99,
            "link": LINK,
        },
        {
            "_id": "test001",
            "timestamp": 0,
            "message": '{"ts":0,"pid":null,"tid":null,"message":"Log at epoch zero"}\n',
            "orig_file_path": "/var/log/app.log",
            "archive_id": "abc123",
            "log_event_ix": 100,
            "link": LINK,
        },
        {
            "_id": "test002",
            "timestamp": 1729267200000,  # Oct 18, 2024 16:00:00.000 (milliseconds)
            "message": (
                '{"ts":1729267200000,"pid":1234,"tid":5678,'
                '"message":"Log with zero milliseconds"}\n'
            ),
            "orig_file_path": "/var/log/app.log",
            "archive_id": "abc124",
            "log_event_ix": 101,
            "link": LINK,
        },
        {
            "_id": "test003",
            "timestamp": 1729267200123,  # Oct 18, 2024 16:00:00.123 (milliseconds)
            "message": (
                '{"ts":1729267200123,"pid":1234,"tid":5678,'
                '"message":"Log with millisecond precision"}\n'
            ),
            "orig_file_path": "/var/log/app.log",
            "archive_id": "abc125",
            "log_event_ix": 102,
            "link": (
                "http://localhost:4000/streamFile"
                "?dataset=default"
                "&type=json"
                "&streamId=abc125"
                "&logEventIdx=102"
            ),
        },
    ]

    EXPECTED_RESULTS = [
        (
            "timestamp: 2024-10-18T16:00:00.123Z, message: "
            '{"ts":1729267200123,"pid":1234,"tid":5678,'
            '"message":"Log with millisecond precision"}\n, '
            "link: http://localhost:4000/streamFile"
            "?dataset=default"
            "&type=json"
            "&streamId=abc125"
            "&logEventIdx=102"
        ),
        (
            f"timestamp: 2024-10-18T16:00:00.000Z, message: "
            f'{{"ts":1729267200000,"pid":1234,"tid":5678,'
            f'"message":"Log with zero milliseconds"}}\n, link: {LINK}'
        ),
        (
            f"timestamp: 1970-01-01T00:00:00.000Z, message: "
            f'{{"ts":0,"pid":null,"tid":null,'
            f'"message":"Log at epoch zero"}}\n, link: {LINK}'
        ),
        (
            f"timestamp: N/A, message: "
            f'{{"pid":null,"tid":null,'
            f'"message":"Log at epoch none"}}\n, link: {LINK}'
        ),
    ]

    def test_convert_date_string_to_epoch(self):
        """Validates converting ISO 8601 format to a Unix epoch."""
        result = convert_date_string_to_epoch("2024-10-18T16:00:00.123Z")
        assert result == 1729267200123

        result = convert_date_string_to_epoch("2024-10-18T16:00:00Z")
        assert result == 1729267200000

        result = convert_date_string_to_epoch("2024-10-18T16:00Z")
        assert result == 1729267200000

    def test_convert_date_string_to_epoch_invalid_date_string(self):
        """Validates the handling of invalid date string."""
        with pytest.raises(ValueError) as exc_info:
            convert_date_string_to_epoch("not-a-date")
        assert self.INVALID_DATE_STRING_ERROR in str(exc_info.value)

        with pytest.raises(ValueError) as exc_info:
            convert_date_string_to_epoch("2024-13-45T25:99:99Z")
        assert self.INVALID_DATE_STRING_ERROR in str(exc_info.value)

        with pytest.raises(ValueError) as exc_info:
            convert_date_string_to_epoch("")
        assert self.INVALID_DATE_STRING_ERROR in str(exc_info.value)

        with pytest.raises(ValueError) as exc_info:
            print(convert_date_string_to_epoch("2024.10.18T16:00:00.123"))
        assert self.INVALID_DATE_STRING_ERROR in str(exc_info.value)

        with pytest.raises(ValueError) as exc_info:
            print(convert_date_string_to_epoch("2024-10-18T16-00-00-123"))
        assert self.INVALID_DATE_STRING_ERROR in str(exc_info.value)

    def test_parse_timestamp_range_invalid_values(self):
        """Validates the handling of invalid date string types and values."""
        with pytest.raises(ValueError, match=self.INVALID_DATE_STRING_FORMAT_ERROR):
            parse_timestamp_range("2024-10-18T16:00:00.123", "2025-10-18T16:00:00.123")

        with pytest.raises(ValueError, match=self.INVALID_DATE_STRING_VALUE_ERROR):
            parse_timestamp_range("2024-10-18T16:00:00.123Z", "2000-10-18T16:00:00.123Z")

    def test_invalid_timestamp_type(self):
        """Validates the handling of noninteger timestamp types."""
        result = format_query_results(self.INVALID_TYPE_ENTRIES)

        assert result == self.EXPECTED_INVALID_TYPE

    def test_invalid_timestamp_value(self):
        """Validates the handling of invalid timestamp values."""
        result = format_query_results(self.INVALID_VALUE_ENTRIES)

        assert result == self.EXPECTED_INVALID_VALUE

    def test_missing_timestamp_and_message(self):
        """Validates the handling of log entries without timestamp and message field."""
        result = format_query_results(self.MISSING_TIMESTAMP_AND_MESSAGE_ENTRY)

        assert result == self.EXPECTED_MISSING_TIMESTAMP_AND_MESSAGE

    def test_sort_and_format_query_results(self):
        """Validates the post-processing functionality."""
        sorted_result = sort_by_timestamp(self.RAW_LOG_ENTRIES)
        formatted_results = format_query_results(sorted_result)

        assert formatted_results == self.EXPECTED_RESULTS

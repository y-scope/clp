"""Unit tests for CLP MCP server's utility functions."""

import pytest

from clp_mcp_server.server.utils import (
    convert_date_string_to_epoch,
    filter_query_results,
    sort_query_results
)

class TestUtils:
    """Test suite for utility functions."""
    
    # Error Messages:
    INVALID_DATE_STRING = "Invalid date string"
    INVALID_DATE_STRING_TYPE = "Object int is not of type str."
    INVALID_DATE_STRING_TYPE_NONE = "Date string cannot be None."

    # Test case: invalid timestamp types.
    INVALID_TYPE_ENTRIES = [
        {
            "timestamp": None,
            "message": '{"message":"Log with None timestamp"}\n',
        },
        {
            "timestamp": "1729267200000",  # str instead of int
            "message": '{"message":"Log with str timestamp"}\n',
        },
        {
            "timestamp": 1729267200000.0,  # float instead of int
            "message": '{"message":"Log with float timestamp"}\n',
        },
    ]
    INVALID_TYPE_EXPECTED = [
        'timestamp: N/A, message: {"message":"Log with None timestamp"}\n',
        'timestamp: N/A, message: {"message":"Log with str timestamp"}\n',
        'timestamp: N/A, message: {"message":"Log with float timestamp"}\n',
    ]

    # Test case: invalid timestamp values.
    INVALID_VALUE_ENTRIES = [
        {
            "timestamp": 9999999999999999,
            "message": '{"message":"Log with overflow timestamp"}\n',
        },
        {
            "timestamp": -9999999999999999,
            "message": '{"message":"Log with negative overflow timestamp"}\n',
        },
    ]
    INVALID_VALUE_EXPECTED = [
        'timestamp: N/A, message: {"message":"Log with overflow timestamp"}\n',
        'timestamp: N/A, message: {"message":"Log with negative overflow timestamp"}\n',
    ]

    # Test case: missing timestamp and message fields.
    MISSING_TIMESTAMP_AND_MESSAGE_ENTRY = [
        {
            "_id": "test001",
        }
    ]
    MISSING_TIMESTAMP_AND_MESSAGE_EXPECTED = [
        "timestamp: N/A, message: "
    ]

    # Testing basic functionality.
    RAW_LOG_ENTRIES = [
        {
            "_id": "test001",
            "timestamp": 0,
            "message": '{"ts":0,"pid":null,"tid":null,"message":"Log at epoch zero"}\n',
            "orig_file_path": "/var/log/app.log",
            "archive_id": "abc123",
            "log_event_ix": 100,
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
        },
    ]

    EXPECTED_RESULTS = [
        (
            'timestamp: 2024-10-18T16:00:00.123Z, message: '
            '{"ts":1729267200123,"pid":1234,"tid":5678,'
            '"message":"Log with millisecond precision"}\n'
        ),
        (
            'timestamp: 2024-10-18T16:00:00.000Z, message: '
            '{"ts":1729267200000,"pid":1234,"tid":5678,'
            '"message":"Log with zero milliseconds"}\n'
        ),
        (
            'timestamp: 1970-01-01T00:00:00.000Z, message: '
            '{"ts":0,"pid":null,"tid":null,'
            '"message":"Log at epoch zero"}\n'
        ),
    ]

    def test_convert_date_string_to_epoch(self):
        """Validates converting ISO 8601 format to a Unix epoch."""
        result = convert_date_string_to_epoch("2024-10-18T16:00:00.123Z")
        assert result == 1729267200123

        result = convert_date_string_to_epoch("2024-10-18T16:00:00.123")
        assert result == 1729267200123

        result = convert_date_string_to_epoch("2024-10-18T16:00:00Z")
        assert result == 1729267200000

        result = convert_date_string_to_epoch("2024-10-18T16:00")
        assert result == 1729267200000

        result = convert_date_string_to_epoch("2024-10-18T16")
        assert result == 1729267200000


    def test_convert_date_string_to_epoch_invalid_type(self):
        """Validates the handling of invalid date string types."""
        with pytest.raises(TypeError) as exc_info:
            convert_date_string_to_epoch(None)
        assert self.INVALID_DATE_STRING_TYPE_NONE in str(exc_info.value)

        with pytest.raises(TypeError) as exc_info:
            convert_date_string_to_epoch(12345)
        assert self.INVALID_DATE_STRING_TYPE in str(exc_info.value)


    def test_convert_date_string_to_epoch_invalid_date_string(self):
        """Validates the handling of invalid date string."""
        with pytest.raises(ValueError) as exc_info:
            convert_date_string_to_epoch("not-a-date")
        assert self.INVALID_DATE_STRING in str(exc_info.value)

        with pytest.raises(ValueError) as exc_info:
            convert_date_string_to_epoch("2024-13-45T25:99:99Z")
        assert self.INVALID_DATE_STRING in str(exc_info.value)

        with pytest.raises(ValueError) as exc_info:
            convert_date_string_to_epoch("")
        assert self.INVALID_DATE_STRING in str(exc_info.value)

        with pytest.raises(ValueError) as exc_info:
            print(convert_date_string_to_epoch("2024.10.18T16:00:00.123"))
        assert self.INVALID_DATE_STRING in str(exc_info.value)

        with pytest.raises(ValueError) as exc_info:
            print(convert_date_string_to_epoch("2024-10-18T16-00-00-123"))
        assert self.INVALID_DATE_STRING in str(exc_info.value)


    def test_invalid_timestamp_type(self):
        """Validates the handling of noninteger timestamp types."""
        result = filter_query_results(self.INVALID_TYPE_ENTRIES)

        assert result == self.INVALID_TYPE_EXPECTED

    def test_invalid_timestamp_value(self):
        """Validates the handling of invalid timestamp values."""
        result = filter_query_results(self.INVALID_VALUE_ENTRIES)

        assert result == self.INVALID_VALUE_EXPECTED
    
    def test_missing_timestamp_and_message(self):
        """Validates the handling of log entries without timestamp and message field."""
        result = filter_query_results(self.MISSING_TIMESTAMP_AND_MESSAGE_ENTRY)

        assert result == self.MISSING_TIMESTAMP_AND_MESSAGE_EXPECTED

    def test_sort_and_filter_query_results(self):
        """Validates the functionality of post-processing for the query response."""
        sorted_result = sort_query_results(self.RAW_LOG_ENTRIES)
        filtered_result = filter_query_results(sorted_result)

        assert filtered_result == self.EXPECTED_RESULTS

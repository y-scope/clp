import pytest
from clp_mcp_server.server.utils import clean_query_results


class TestConstants:
    """Constants for the clean_query_results test suite."""

    # Test data: CLP stores timestamps as milliseconds since Unix epoch
    RAW_LOG_ENTRIES = [
        {
            "_id": "507f1f77bcf86cd799439011",
            "timestamp": 0,  # Epoch zero (milliseconds)
            "message": '{"ts":0,"pid":null,"tid":null,"verbosity":"INFO","logger":"test.logger","message":"Log at epoch zero"}\n',
            "orig_file_path": "/var/log/app.log",
            "archive_id": "abc123",
            "log_event_ix": 100,
        },
        {
            "_id": "507f1f77bcf86cd799439012",
            "timestamp": 1729267200000,  # Oct 18, 2024 16:00:00.000 (milliseconds)
            "message": '{"ts":1729267200000,"pid":1234,"tid":5678,"verbosity":"INFO","logger":"app.service","message":"Log with zero milliseconds"}\n',
            "orig_file_path": "/var/log/app.log",
            "archive_id": "abc124",
            "log_event_ix": 101,
        },
        {
            "_id": "507f1f77bcf86cd799439013",
            "timestamp": 1729267200123,  # Oct 18, 2024 16:00:00.123 (milliseconds)
            "message": '{"ts":1729267200123,"pid":1234,"tid":5678,"verbosity":"DEBUG","logger":"app.handler","message":"Log with millisecond precision"}\n',
            "orig_file_path": "/var/log/app.log",
            "archive_id": "abc125",
            "log_event_ix": 102,
        },
    ]

    # Expected results - plain text format
    EXPECTED_RESULTS = [
        'timestamp: 1970-01-01T00:00:00.000Z, message: {"ts":0,"pid":null,"tid":null,"verbosity":"INFO","logger":"test.logger","message":"Log at epoch zero"}\n',
        'timestamp: 2024-10-18T16:00:00.000Z, message: {"ts":1729267200000,"pid":1234,"tid":5678,"verbosity":"INFO","logger":"app.service","message":"Log with zero milliseconds"}\n',
        'timestamp: 2024-10-18T16:00:00.123Z, message: {"ts":1729267200123,"pid":1234,"tid":5678,"verbosity":"DEBUG","logger":"app.handler","message":"Log with millisecond precision"}\n',
    ]


class TestCleanQueryResults:
    """Test suite for clean_query_results function."""

    def test_timestamp_formats_and_millisecond_precision(self):
        """Test CLP millisecond timestamps are converted correctly to ISO 8601."""
        cleaned = clean_query_results(TestConstants.RAW_LOG_ENTRIES)

        assert len(cleaned) == 3

        # Compare as strings directly
        for i, result_str in enumerate(cleaned):
            expected_str = TestConstants.EXPECTED_RESULTS[i]
            assert result_str == expected_str

        # Verify millisecond precision in third result
        assert "2024-10-18T16:00:00.123Z" in cleaned[2]

    def test_only_timestamp_and_message_fields(self):
        """Test that ONLY timestamp and message are kept, all metadata excluded."""
        cleaned = clean_query_results(TestConstants.RAW_LOG_ENTRIES)

        for result_str in cleaned:
            # Check the string contains both fields
            assert "timestamp:" in result_str
            assert "message:" in result_str
            # Check excluded fields are NOT present
            assert "_id" not in result_str
            assert "orig_file_path" not in result_str
            assert "archive_id" not in result_str
            assert "log_event_ix" not in result_str

    def test_returns_list_of_strings(self):
        """Test return type is list[str]."""
        cleaned = clean_query_results(TestConstants.RAW_LOG_ENTRIES)

        # Must be list of strings
        assert isinstance(cleaned, list)
        assert len(cleaned) == 3
        assert all(isinstance(item, str) for item in cleaned)
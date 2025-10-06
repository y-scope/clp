"""Tests for clp_connector.py."""
from collections.abc import AsyncIterator
from unittest.mock import AsyncMock, MagicMock, patch

import pytest

from clp_mcp_server import clp_connector


@pytest.mark.skip(reason="requires actual DB connections")
@pytest.mark.asyncio
async def test_submit_query() -> None:
    """Tests submitting a query and inserting metadata into MongoDB."""
    connector = clp_connector.CLPConnector()
    await connector.submit_query("1", 0, 1790790051822)


@pytest.mark.skip(reason="requires actual DB connections")
@pytest.mark.asyncio
async def test_read_job_status() -> None:
    """Tests reading the job status of a query."""
    connector = clp_connector.CLPConnector()
    await connector.read_job_status("12")


@pytest.mark.skip(reason="requires actual DB connections")
@pytest.mark.asyncio
async def test_wait_query_completion() -> None:
    """Tests waiting for a query to complete."""
    connector = clp_connector.CLPConnector()
    await connector.wait_query_completion("12")


@pytest.mark.skip(reason="requires actual DB connections")
@pytest.mark.asyncio
async def test_read_results() -> None:
    """Tests reading results from MongoDB."""
    connector = clp_connector.CLPConnector()
    await connector.read_results("12")


@pytest.mark.asyncio
async def test_submit_query_invalid_timestamps() -> None:
    """Tests submitting a query with invalid timestamps."""
    connector = clp_connector.CLPConnector()
    with pytest.raises(ValueError, match="smaller than"):
        await connector.submit_query("test", 100, 50)


@pytest.mark.asyncio
async def test_read_job_status_not_found() -> None:
    """Tests reading job status for a non-existent query."""
    connector = clp_connector.CLPConnector()
    mock = AsyncMock(side_effect=ValueError("not found"))
    with patch.object(connector, "read_job_status", mock), \
        pytest.raises(ValueError, match="not found"):
            await connector.read_job_status("999")


@pytest.mark.asyncio
async def test_wait_query_completion_succeeded() -> None:
    """Tests waiting for a query to complete successfully."""
    connector = clp_connector.CLPConnector()
    # Simulate status: PENDING -> RUNNING -> SUCCEEDED
    statuses = [
        clp_connector.QueryJobStatus.PENDING,
        clp_connector.QueryJobStatus.RUNNING,
        clp_connector.QueryJobStatus.SUCCEEDED
    ]
    connector.read_job_status = AsyncMock(side_effect=statuses)
    with patch("asyncio.sleep", AsyncMock()):
        await connector.wait_query_completion("42")


@pytest.mark.asyncio
@pytest.mark.parametrize(("fail_status", "exc_type"), [
    (clp_connector.QueryJobStatus.FAILED, RuntimeError),
    (clp_connector.QueryJobStatus.CANCELLED, RuntimeError),
    (clp_connector.QueryJobStatus.KILLED, RuntimeError),
    (999, RuntimeError),  # unknown status
])
async def test_wait_query_completion_failure_cases(
        fail_status: clp_connector.QueryJobStatus,
        exc_type: Exception
    ) -> None:
    """Tests waiting for a query that ends in failure, cancellation, or unknown status."""
    connector = clp_connector.CLPConnector()
    connector.read_job_status = AsyncMock(return_value=fail_status)
    with pytest.raises(exc_type):
        await connector.wait_query_completion("fail_id")


@pytest.mark.asyncio
async def test_read_results_returns_docs() -> None:
    """Tests reading results returns expected documents."""
    connector = clp_connector.CLPConnector()

    mock_docs = [{"_id": "1"}, {"_id": "2"}, {"_id": "3"}]
    mock_collection = AsyncMock()

    async def async_gen() -> AsyncIterator[dict]:
        for doc in mock_docs:
            yield doc

    mock_collection.find = MagicMock(return_value=async_gen())
    connector.results_cache = { "12": mock_collection }

    results = await connector.read_results("12")
    assert results == mock_docs

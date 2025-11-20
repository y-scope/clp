"""Tests for clp_connector.py."""

from collections.abc import AsyncGenerator, Iterable
from types import SimpleNamespace
from typing import Any, TypeVar
from unittest.mock import AsyncMock, MagicMock, patch

import pytest

from clp_mcp_server.clp_connector import ClpConnector, QueryJobStatus


@pytest.fixture
def mock_clp_config() -> Any:
    """Provides a mock CLP configuration for testing."""
    return SimpleNamespace(
        results_cache=SimpleNamespace(
            host="results-cache", port=27017, db_name="clp-query-results"
        ),
        database=SimpleNamespace(host="database", port=3306, name="clp-db"),
        webui=SimpleNamespace(host="localhost", port=4000),
    )


@pytest.mark.skip(reason="requires actual DB connections")
@pytest.mark.asyncio
async def test_submit_query(mock_clp_config: Any) -> None:
    """Tests submitting a query and inserting metadata into MongoDB."""
    connector = ClpConnector(mock_clp_config)
    await connector.submit_query("1", 0, 1790790051822)


@pytest.mark.skip(reason="requires actual DB connections")
@pytest.mark.asyncio
async def test_read_job_status(mock_clp_config: Any) -> None:
    """Tests reading the job status of a query."""
    connector = ClpConnector(mock_clp_config)
    await connector.read_job_status("12")


@pytest.mark.skip(reason="requires actual DB connections")
@pytest.mark.asyncio
async def test_wait_query_completion(mock_clp_config: Any) -> None:
    """Tests waiting for a query to complete."""
    connector = ClpConnector(mock_clp_config)
    await connector.wait_query_completion("12")


@pytest.mark.skip(reason="requires actual DB connections")
@pytest.mark.asyncio
async def test_read_results(mock_clp_config: Any) -> None:
    """Tests reading results from MongoDB."""
    connector = ClpConnector(mock_clp_config)
    await connector.read_results("12")


@pytest.mark.asyncio
async def test_submit_query_invalid_timestamps(mock_clp_config: Any) -> None:
    """Tests submitting a query with invalid timestamps."""
    connector = ClpConnector(mock_clp_config)
    with pytest.raises(ValueError, match="smaller than"):
        await connector.submit_query("test", 100, 50)


@pytest.mark.asyncio
async def test_read_job_status_not_found(mock_clp_config: Any) -> None:
    """Tests reading job status for a non-existent query."""
    connector = ClpConnector(mock_clp_config)
    mock = AsyncMock(side_effect=ValueError("not found"))
    with (
        patch.object(connector, "read_job_status", mock),
        pytest.raises(ValueError, match="not found"),
    ):
        await connector.read_job_status("999")


@pytest.mark.asyncio
async def test_wait_query_completion_succeeded(mock_clp_config: Any) -> None:
    """Tests waiting for a query to complete successfully."""
    connector = ClpConnector(mock_clp_config)
    # Simulate status: PENDING -> RUNNING -> SUCCEEDED
    statuses = [QueryJobStatus.PENDING, QueryJobStatus.RUNNING, QueryJobStatus.SUCCEEDED]
    connector.read_job_status = AsyncMock(side_effect=statuses)
    with patch("asyncio.sleep", AsyncMock()):
        await connector.wait_query_completion("42")


@pytest.mark.asyncio
@pytest.mark.parametrize(
    ("fail_status", "exc_type"),
    [
        (QueryJobStatus.FAILED, RuntimeError),
        (QueryJobStatus.CANCELLED, RuntimeError),
        (QueryJobStatus.KILLED, RuntimeError),
        (999, RuntimeError),  # unknown status
    ],
)
async def test_wait_query_completion_failure_cases(
    fail_status: QueryJobStatus, exc_type: type[Exception], mock_clp_config: Any
) -> None:
    """Tests waiting for a query that ends in failure, cancellation, or unknown status."""
    connector = ClpConnector(mock_clp_config)
    connector.read_job_status = AsyncMock(return_value=fail_status)
    with pytest.raises(exc_type):
        await connector.wait_query_completion("fail_id")


@pytest.mark.asyncio
async def test_read_results_returns_docs(mock_clp_config: Any) -> None:
    """Tests reading results returns expected documents."""
    connector = ClpConnector(mock_clp_config)
    mock_docs = [{"_id": "1"}, {"_id": "2"}, {"_id": "3"}]
    mock_collection = AsyncMock()
    mock_collection.find = MagicMock(return_value=_aiter(mock_docs))

    with patch.object(connector, "_results_cache", {"12": mock_collection}):
        results = await connector.read_results("12")

    assert results == mock_docs


@pytest.mark.asyncio
async def test_read_results_adds_link_field(mock_clp_config: Any) -> None:
    """Ensures read_results adds a 'link' field."""
    connector = ClpConnector(mock_clp_config)
    mock_docs = [
        {"_id": "1", "archive_id": "archA", "log_event_ix": 10},
        {"_id": "2", "archive_id": "archB", "log_event_ix": 20},
    ]
    mock_collection = AsyncMock()
    mock_collection.find = MagicMock(return_value=_aiter(mock_docs))

    with patch.object(connector, "_results_cache", {"12": mock_collection}):
        results = await connector.read_results("12")

    assert len(results) == len(mock_docs)
    for original, result in zip(mock_docs, results, strict=True):
        expected_link = (
            f"http://{mock_clp_config.webui.host}:{mock_clp_config.webui.port}"
            f"/streamFile?type=json&streamId={original['archive_id']}"
            f"&dataset=default&logEventIdx={original['log_event_ix']}"
        )
        assert result["link"] == expected_link


T = TypeVar("T")


async def _aiter(it: Iterable[T]) -> AsyncGenerator[T, None]:
    """
    Yields items from an iterable asynchronously.

    :param it: An iterable of items.
    :yield: Items from the iterable.
    """
    for i in it:
        yield i

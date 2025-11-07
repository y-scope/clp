"""
The integration tests for the CLP MCP server.

IMPORTANT: These tests are designed to be run **manually** against a deployed CLP package.
We will migrate them to automated tests when the test environment includes docker compose.

### Environment Setup

Deploy the CLP package with MCP server. The test suites assume the  MCP server is accessible
at http://0.0.0.0:8000/mcp. If the MCP server is hosted at a different address, update the
`MCP_SERVER_ENDPOINT` variable below accordingly.

### Log Dataset

The test dataset is taken from the `HDFS-11896` case study. To access the logs, visit the
repository [y-scope/agent-dev](https://github.com/y-scope/agent-dev) and follow the instructions
from `case-study/sync-logs-to-r2.sh`. The log file is located in `CLODS/HDFS/HDFS-11896/json`.
Use the instructions below to ingest the logs into CLP package:
```
gzip -d CLODS/HDFS/HDFS-11896/json/hadoop-dn1__logs__hadoop.jsonl.gz
sbin/compress.sh --timestamp-key ts CLODS/HDFS/HDFS-11896/json/hadoop-dn1__logs__hadoop.jsonl
```

### Running Tests

The tests in this suite are skipped by default because they require a manually deployed CLP package
with MCP server enabled. (The skipping is done by setting `-m "not mcp"` in `.pytest.ini`.) To run
them, execute the following command in the terminal:
```
pytest -m mcp tests/test_mcp_server.py
```
"""

import pytest
from fastmcp import Client

MCP_SERVER_ENDPOINT = "http://0.0.0.0:8000/mcp"


@pytest.mark.mcp
@pytest.mark.asyncio
async def test_get_instructions() -> None:
    """Tests the get_instructions tool."""
    async with Client(MCP_SERVER_ENDPOINT) as client:
        res = await client.call_tool("get_instructions")
        assert isinstance(res.data, str)


@pytest.mark.mcp
@pytest.mark.asyncio
async def test_search_by_kql() -> None:
    """Tests the search_by_kql tool."""
    async with Client(MCP_SERVER_ENDPOINT) as client:
        await client.call_tool("get_instructions")

        query = "*filter*"
        result = await client.call_tool("search_by_kql", {"kql_query": query})

        assert "items" in result.data
        assert isinstance(result.data["items"], list)
        assert "num_total_items" in result.data
        assert "num_total_pages" in result.data
        assert "num_items_per_page" in result.data
        assert "has_next" in result.data
        assert "has_previous" in result.data


@pytest.mark.mcp
@pytest.mark.asyncio
async def test_get_nth_page() -> None:
    """Tests the get_nth_page tool."""
    async with Client(MCP_SERVER_ENDPOINT) as client:
        await client.call_tool("get_instructions")

        query = "*filter*"
        await client.call_tool("search_by_kql", {"kql_query": query})

        result = await client.call_tool("get_nth_page", {"page_index": 1})
        assert "items" in result.data
        assert isinstance(result.data["items"], list)
        assert "num_total_items" in result.data
        assert "num_total_pages" in result.data
        assert "num_items_per_page" in result.data
        assert "has_next" in result.data
        assert "has_previous" in result.data


@pytest.mark.mcp
class TestSearchWithTimestamp:
    """Tests the search_by_kql_with_timestamp_range tool."""

    @pytest.mark.asyncio
    async def test_success_case(self) -> None:
        """Tests the success case of the search_by_kql_with_timestamp_range tool."""
        async with Client(MCP_SERVER_ENDPOINT) as client:
            await client.call_tool("get_instructions")

            query = "*filter*"
            result = await client.call_tool(
                "search_by_kql_with_timestamp_range",
                {
                    "kql_query": query,
                    "formatted_begin_timestamp": "2025-08-27T15:35:30.000Z",
                    "formatted_end_timestamp": "2025-08-27T15:35:50.000Z",
                },
            )
            assert "items" in result.data
            assert isinstance(result.data["items"], list)
            assert "num_total_items" in result.data
            assert "num_total_pages" in result.data
            assert "num_items_per_page" in result.data
            assert "has_next" in result.data
            assert "has_previous" in result.data

    @pytest.mark.asyncio
    async def test_invalid_timestamps(self) -> None:
        """Tests the search_by_kql_with_timestamp_range tool with invalid timestamps."""
        async with Client(MCP_SERVER_ENDPOINT) as client:
            await client.call_tool("get_instructions")

            query = "*filter*"
            result = await client.call_tool(
                "search_by_kql_with_timestamp_range",
                {
                    "kql_query": query,
                    "formatted_begin_timestamp": "2025-08-27T15:35:55.000Z",
                    "formatted_end_timestamp": "2025-08-27T15:35:50.000Z",
                },
            )
            assert "Error" in result.data

    @pytest.mark.asyncio
    async def test_query_failure(self) -> None:
        """Tests the search_by_kql_with_timestamp_range tool with a query that is killed."""
        async with Client(MCP_SERVER_ENDPOINT) as client:
            await client.call_tool("get_instructions")

            query = "*filter*"
            result = await client.call_tool(
                "search_by_kql_with_timestamp_range",
                {
                    "kql_query": query,
                    # This test (ab)uses a design in CLP: when the begin timestamp is set before
                    # Unix epoch, the query will be killed. It works as of Oct. 22, 2025. The
                    # future versions of CLP may change this behavior.
                    "formatted_begin_timestamp": "1970-01-01T00:00:00.000Z",
                    "formatted_end_timestamp": "1970-01-02T00:00:00.000Z",
                },
            )
            assert "Error" in result.data


@pytest.mark.mcp
@pytest.mark.asyncio
async def test_log_link() -> None:
    """Tests that the search results contain log links."""
    async with Client(MCP_SERVER_ENDPOINT) as client:
        await client.call_tool("get_instructions")

        query = "*error*"
        result = await client.call_tool("search_by_kql", {"kql_query": query})

        for item in result.data["items"]:
            assert "link" in item


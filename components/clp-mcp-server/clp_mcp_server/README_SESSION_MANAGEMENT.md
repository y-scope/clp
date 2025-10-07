# Session Management System for CLP MCP Server

## Overview

The session management system provides a robust solution for handling multiple concurrent user sessions with paginated query results. It implements the design specification for the `get_nth_page` tool, allowing efficient retrieval of large log query results in manageable pages.

## Architecture

### Core Components

1. **QueryResult**: Manages cached query results with pagination support
2. **SessionState**: Represents individual user sessions with their query history
3. **SessionManager**: Orchestrates multiple sessions and handles lifecycle management

## Features

### Pagination
- Uses the `paginate` library for efficient page management
- Default page size of 10 items (configurable)
- Supports navigation through large result sets
- Maximum of 1,000 cached results per query (configurable)

### Session Management
- Unique session identification via `session_id`
- Automatic session creation on first request
- Session expiration after 60 minutes of inactivity (configurable)
- Query result expiration after 30 minutes (configurable)

### Memory Management
- Automatic truncation of results exceeding the cache limit
- Expired session cleanup to prevent memory leaks
- Efficient storage of only the most recent query per session

## Usage

### Basic Workflow

1. **Initialize Instructions** (once per session)
```python
# User calls get_instructions() first
instructions = get_instructions()
```

2. **Execute a Query**
```python
# Search logs with KQL query
result = search_kql_query(
    kql_query="error AND service:api",
    ctx=Context(session_id="user_123")
)
# Returns: {"Page0": [...], "NumPages": 5, "TotalResults": 45}
```

3. **Retrieve Additional Pages**
```python
# Get the second page (0-indexed)
page_data = get_nth_page(
    page_index=1,
    ctx=Context(session_id="user_123")
)
# Returns: {"Page": [...], "PageNumber": 2, "TotalPages": 5, ...}
```

### Configuration

The `SessionManager` can be configured with the following parameters:

```python
session_manager = SessionManager(
    page_size=10,              # Items per page
    max_cached_results=1000,   # Maximum cached results per query
    session_ttl_minutes=60,    # Session expiration time
    query_ttl_minutes=30,      # Query result expiration time
)
```

## API Reference

### SessionManager Methods

#### `get_or_create_session(session_id: str) -> SessionState`
Get an existing session or create a new one.

#### `cache_query_result(session_id, query, results, page_size) -> Tuple[Dict, int]`
Cache query results and return the first page with total page count.

#### `get_nth_page(session_id: str, page_index: int) -> Dict`
Retrieve a specific page from cached results.

#### `cleanup_expired_sessions() -> int`
Remove expired sessions and return the count of removed sessions.

### Error Handling

The system provides clear error messages for common scenarios:

- **Session not found**: "Session not found. Please run a query first."
- **Session expired**: "Session expired. Please run a new query."
- **Query expired**: "Query results expired. Please run a new query."
- **Invalid page**: "Invalid page index. Valid range: 0-N"
- **Instructions not run**: "Please call get_instructions() first..."

## Scalability Considerations

### Current Implementation
- In-memory storage for session states
- One cached query result per session
- Automatic cleanup of expired sessions

### Future Enhancements
1. **Persistent Storage**: Redis or database backend for session storage
2. **Query History**: Multiple cached queries per session
3. **Distributed Sessions**: Support for horizontal scaling
4. **Query Optimization**: Smart caching based on query patterns
5. **Compression**: Compress cached results to reduce memory usage

## Testing

Run the test suite:
```bash
pytest tests/test_session_manager.py -v
```

The test suite covers:
- Query result pagination
- Session lifecycle management
- Expiration handling
- Error scenarios
- Memory management

## Example Integration

```python
from fastmcp import FastMCP, Context
from session_manager import SessionManager

mcp = FastMCP(name="clp-mcp-server")
session_manager = SessionManager()

@mcp.tool
def search_logs(query: str, ctx: Context) -> dict:
    # Perform actual log search
    results = search_backend(query)
    
    # Cache and return first page
    first_page, total_pages = session_manager.cache_query_result(
        session_id=ctx.session_id,
        query=query,
        results=results
    )
    
    return {
        "Page0": first_page["items"],
        "NumPages": total_pages,
        "TotalResults": first_page["total_items"]
    }

@mcp.tool
def get_nth_page(page_index: int, ctx: Context) -> dict:
    return session_manager.get_nth_page(ctx.session_id, page_index)
```

## Performance Metrics

- **Page retrieval**: O(1) for cached results
- **Session lookup**: O(1) hash table lookup
- **Memory usage**: ~1MB per 1000 log entries (varies by content)
- **Cleanup operation**: O(n) where n is number of sessions

## Security Considerations

1. **Session Isolation**: Each session has isolated query results
2. **No Cross-Session Access**: Sessions cannot access other sessions' data
3. **Automatic Expiration**: Prevents indefinite data retention
4. **Resource Limits**: Maximum cache size prevents memory exhaustion

## Monitoring

Key metrics to monitor:
- Active session count
- Memory usage per session
- Query cache hit/miss ratio
- Session expiration rate
- Average results per query
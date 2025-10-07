"""MCP Server implementation."""

from datetime import datetime
from typing import Any, Dict, List, Optional, Tuple

from fastmcp import FastMCP
from fastmcp import Context

from .session_manager import SessionManager


class ProtocolConstant:
    """Constants for the CLP MCP Server."""

    SERVER_NAME = "clp-mcp-server"

    # Tool names
    TOOL_HELLO_WORLD = "hello_world"
    TOOL_GET_SERVER_INFO = "get_server_info"

    @classmethod
    def get_capabilities(cls) -> list[str]:
        """
        Gets the capabilities of the server.
        :return: A list of tool names supported by the server.
        """
        return [cls.TOOL_HELLO_WORLD, cls.TOOL_GET_SERVER_INFO]


def create_mcp_server() -> FastMCP:
    """
    Creates and defines API tool calls for the CLP MCP server.

    :return: A configured `FastMCP` instance.
    :raise: Propagates `FastMCP.__init__`'s exceptions.
    :raise: Propagates `FastMCP.tool`'s exceptions.
    """
    mcp = FastMCP(name=ProtocolConstant.SERVER_NAME)

    # Initialize the session manager with configuration
    session_manager = SessionManager(
        page_size=10,
        max_cached_results=1000,
        session_ttl_minutes=60,
    )

    def ensure_instructions_run(session_id: str) -> Optional[str]:
        """
        Ensures that get_instructions has been called once per session.
        
        :param session_id: The session identifier
        :return: Error message if instructions haven't been run, None otherwise
        """
        session = session_manager.get_or_create_session(session_id)
        if not session.flags.get("ran_instructions", False):
            return "Please call get_instructions() first to understand how to use this MCP server."
        return None

    @mcp.tool
    def get_instructions() -> str:
        """This should be the first tool called by the MCP server
        to get instructions on how to use this MCP server.

        Returns:
         instructions: A string of instructions on how to use this MCP server
        """
        instructions = """You are an AI assistant that helps users query a log database using KQL (Kibana Query Language). 
        When given a user query, you should generate a KQL query that accurately captures the user's intent.
        The KQL query should be as specific as possible to minimize the number of log messages returned.
        You should also consider the following guidelines when generating KQL queries:
        - Use specific field names and values to narrow down the search. 
        - Avoid using wildcards (*) unless absolutely necessary, as they can lead to large result sets.
        - Use logical operators (AND, OR, NOT) to combine multiple conditions.
        - Consider the time range of the logs you are searching. If the user specifies a time range, include it in the KQL query.
        - If the user query is ambiguous or lacks detail, ask clarifying questions to better understand their intent before generating the KQL query.
        - Always ensure that the generated KQL query is syntactically correct and can be executed without errors.
        
        Available tools:
        1. search_kql_query: Search logs with a KQL query
        2. search_kql_query_with_timestamp: Search logs with a KQL query and timestamp range
        3. get_nth_page: Retrieve additional pages of results from the last query
        
        Note: Results are paginated with 10 items per page. Use get_nth_page to retrieve additional pages."""
        
        return instructions

    @mcp.tool
    def search_kql_query(kql_query: str, ctx: Context) -> dict[str, object]:
        """Search the logs for the specified query and returns the latest 10 log messages
        matching the query (first page of results) and the total number of pages available

        Args:
         kql_query: KQL (Kabana Query Language) query to find matching log messages 

        Returns:
         A dictionary with two keys:
          - Page0: The latest 10 log messages matching the query.
          - NumPages: Total number of pages that can be indexed
        """
        # Mark instructions as run for this session
        session = session_manager.get_or_create_session(ctx.session_id)
        session.flags["ran_instructions"] = True
        
        # TODO: Replace with actual log search implementation
        # This is a placeholder that generates sample log data
        sample_logs = [
            f"Log entry {i}: Query '{kql_query}' matched this log message"
            for i in range(25)  # Simulate 25 results (3 pages)
        ]
        
        # Cache the query results and get the first page
        first_page_data, total_pages = session_manager.cache_query_result(
            session_id=ctx.session_id,
            query=kql_query,
            results=sample_logs,
        )
        
        return {
            "Page0": first_page_data.get("items", []),
            "NumPages": total_pages,
            "TotalResults": first_page_data.get("total_items", 0),
        }

    @mcp.tool
    def search_kql_query_with_timestamp(
        kql_query: str,
        begin_timestamp: datetime,
        end_timestamp: datetime,
        ctx: Context
    ) -> dict[str, object]:
        """Search the logs for the specified query and returns the latest 10 log messages
        matching the query (first page of results) and the total number of pages available

        Args:
         kql_query: KQL (Kabana Query Language) query to find matching log messages 
         begin_timestamp: Log messages with timestamp before begin_timestamp will not be matched
         end_timestamp: Log messages with timestamp after end_timestamp will not be matched.

        Returns:
         A dictionary with keys:
          - Page0: The latest 10 log messages matching the query and timestamp range
          - NumPages: Total number of pages that can be indexed
          - TotalResults: Total number of matching results
        """
        # Mark instructions as run for this session
        session = session_manager.get_or_create_session(ctx.session_id)
        session.flags["ran_instructions"] = True
        
        # TODO: Replace with actual log search implementation with timestamp filtering
        # This is a placeholder that generates sample log data
        sample_logs = [
            f"Log entry {i}: Query '{kql_query}' matched between {begin_timestamp.isoformat()} and {end_timestamp.isoformat()}"
            for i in range(35)  # Simulate 35 results (4 pages)
        ]
        
        # Cache the query results and get the first page
        first_page_data, total_pages = session_manager.cache_query_result(
            session_id=ctx.session_id,
            query=f"{kql_query} [timestamp: {begin_timestamp} to {end_timestamp}]",
            results=sample_logs,
        )
        
        return {
            "Page0": first_page_data.get("items", []),
            "NumPages": total_pages,
            "TotalResults": first_page_data.get("total_items", 0),
        }

    @mcp.tool
    def get_nth_page(page_index: int, ctx: Context) -> dict[str, Any]:
        """CLP:
        Retrieve the last paginated response by index.

        Args:
            page_index: The zero-based index of the page to retrieve.

        Returns:
            The part of the response at page index, or an error message if unavailable.
        """
        # Check if instructions have been run
        error_msg = ensure_instructions_run(ctx.session_id)
        if error_msg:
            return {"error": error_msg}
        
        # Get the requested page from the session manager
        page_data = session_manager.get_nth_page(ctx.session_id, page_index)
        
        # If there's an error, return it
        if "error" in page_data:
            return page_data
        
        # Return the page data with additional metadata
        return {
            "Page": page_data.get("items", []),
            "PageNumber": page_data.get("page_number", 0),
            "TotalPages": page_data.get("total_pages", 0),
            "HasNext": page_data.get("has_next", False),
            "HasPrevious": page_data.get("has_previous", False),
        }

    @mcp.tool
    def hello_world(name: str = "clp-mcp-server user") -> dict[str, Any]:
        """
        Provides a simple hello world greeting.

        :param name:
        :return: A greeting message to the given `name`.
        """
        return {
            "message": f"Hello World, {name.strip()}!",
            "server": ProtocolConstant.SERVER_NAME,
            "status": "running",
        }

    return mcp

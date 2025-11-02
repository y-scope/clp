# Connecting Cursor

This guide explains how to connect [Cursor][cursor] to the CLP MCP server over HTTP.

## Setup

1. Follow the [official Cursor guide][cursor-guide] to add a Remote MCP Server using `mcp.json`.
2. Add the following `clp-mcp-server` config to the `mcpServers` object and save the file.

    ```json
    {
       "mcpServers": {
          "clp-mcp-server": {
             "url": "http://<host>:<port>/mcp",
             "type": "http"
          }
       }
    }
    ```

    The `host` and `port` values should match the hostname and port number where the MCP server is
    running, as specified during
    [configuration](../guides-mcp-server/index.md#starting-the-mcp-server).

3. Enable and use the MCP server's tools as described in the [official Cursor
   documentation][cursor-using-mcp].

[cursor]: https://cursor.com/
[cursor-guide]: https://cursor.com/docs/context/mcp
[cursor-using-mcp]: https://cursor.com/docs/context/mcp#using-mcp-in-chat

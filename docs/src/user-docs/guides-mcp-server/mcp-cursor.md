# Connecting Cursor

This guide explains how to connect Cursor to the CLP MCP server over HTTP.

## Steps

1. Follow the [official Cursor guide](https://cursor.com/docs/context/mcp) to add a Remote MCP Server using `mcp.json`.
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
The `host` and `port` values should match the hostname and port number where the MCP server is running, as specified during [configuration](../guides-mcp-server/index.md#starting-mcp-server).

3. Restart Cursor. This applies the new configuration.
4. Enable and use the MCP server's tools as described in the [official Cursor documentation](https://cursor.com/docs/context/mcp#using-mcp-in-chat).
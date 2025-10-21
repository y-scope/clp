# Connecting Cursor
This guide explains how a user can connect Cursor to CLP MCP server over HTTP using a JSON configuration file.

## Steps
1. Follow the [official Cursor guide](https://cursor.com/docs/context/mcp) to add a Remote MCP Server using `mcp.json`. 
2. Add the following `clp-mcp-server` config to the `mcpServers` object and save the file.
```
{
	"mcpServers": {
	  "clp-mcp-server": {
		"url": "http://<host>:<port/mcp",
		"type": "http"
	  }
	}
  }
```
The `host` and the `port` are the hostname and the port number the MCP server is started on as specified [here](../guides-mcp-server/index.md#starting-mcp-server)

3. Restart Cursor
4. Enable and use the MCP server's tools as described in [here](https://cursor.com/docs/context/mcp#using-mcp-in-chat).
# Connecting Claude Desktop to CLP MCP server

This guide explains how a user can connect Claude Desktop to CLP MCP server over HTTP using a JSON configuration file.

## Prerequisites
1. Install Node.js >= 16 
2. Install the [mcp-remote] package 

## Steps
1. Open Claude Desktop settings: Navigate to the "Developer" tab in the sidebar.
TODO: add images
2. Click on "Edit Config" to edit `claude_desktop_config.json` file.
3. Add the server: Add an entry to the `mcpServers` object. If the `mcpServers` object does not exist, create one.
```
{
  "mcpServers": {
    "clp-mcp-server": {
      "command": "npx",
      "args": [
        "-y",
        "mcp-remote",
        "http://<host>:<port>/mcp",
        "--allow-http",
        "--transport",
        "http-first"
      ]
    }
  }
}
```
The `host` and the `port` are the hostname and the port number the MCP server is started on as specified [here](../guides-mcp-server/index.md#starting-mcp-server)

4. Save changes to the `claude_desktop_config.json` file
5. Restart Claude Desktop: Close and reopen the application for the new configuration to be applied.
6. The user should now be able to see the MCP server and the available tools under "Search and tools" icon in the Prompt window of Claude Desktop.

[mcp-remote]: https://www.npmjs.com/package/mcp-remote

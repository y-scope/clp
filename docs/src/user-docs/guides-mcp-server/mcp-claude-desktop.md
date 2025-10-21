# Connecting Claude Desktop

This guide explains how to connect Claude Desktop to the CLP MCP server over HTTP.

## Prerequisites

1. Install Node.js >= 16 
2. Install the [mcp-remote] package 

## Steps

1. Open Claude Desktop settings.

:::{image} ./claude-settings.png
:alt: Open settings
:::

2. Navigate to the "Developer" tab in the sidebar. Click on "Edit Config" to edit the `claude_desktop_config.json` file.
:::{image} ./claude-developer-menu.png
:alt: Navigate to the Developer tab
:::

3. Add the MCP server: Add an entry to the `mcpServers` object. If the `mcpServers` object does not exist, create one.

```json
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
The `host` and `port` values should match the hostname and port number where the MCP server is running, as specified during [configuration](../guides-mcp-server/index.md#starting-mcp-server). Check [mcp-remote]'s package description for additional arguments that can be specified.

4. Save the configuration: Save changes to the `claude_desktop_config.json` file
5. Restart Claude Desktop: Close and reopen the application for the new configuration to take effect.
6. You should now see the MCP server and its available tools under the “Search and Tools” icon in Claude Desktop’s Prompt window.

[mcp-remote]: https://www.npmjs.com/package/mcp-remote

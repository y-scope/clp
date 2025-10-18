# CLP MCP Server

The CLP MCP Server is a Python module that provides a [Model Context Protocol][mcp] (MCP) interface
for AI agents to invoke CLP operations.

## Build/Packaging

This module is built and packaged as a part of the CLP package.

## Connecting Agent to CLP MCP Server

Any MCP-compatible agent can connect to the CLP MCP server. This guide uses
[Claude Desktop][claude-desktop] as an example.

### Example: Connect Claude Desktop to CLP MCP Server

#### Prerequisites

* CLP package is running with the MCP server enabled.
* [Node.js][node-js] is installed.
* [mcp-remote] is installed and available via `npx`.

#### Configuration

Add the following snippet to your Claude Desktop settings file (`claude-settings.json`):

```json
{
  "mcpServers": {
    "clp": {
      "command": "npx",
      "args": [
        "mcp-remote",
        "http://0.0.0.0:8000/mcp"
      ]
    }
  }
}
```

* You can replace the server name (`clp`) with any name you prefer.
* Update the URL (<http://0.0.0.0:8000>) to the actual host or port where your CLP MCP server is
  running.

For more details on Claude Desktop MCP setup, see the
[official documentation][claude-desktop-mcp-doc].

## Available Tools

> **Warning:** 🚧 This section is still under construction.

## Testing

Use the following command to run all unit tests:

```shell
uv test pytest
```

[claude-desktop]: https://claude.ai/download
[claude-desktop-mcp-doc]: https://modelcontextprotocol.io/docs/develop/connect-local-servers
[mcp]: https://modelcontextprotocol.io/docs/getting-started/intro
[mcp-remote]: https://www.npmjs.com/package/mcp-remote
[node-js]: https://nodejs.org/en

# Connecting Visual Studio Code to CLP MCP server

This guide explains how a user can connect Visual Studio Code (VSCode) to CLP MCP server over HTTP using a JSON configuration file.

## Steps
1. Follow the [official VSCode guide](https://code.visualstudio.com/docs/copilot/customization/mcp-servers) to add an MCP server to VSCode.
2. The json configuration for CLP's MCP server would be:
```
"clp-mcp-server": {
    "type": "http",
    "url": "http://<host>:<port>/mcp"
}
```
The `host` and the `port` are the hostname and the port number the MCP server is started on as specified [here](../guides-mcp-server/index.md#starting-mcp-server)

3. Once you have added the MCP server, you can follow [this section](https://code.visualstudio.com/docs/copilot/customization/mcp-servers#_use-mcp-tools-in-agent-mode) to use the CLP MCP tools in agent mode.


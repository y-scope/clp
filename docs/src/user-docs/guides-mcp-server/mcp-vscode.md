# Connecting Visual Studio Code to CLP MCP server

This guide explains how a user can connect Visual Studio Code (VSCode) to CLP MCP server over HTTP using a JSON configuration file.

## Steps
1. Follow the [official VSCode guide](https://code.visualstudio.com/docs/copilot/customization/mcp-servers) to add an MCP server to VSCode.
2. The json configuration for CLP's MCP server would be:
```
"clp-mcp": {
    "type": "http",
    "url": "http://<server-ip>:<port-number>/mcp"
}
```
The `server-ip` and the `port-number` are the IP address and the port number the CLP MCP server is configured to run on. The default options are `localhost` and `8000`.

3. Once you have added the MCP server, you can follow [this section](https://code.visualstudio.com/docs/copilot/customization/mcp-servers#_use-mcp-tools-in-agent-mode) to use the CLP MCP tools in agent mode.


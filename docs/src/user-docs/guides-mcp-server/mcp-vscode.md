# Connecting Visual Studio Code

This guide explains how to connect [Visual Studio Code (VSCode)][vscode] to the CLP MCP server over
HTTP.

## Setup

1. Follow the [official VSCode guide][vscode-guide] to add an MCP server to VSCode.
2. The json configuration for CLP's MCP should be added to the `servers` object:

    ```json
    "servers": {
       "clp-mcp-server": {
          "type": "http",
          "url": "http://<host>:<port>/mcp"
       }
    }
    ```

    The `host` and `port` values should match the hostname and port number where the MCP server is
    running, as specified during
    [configuration](../guides-mcp-server/index.md#starting-the-mcp-server).

3. Once you have added the MCP server, you can follow [these instructions][vscode-using-mcp] to use
   the CLP MCP tools in agent mode.

[vscode]: https://code.visualstudio.com/
[vscode-guide]: https://code.visualstudio.com/docs/copilot/customization/mcp-servers
[vscode-using-mcp]: https://code.visualstudio.com/docs/copilot/customization/mcp-servers#_use-mcp-tools-in-agent-mode

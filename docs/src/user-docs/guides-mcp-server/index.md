# MCP Server
CLP supports an MCP server that allows AI agents like Claude Desktop, Cursor and Visual Studio Code's (VSCode) Copilot to query compressed logs.

This guide explains how to configure and run CLP's MCP server. It will also show you how to connect an AI agent to the MCP server.

## Prerequisites
This guide assumes:
1. You are able to configure, start, stop, and use CLP as described in the
   [clp-json quick-start guide](../quick-start/clp-json.md).
2. You have an Agent installed that supports connections to MCP servers over HTTP.

## Starting MCP Server:
1. Configure CLP package to run the MCP server by specifying the host and port number in `etc/clp-config.yml` as follows:
```
mcp_server:
   host: "<host-ip>"
   port: <port-number>
```
The default `<host-ip>` and `<port-number>` are `localhost` and `8000` respectively.

2. Start clp-json and compress the logs you want to query by following the [clp-json quick-start](../quick-start/clp-json.md). This will run the CLP MCP server on the specified `host` and `port`.

## Connecting to MCP Server:
The following subsections explain how to connect CLP's MCP server to each of the Agents:

::::{grid} 1 1 1 1
:gutter: 2

:::{grid-item-card}
:link: mcp-claude-desktop
Connecting Claude Desktop
^^^
Configuring Claude Desktop to connect to CLP MCP server
:::

:::{grid-item-card}
:link: mcp-cursor
Connecting Cursor
^^^
Configuring Cursor to connect to CLP MCP server
:::

:::{grid-item-card}
:link: mcp-vscode
Connecting VSCode
^^^
Configuring VSCode to connect to CLP MCP server
:::
::::

:::{caution}
CLP currently doesn't store timezone information in the compressed logs. All timestamps in the logs being compressed are assumed to be in UTC timezone. So you should specify the timezone of the compressed logs to the agent if they are not UTC. 
Support for encoding timezone information in the compressed logs will be available in a future release.
:::

:::{toctree}
:hidden:

mcp-claude-desktop
mcp-cursor
mcp-vscode
:::

# MCP Server

`clp-json` includes an [MCP server][mcp-server] that allows AI agents--such as [Claude
Desktop][claude-desktop], [Cursor][cursor], and [Visual Studio Code (VSCode)][vscode]--to query
CLP-compressed logs.

This guide explains how to configure and run CLP's MCP server, as well as how to connect an AI agent
to the MCP server.

:::{note}
Currently, support for the MCP server is only included with `clp-json` (not `clp-text`).
:::

## Prerequisites

This guide assumes:

1. you can configure, start, stop, and use `clp-json` as described in the
   [clp-json quick-start guide](../quick-start/clp-json.md).
2. you have an Agent installed that supports connections to MCP servers over HTTP.

## Starting the MCP Server

1. Configure `clp-json` to run the MCP server by uncommenting the `mcp_server` section in
   `etc/clp-config.yaml` and specifying a `host` and `port`. Replace the default values if
   necessary.

    ```yaml
    mcp_server:
       host: "localhost"
       port: 8000
       # other settings
    ```

    :::{note}
    Setting `mcp_server: null` or leaving the section commented will prevent the MCP server from
    starting with `clp-json`.
    :::

2. Start `clp-json` and compress the logs you want to query by following the [clp-json
   quick-start](../quick-start/clp-json.md) guide. This will run the CLP MCP server on the `host`
   and `port` you specified in step (1).

## Connecting to the MCP Server

See one of the following guides for how to connect CLP's MCP server to the agent of your choice.

:::{note}
Agents sometimes ignore MCP instructions even after making the `get_instructions` tool call. To
reduce erroneous queries from the agent, in your prompt, you can tell the agent to read the MCP
instructions carefully before analyzing any logs.
:::

::::{grid} 1 1 1 1
:gutter: 2

:::{grid-item-card}
:link: mcp-claude-desktop
Connecting Claude Desktop
^^^
Configuring Claude Desktop to connect to the CLP MCP server
:::

:::{grid-item-card}
:link: mcp-cursor
Connecting Cursor
^^^
Configuring Cursor to connect to the CLP MCP server
:::

:::{grid-item-card}
:link: mcp-vscode
Connecting VSCode
^^^
Configuring VSCode to connect to the CLP MCP server
:::
::::

## Limitations

CLP currently doesn't store time zone information in the compressed logs. This means that if the
logs you compressed were *not* in the UTC time zone, you will need to tell your agent what time zone
the logs were in originally. Support for encoding time zone information in the compressed logs will
be available in a future release.

:::{toctree}
:hidden:

mcp-claude-desktop
mcp-cursor
mcp-vscode
:::

[claude-desktop]: https://claude.com/product/overview
[cursor]: https://cursor.com/
[mcp-server]: https://modelcontextprotocol.io/docs/getting-started/intro
[vscode]: https://code.visualstudio.com/

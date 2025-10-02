# CLP MCP Server

A Model Context Protocol (MCP) server for AI to invoke CLP operations.

## Overview

This component provides an MCP server that integrates with the CLP package. The server runs over HTTP transport with sensible defaults for quick startup.

## Installation

Build the package using the taskfile:

```bash
task clp-mcp-server
```

## Usage

The server only supports HTTP transport. Host and port parameters are optional with defaults:

### Quick Start (using defaults)

```bash
python3 -m clp_mcp_server
```

This will start the server on `0.0.0.0:8000`.

### Custom Configuration

```bash
python3 -m clp_mcp_server --host 127.0.0.1 --port 3000
```


## Available Tools

The server currently provides these tools:

1. **get_server_info**: Get server information
   - Returns: Server name, version, and capabilities

2. **hello_world**: A simple greeting function
   - Parameters: `name` (optional, defaults to "World")
   - Returns: A greeting message with status information

## Connecting Agent to MCP Server

To connect Claude Desktop to the CLP MCP server, you need to configure the MCP client settings in your Claude Desktop configuration.

### Prerequisites

1. **Start the MCP Server**: First, ensure the CLP MCP server is running:
   ```bash
   python3 -m clp_mcp_server
   ```
   The server will start on `http://0.0.0.0:8000` by default.

2. **Install mcp-remote**: The configuration uses `mcp-remote` to connect to HTTP-based MCP servers:
   ```bash
   npm install -g mcp-remote
   ```

### Configuration

Add the following configuration to your Claude Desktop settings file (`claude-settings.json`):

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

### Configuration Details

- **Server Name** (`clp`): This is the connector's name that will appear in your Claude conversation interface. You'll see it displayed as an available MCP server that you can toggle on/off during conversations. When enabled, Claude can use CLP’s API tools. By default, all tools are active, but you can toggle them individually depending on your needs.

- **Command** (`npx`): Uses the Node.js package runner to execute the 'mcp-remote' package. This handles the process of connecting Claude Desktop to HTTP-based MCP servers.

- **Arguments**:
  - `mcp-remote`: The npm package that provides HTTP transport capabilities for MCP connections
  - `http://0.0.0.0:8000/mcp`: The full URL endpoint where your CLP MCP server is running and listening for requests




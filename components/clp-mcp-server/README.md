# CLP MCP Server

A minimal Model Context Protocol (MCP) server for CLP operations.

## Overview

This component provides an MCP server that integrates with the CLP package.

## Installation

Build the package using the taskfile:
```bash
task clp-mcp-server
```

## Usage

### Run with stdio transport (default)
```bash
clp-mcp-server
```

### Run with Server-Sent Events (SSE)
```bash
clp-mcp-server --sse
# Or specify host and port
clp-mcp-server --transport sse --host 0.0.0.0 --port 8080
```

### Run with HTTP transport
```bash
clp-mcp-server --transport http --port 8080
```

## Available Tools

The server currently provides these minimal tools:

1. **hello_world**: A simple greeting function
   - Parameters: `name` (optional, defaults to "World")
   - Returns: A greeting message

2. **get_server_info**: Get server information
   - Returns: Server name, version, and capabilities

## Development

To extend the server, add new tool functions in `server.py` within the `CLPMcpServer` function.

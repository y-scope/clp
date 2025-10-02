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


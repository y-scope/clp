# WebUI - Log Viewer

This component is part of the WebUI and enables log viewing service. It provides an interface for
viewing logs through the WebUI. It leverages Fastify for the server framework and includes plugins
for authentication and database management. Static files are served from the `client/dist` directory.

## Setup

### Prerequisites

- Node.js (v20)

### Environment Variables

The following environment variables need to be set:

- `CLP_DB_USER`: Database username for MySQL
- `CLP_DB_PASS`: Database password for MySQL
- 
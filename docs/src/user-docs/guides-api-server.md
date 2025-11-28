# API Server

API Server provides a RESTful interface for interacting with CLP.

## Starting the API Server

CLP starts the API Server based on the `api_server` section in `etc/clp-config.yaml`, which includes
a default configuration. You can uncomment and modify this section to override the defaults.

## Example: Submitting Search Queries and Receiving Results

API server exposes endpoints to submit search queries, and returns search results as a continuous
stream using [Server-sent Events][server-sent-events].

Assuming the server is running on the default host and port (`localhost:3001`), you can use the
following commands to submit a query to clp-json and stream the results.

1. Submit a search query:

   ```shell
   curl -X POST http://localhost:3001/query \
    -H "Content-Type: application/json" \
    -d '{
      "query_string": "*log*",
      "dataset": "default",
      "ignore_case": false,
      "max_num_results": 100,
      "write_to_file": false
    }'
   ```

   On success, the server responds with:

    ```json
    {
      "query_results_uri": "/query_results/100"
    }
    ```

2. Retrieve search results:
   Use the returned `query_results_uri` to receive search results as an SSE stream:

   ```bash
   curl -N http://localhost:3001/query_results/100
   ```

   Example streamed output:

   ```text
   data: {"timestamp": 1633036800, "message": "Example log message"}

   data: {"timestamp": 1633036810, "message": "Another matched log line"}

   data: {"timestamp": 1633036820, "message": "No logs found" }
   ```

## API Reference

All available API endpoints are defined in the [OpenAPI] Specification. You can explore the API
using [Swagger UI][swagger-ui].

[OpenAPI]: https://swagger.io/specification/
[server-sent-events]: https://developer.mozilla.org/en-US/docs/Web/API/Server-sent_events
[swagger-ui]: https://petstore.swagger.io/?url=https://docs.yscope.com/clp/main/_static/openapi.json

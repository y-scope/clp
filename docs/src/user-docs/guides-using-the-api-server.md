# Using the API server

CLP includes an API server that provides a RESTful interface for interacting with CLP.

## Starting the API server

CLP starts the API server based on the `api_server` section in `etc/clp-config.yaml`, which includes
a default configuration. You can uncomment and modify this section to override the defaults.

## API reference

All available API endpoints are defined in the [OpenAPI] Specification. You can explore the API
using [Swagger UI][swagger-ui].

:::{note}
`clp-text` doesn't support buffering search results in the file system or S3. When using `clp-text`,
set `output` to `{"type": "results_cache"}` so that results are buffered in `MongoDB` instead.
:::

## Example: Submitting search queries and receiving results

The API server exposes endpoints to submit search queries, and returns search results as a
continuous stream using [Server-sent Events][server-sent-events].

Assuming the server is running on the default host and port (`localhost:3001`), you can use the
following commands to submit a query to clp-json and stream the results.

1. Submit a search query:

   ```shell
   curl -X POST http://localhost:3001/query \
       -H "Content-Type: application/json" \
       -d '{
          "query": {
             "datasets": ["default"],
             "query_string": "*log*",
             "ignore_case": false
          },
          "output": {
             "type": "results_cache",
             "max_num_results": 100
          }
       }'
   ```

   `query` is the search to run, including the datasets to search within, which must not be empty.
   `output` chooses where the results are buffered: `{"type": "results_cache"}` buffers them in
   `MongoDB`, and `{"type": "file"}` buffers them in the file system or S3. Omit `max_num_results`
   to return all matching results. When specified, the limit must be a positive integer; zero is
   rejected. Only the `results_cache` output accepts a limit.

   On success, the server responds with:

    ```json
    {
      "query_results_uri": "query_results/100"
    }
    ```

2. Retrieve search results:
   Use the returned `query_results_uri` to receive search results as an SSE stream:

   ```bash
   curl -N http://localhost:3001/query_results/100
   ```

   Example streamed output:

   ```text
   data: {"timestamp": 1767225600000, "message": "Example log message"}

   data: {"timestamp": 1767225600010, "message": "Another matched log line"}

   data: {"timestamp": 1767225600020, "message": "No logs found" }
   ```

## Example: Streaming search results in a single request

The API server can also submit a search query and stream its results back in the same response,
while the query runs, without buffering them:

```shell
curl -N -X POST http://localhost:3001/query/stream \
    -H "Content-Type: application/json" \
    -d '{
       "query": {
          "datasets": ["default"],
          "query_string": "*log*",
          "ignore_case": false
       }
    }'
```

The request has only the `query` of `POST /query`; it takes no `output`.

The response is a stream of [Server-sent Events][server-sent-events]: a `job` event with the query
job's ID, one event per result, and an `end` event with the job's terminal status once the job has
terminated:

```text
event: job
data: {"query_job_id":100}

data: {"archive_id":"018e90e5-8b2a-4a61-a2fc-cac799936caf","timestamp_millisecs":1767225600000,"message":"Example log message"}

event: end
data: {"status":"Succeeded","num_results_emitted":1}
```

Results from different archives arrive in no particular order. A query job that fails still ends
the stream with an `end` event, whose `status` is `Failed`. If the API server can no longer stream
the query after its job was submitted (e.g., it can't read the job's status), an `error` event
replaces the `end` event, and the job's cancellation is requested. If the client disconnects before
the `end` event, the query job's cancellation is requested too.

[OpenAPI]: https://swagger.io/specification/
[server-sent-events]: https://developer.mozilla.org/en-US/docs/Web/API/Server-sent_events
[swagger-ui]: https://petstore.swagger.io/?url=https://docs.yscope.com/clp/DOCS_VAR_CLP_GIT_REF/_static/generated/api-server-openapi.json

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

Use `POST /query/stream` to submit a search query and receive results in the same response as the
query job runs in the background.

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

:::{tip}
Use the `-N` flag to disable curl's output buffering so you can see events as they arrive.
:::

The response uses [server-sent events][server-sent-events]. This example shows a search
that returns one result and finishes successfully.

```text
event: job
data: {"query_job_id":100}

data: {"archive_id":"018e90e5-8b2a-4a61-a2fc-cac799936caf","timestamp_millisecs":1767225600000,"message":"Example log message"}

event: end
data: {"status":"Succeeded","num_results_emitted":1}
```

* The `job` event arrives first and contains the query job ID.
* Each unnamed event carries one search result. Results from different archives can arrive in any
  order.
* The `end` event closes the stream and reports the job's terminal status.

For more details, see the [streaming search API reference][streaming-search-api].

:::{tip}
`sbin/search.sh` is a client of `POST /query/stream`: it prints the data of each result event as a
line of JSON (see [Searching from the command line][clp-json-cli-search]).
:::

[clp-json-cli-search]: quick-start/clp-json.md#searching-from-the-command-line
[OpenAPI]: https://swagger.io/specification/
[server-sent-events]: https://developer.mozilla.org/en-US/docs/Web/API/Server-sent_events
[streaming-search-api]: https://petstore.swagger.io/?url=https://docs.yscope.com/clp/DOCS_VAR_CLP_GIT_REF/_static/generated/api-server-openapi.json#/default/stream_query
[swagger-ui]: https://petstore.swagger.io/?url=https://docs.yscope.com/clp/DOCS_VAR_CLP_GIT_REF/_static/generated/api-server-openapi.json

# API Server for CLP

Explore the API with `http://<domain>/openapi.json` on [Swagger UI](https://petstore.swagger.io/).

## health

<a id="opIdhealth"></a>

`GET /health`

> Example responses

> 200 Response

```
"API server is running"
```

<h3 id="health-responses">Responses</h3>

|Status|Meaning|Description|Schema|
|---|---|---|---|
|200|[OK](https://tools.ietf.org/html/rfc7231#section-6.3.1)|none|string|

<aside class="success">
This operation does not require authentication
</aside>

## query

<a id="opIdquery"></a>

`POST /query`

> Body parameter

```json
{
  "begin_timestamp": 0,
  "dataset": "string",
  "end_timestamp": 0,
  "ignore_case": true,
  "max_num_results": 0,
  "query_string": "string",
  "write_to_file": true
}
```

<h3 id="query-parameters">Parameters</h3>

|Name|In|Type|Required|Description|
|---|---|---|---|---|
|body|body|[QueryConfig](#schemaqueryconfig)|true|none|

> Example responses

> 200 Response

```json
{
  "query_results_uri": "/query_results/1"
}
```

<h3 id="query-responses">Responses</h3>

|Status|Meaning|Description|Schema|
|---|---|---|---|
|200|[OK](https://tools.ietf.org/html/rfc7231#section-6.3.1)|none|Inline|
|500|[Internal Server Error](https://tools.ietf.org/html/rfc7231#section-6.6.1)|none|None|

<h3 id="query-responseschema">Response Schema</h3>

<aside class="success">
This operation does not require authentication
</aside>

## query_results

<a id="opIdquery_results"></a>

`GET /query_results/{search_job_id}`

<h3 id="query_results-parameters">Parameters</h3>

|Name|In|Type|Required|Description|
|---|---|---|---|---|
|search_job_id|path|integer(int64)|true|none|

> Example responses

> 200 Response

<h3 id="query_results-responses">Responses</h3>

|Status|Meaning|Description|Schema|
|---|---|---|---|
|200|[OK](https://tools.ietf.org/html/rfc7231#section-6.3.1)|none|string|
|500|[Internal Server Error](https://tools.ietf.org/html/rfc7231#section-6.6.1)|none|None|

<aside class="success">
This operation does not require authentication
</aside>

## Schemas

<h2 id="tocS_QueryConfig">QueryConfig</h2>
<!-- backwards compatibility -->
<a id="schemaqueryconfig"></a>
<a id="schema_QueryConfig"></a>
<a id="tocSqueryconfig"></a>
<a id="tocsqueryconfig"></a>

```json
{
  "begin_timestamp": 0,
  "dataset": "string",
  "end_timestamp": 0,
  "ignore_case": true,
  "max_num_results": 0,
  "query_string": "string",
  "write_to_file": true
}

```

Defines the request configuration for submitting a search query.

### Properties

|Name|Type|Required|
|---|---|---|
|begin_timestamp|integer,null(int64)|false|
|dataset|string,null|false|
|end_timestamp|integer,null(int64)|false|
|ignore_case|boolean|false|
|max_num_results|integer(int32)|false|
|query_string|string|true|
|write_to_file|boolean|false|


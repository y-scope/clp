# Using log-ingestor

`clp-json` includes a component called `log-ingestor` that enables users to ingest logs into CLP 
archives by creating and managing ingestion jobs.


:::{note}
Currently, `log-ingestor` only supports [clp-json][release-choices] when configured with S3 log
inputs. The following capabilities are not yet supported but are planned for future releases:

* One-time ingestion jobs (similar to the existing compression CLI workflows)
* Ingesting from local filesystems
* Ingestion using `clp-text`
:::

## Starting log-ingestor

`clp-json` starts `log-ingestor` based on the `log_ingestor` section in `etc/clp-config.yaml` if
`logs_input` is configured to be `"s3"` type. You can uncomment and modify this section to override
the defaults.

## Ingestion jobs

`log-ingestor` manages log ingestion through ingestion jobs. An ingestion job continuously monitors
a configured log source, buffers incoming log data, and groups it into compression jobs. This
buffering and batching strategy improves compression efficiency and reduces overall storage
overhead.

### Ingestion job orchestration

`log-ingestor` exposes **RESTfuls APIs** for ingestion job orchestrations. You can explore all
available endpoints and their schemas using [Swagger UI][swagger-ui-all].

:::{note}
Currently, requests to `log-ingestor` must be sent directly to the log-ingestor service. In future
releases, these requests will be routed through the API server.
:::

### Fault tolerance

:::{warning}
The current version of `log-ingestor` does **not provide fault tolerance**.

If `log-ingestor` crashes or is restarted, all in-progress ingestion jobs and their associated state
are lost and must be recreated manually.

Robust fault tolerance for the ingestion pipeline is planned and will be introduced in future
releases.
:::

## Continuous ingestion from S3

`log-ingestor` supports **continuous ingestion jobs** for ingesting logs from S3-compatible object
storage. Currently, two types of ingestion jobs are available:

* [S3 scanner](#s3-scanner): Periodically scans a specified S3 bucket and prefix for new log files
  to ingest.
* [SQS listener](#sqs-listener): Listens to an SQS queue for notifications about newly created log
  files in S3.

### S3 scanner

An S3 scanner ingestion job periodically scans a specified S3 bucket and key prefix for new log
files to ingest. The scan interval and other parameters can be configured when creating the job.

For configuration details and request body, see the
[API reference for creating S3 scanner ingestion jobs][s3-scanner-api].

:::{important}
To ensure correct and efficient ingestion, the scanner relies on the following assumptions:

* **Lexicographical order**: New objects are added in lexicographical order based on their keys. For
  example, objects with keys `log1` and `log2` will be ingested sequentially. If a new object with 
  key `log0` is added after `log2`, it will be ignored because it is not lexicographically greater
  than the last ingested key.
* **Immutability**: Objects under the specified prefix are immutable. Once an object is created, it
  is not modified or overwritten.
:::

### SQS listener

[AWS SQS][aws-sqs] can be configured to receive S3 event notifications when new objects are created.
For details on configuring S3 event notifications for SQS, see the
[AWS documentation][aws-s3-event-notifications].

An SQS listener ingestion job listens to a specified SQS queue and ingests S3 objects referenced
by incoming notifications.

For configuration details and request body, see the
[API reference for creating SQS listener ingestion jobs][sqs-listener-api].

:::{important}
To ensure correct and efficient ingestion, the listener relies on the following assumptions:

* **Dedicated queue**: The given SQS queue must be dedicated to this ingestion job. No other
  consumers should read from or delete messages in the queue. The ingestion job must have permission
  to delete messages after they are successfully processed.
* **Immutability**: Objects under the specified prefix are immutable. Once an object is created, it
  is not modified or overwritten.
:::

:::{note}
Currently, an SQS listener ingestion job only supports ingesting objects from a single S3 bucket and
prefix. Support for multiple buckets or prefixes will be added in future releases.
:::

:::{note}
Currently, the SQS listener currently does not support custom S3 endpoint configurations. Support
for custom endpoints will be added in future releases.
:::

[aws-s3-event-notifications]: https://docs.aws.amazon.com/AmazonS3/latest/userguide/ways-to-add-notification-config-to-bucket.html#step2-enable-notification
[aws-sqs]: https://aws.amazon.com/sqs/
[release-choices]: ./quick-start/index.md#choosing-a-flavor
[s3-scanner-api]: https://petstore.swagger.io/?url=https://docs.yscope.com/clp/DOCS_VAR_CLP_GIT_REF/_static/log-ingestor-openapi.json#/IngestionJob/create_s3_scanner_job
[sqs-listener-api]: https://petstore.swagger.io/?url=https://docs.yscope.com/clp/DOCS_VAR_CLP_GIT_REF/_static/log-ingestor-openapi.json#/IngestionJob/create_sqs_listener_job
[swagger-ui-all]: https://petstore.swagger.io/?url=https://docs.yscope.com/clp/DOCS_VAR_CLP_GIT_REF/_static/log-ingestor-openapi.json

# Using `log-ingestor`

`log-ingestor` is a CLP component that facilitates continuous log ingestion from a configured log
source.

:::{note}
Currently, `log-ingestor` can only be used by
[`clp-json` CLP deployments](./quick-start/clp-json.md) that are configured for S3 object storage.
To learn how to set up this configuration, check out the
[object storage guide](./guides-using-object-storage/index.md).

Support for ingestion from local filesystems and ingestion using `clp-text` is planned for a future
release.
:::

---

## Starting `log-ingestor`

`clp-json` will spin up `log-ingestor` on startup as long as the `logs_input` field in the CLP
package's config file (`clp-package/etc/clp-config.yaml`) is configured for object storage.

If you'd like, you can further configure `log-ingestor` by modifying the `log_ingestor` field in
the same file.

---

## Ingestion jobs

`log-ingestor` facilitates continuous log ingestion with **ingestion jobs**. An ingestion job
continuously monitors a configured log source, buffers incoming log data, and groups it into
compression jobs. This buffering and batching strategy improves compression efficiency and reduces
overall storage overhead.

:::{note}
Support for one-time ingestion jobs (similar to the current CLP compression CLI workflows) is
planned for a future release.
:::

### Interacting with `log-ingestor`

`log-ingestor` exposes **RESTful APIs** that allow you to write ingestion jobs to configure what
`log-ingestor` should ingest (for example, the S3 bucket and key prefix), manage ingestion jobs, and
check `log-ingestor` health.

You can explore all available endpoints and their schemas at the
[Swagger UI `log-ingestor` page][swagger-ui-all].

:::{note}
Currently, requests to `log-ingestor` must be sent directly to the `log-ingestor` service. Requests
will be routed through CLP's [API server](./guides-using-the-api-server.md) in a future release.
:::

### Fault tolerance

:::{warning}
**The current version of `log-ingestor` does not provide fault tolerance.**

If `log-ingestor` crashes or is restarted, all in-progress ingestion jobs and their associated state
will be lost, and must be restored manually. Robust fault tolerance for the ingestion pipeline is
planned for a future release.
:::

---

## Continuous ingestion from S3

`log-ingestor` supports **continuous ingestion jobs** for ingesting logs from S3-compatible object
storage. Currently, two types of ingestion jobs are available:

* [**S3 scanner**](#s3-scanner): Periodically scans a specified S3 bucket and prefix for new log
  files to ingest.
* [**SQS listener**](#sqs-listener): Listens to an SQS queue for notifications about newly created
  log files in S3.

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

An SQS listener ingestion job listens to a specified AWS SQS queue and ingests S3 objects referenced
by incoming notifications. For details on configuring S3 event notifications for SQS, see the
[AWS documentation][aws-s3-event-notifications].

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
SQS listener ingestion jobs carry the following limitations:

* An SQS listener ingestion job can only ingest objects from a single S3 bucket and prefix. Support
  for multiple buckets or prefixes is planned for a future release.
* SQS listener ingestion jobs do not support custom S3 endpoint configurations. Support for custom
  endpoints is planned for a future release.
:::

[aws-s3-event-notifications]: https://docs.aws.amazon.com/AmazonS3/latest/userguide/ways-to-add-notification-config-to-bucket.html#step2-enable-notification
[s3-scanner-api]: https://petstore.swagger.io/?url=https://docs.yscope.com/clp/DOCS_VAR_CLP_GIT_REF/_static/generated/log-ingestor-openapi.json#/IngestionJob/create_s3_scanner_job
[sqs-listener-api]: https://petstore.swagger.io/?url=https://docs.yscope.com/clp/DOCS_VAR_CLP_GIT_REF/_static/generated/log-ingestor-openapi.json#/IngestionJob/create_sqs_listener_job
[swagger-ui-all]: https://petstore.swagger.io/?url=https://docs.yscope.com/clp/DOCS_VAR_CLP_GIT_REF/_static/generated/log-ingestor-openapi.json

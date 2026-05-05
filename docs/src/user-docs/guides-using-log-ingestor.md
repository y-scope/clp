# Using `log-ingestor`

`log-ingestor` is a CLP component that facilitates continuous log ingestion from a given log source.

:::{note}
Currently, `log-ingestor` can only be used by [`clp-json`](./quick-start/clp-json.md) deployments
that are configured for S3 object storage. To set up this configuration, check out the
[object storage guide](./guides-using-object-storage/index.md).

Support for ingestion from local filesystems, or for using `clp-text`, is planned for a future
release.
:::

---

## Starting `log-ingestor`

`clp-json` will spin up `log-ingestor` on startup as long as the `logs_input` field in the CLP
package's config file (`clp-package/etc/clp-config.yaml`) is
[configured for object storage][clp-s3-logs-input-config].

You can specify a custom configuration for `log-ingestor` by modifying the `log_ingestor` field in
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

`log-ingestor` exposes **RESTful APIs** that allow you to submit ingestion jobs, manage ingestion
jobs, and check `log-ingestor`'s health.

You can explore all available endpoints and their schemas at the
[Swagger UI `log-ingestor` page][swagger-ui-all].

:::{note}
Currently, requests to `log-ingestor` must be sent directly to the `log-ingestor` service. Requests
will be routed through CLP's [API server](./guides-using-the-api-server.md) in a future release.
:::

### Fault tolerance

`log-ingestor` is designed to tolerate unexpected crashes or restarts without losing information
about ingestion jobs or the files that have been submitted for compression. Note that this does not
include fault tolerance of the components external to `log-ingestor`. Specifically, `log-ingestor`
guarantees the following, even in the presence of crashes or restarts of `log-ingestor`:

* Any ingestion job successfully submitted to `log-ingestor` will run continuously.
* Within an ingestion job, any files that have been found on S3 or received as messages from SQS
  queue will eventually be submitted for compression.

:::{note}
`log-ingestor` **DOES NOT** guarantee the following after a crash or restart:

* Any file submitted for compression (that can be compressed successfully) will eventually be
  compressed successfully.
  * This is because failures of the compression cluster are external to `log-ingestor`. Future
    versions of CLP will address this limitation.
* Any file submitted for compression will *only* be compressed once.
  * This is because for [SQS listener](#sqs-listener) ingestion jobs, the processes for deleting
    messages from the SQS queue and recording the files for ingestion are not synchronized. As a
    result, a failure during this process may cause the same file to be ingested multiple times.
:::

---

## Continuous ingestion from S3

`log-ingestor` supports **continuous ingestion jobs** for ingesting logs from S3-compatible object
storage. Currently, two types of ingestion jobs are available:

* [**S3 scanner**](#s3-scanner): Periodically scans an S3 bucket and prefix for new log files to
  ingest.
* [**SQS listener**](#sqs-listener): Listens to an [SQS queue][sqs] for notifications about newly
  created log files in S3.

### S3 scanner

An S3 scanner ingestion job periodically scans a specified S3 bucket and key prefix for new log
files to ingest. The scan interval and other parameters can be configured when creating the job.

For configuration details and the request body, see the
[API reference for creating S3 scanner ingestion jobs][s3-scanner-api].

:::{important}
To ensure correct and efficient ingestion, the scanner relies on the following assumptions:

* **Lexicographical order**: Every new object added to the S3 bucket has a key that is
  lexicographically greater than the previously added object. For example, objects with keys `log1`
  and `log2` will be ingested sequentially. If a new object with key `log0` is added after `log2`,
  it will be ignored because it is not lexicographically greater than the last ingested key.
* **Immutability**: Objects under the specified prefix are immutable. Once an object is created, it
  is not modified or overwritten.
:::

### SQS listener

An SQS listener ingestion job listens to a specified AWS SQS queue and ingests S3 objects referenced
by incoming notifications. For details on configuring S3 event notifications for SQS, see the
[AWS documentation][aws-s3-event-notifications].

For configuration details and the request body, see the
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
[clp-s3-logs-input-config]: ./guides-using-object-storage/aws-s3/clp-config.md#configuration-for-input-logs
[s3-scanner-api]: https://petstore.swagger.io/?url=https://docs.yscope.com/clp/DOCS_VAR_CLP_GIT_REF/_static/generated/log-ingestor-openapi.json#/IngestionJob/create_s3_scanner_job
[sqs]: https://docs.aws.amazon.com/sqs/
[sqs-listener-api]: https://petstore.swagger.io/?url=https://docs.yscope.com/clp/DOCS_VAR_CLP_GIT_REF/_static/generated/log-ingestor-openapi.json#/IngestionJob/create_sqs_listener_job
[swagger-ui-all]: https://petstore.swagger.io/?url=https://docs.yscope.com/clp/DOCS_VAR_CLP_GIT_REF/_static/generated/log-ingestor-openapi.json

# Using CLP with AWS S3

To compress logs from AWS S3, follow the steps in the section below. For all other operations (e.g.,
searching or viewing logs in the Web UI), use CLP as described in the
[clp-json quick-start guide](../../quick-start/clp-json.md).

## Compressing logs from AWS S3

To compress logs from AWS S3, create an ingestion job with [`log-ingestor`][log-ingestor-guide]. An
ingestion job continuously finds new log objects in an S3 bucket and key prefix, and submits them
for compression. `log-ingestor` supports the following types of ingestion jobs:

* [**S3 scanner**][s3-scanner]: Periodically scans an S3 bucket and key prefix for new log objects.
* [**SQS listener**][sqs-listener]: Listens to an SQS queue for notifications about new log objects.

The key prefix of an ingestion job must begin with the `<all-logs-prefix>` value from the
[compression IAM policy][compression-iam-policy].

:::{note}
CLP doesn't support one-time compression of S3 objects.
:::

[compression-iam-policy]: ./aws-s3-config.md#configuration-for-compression
[log-ingestor-guide]: ../../guides-using-log-ingestor.md
[s3-scanner]: ../../guides-using-log-ingestor.md#s3-scanner
[sqs-listener]: ../../guides-using-log-ingestor.md#sqs-listener

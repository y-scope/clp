# Using CLP with AWS S3

To compress logs from AWS S3, follow the steps in the section below. For all other operations (e.g.,
searching or viewing logs in the Web UI), use CLP as described in the
[clp-json quick-start guide](../../quick-start/clp-json.md).

## Compressing logs from AWS S3

Use [log-ingestor](../../guides-using-log-ingestor.md) to ingest and compress logs from AWS S3.
Create an [S3 scanner ingestion job](../../guides-using-log-ingestor.md#s3-scanner) for the bucket and
key prefix containing the logs.

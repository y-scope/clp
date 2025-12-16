# Using log-ingestor

`clp-json` includes a component called `log-ingestor` that enables users to ingest logs into CLP 
archives by creating and managing ingestion jobs.

An ingestion job continuously monitors a configured log source, buffers incoming log data, and
batches it into compression jobs. This buffering and batching process improves compression
efficiency and reduces overall storage overhead.

:::{note}
Currently, `log-ingestor` only supports [clp-json][release-choices] when configured with S3 log
inputs. The following capabilities are not yet supported but are planned for future releases:

* One-time ingestion jobs (similar to the existing compression CLI workflows)
* Ingesting from local filesystems
* Ingestion using `clp-text`
:::

## Starting log-ingestor

CLP starts log-ingestor based on the `log_ingestor` section in `etc/clp-config.yaml` if `logs_input`
are configured to be `"s3"` type. You can uncomment and modify this section to override the 
defaults.

[release-choices]: ./quick-start/index.md#choosing-a-flavor

# Using `compression-coordinator`

`compression-coordinator` is the CLP component that coordinates compression jobs. It submits each
compression job to [Spider][spider], which runs the job's tasks on its workers. For Spider's
settings, see [Configuring Spider][configuring-spider].

## Features

`compression-coordinator` provides the following user-experience, reliability, and performance
features:

* **All-or-nothing semantics**: Compression jobs publish all archive metadata to the
  `clp_<dataset>_archives` table in a single commit operation. The commit operation is both
  transactional and idempotent, so:
  * a job-level failure doesn't result in partial updates.
  * internal task retries don't result in duplicate updates.

  :::{warning}
  The **all-or-nothing semantics** do not apply to the column metadata table
  (`clp_<dataset>_column_metadata`), since it's still updated during compression job execution. So
  if a compression job fails, this table may contain partial updates. This limitation is tracked in
  [y-scope/clp#2480][column-metadata-issue].
  :::

* **Failure recovery**: After `compression-coordinator` or Spider fails and restarts, it can resume
  in-progress jobs.
* **Automatic retries**: If a compression task fails, it can automatically be retried, allowing
  tasks to automatically recover from transient failures.
  * To configure the maximum number of retries for compression and commit tasks, set
    `compression_task_max_retry` and `commit_task_max_retry`, respectively, in the
    `compression-coordinator` configuration.
* **Fairness between concurrent compression jobs**: Spider processes active jobs in round-robin
  order, at the granularity of individual *tasks*.
  * To configure the maximum number of compression jobs that make progress concurrently in Spider,
    set {confval}`active_job_list_capacity`.
* **Bounded job concurrency**: `compression-coordinator` limits the number of compression jobs that
  can be submitted to Spider concurrently.
  * To configure the maximum number of compression jobs that `compression-coordinator` submits to
    Spider concurrently, set `max_concurrent_jobs` in the `compression-coordinator` configuration.

## Limitations

`compression-coordinator` currently has the following limitations:

* It's only available when using `clp-json`.
* It only compresses logs from S3 in compression jobs created by [`log-ingestor`][log-ingestor].
  One-time compression of S3 objects isn't supported.

[column-metadata-issue]: https://github.com/y-scope/clp/issues/2480
[configuring-spider]: guides-using-spider-scheduler.md
[log-ingestor]: guides-using-log-ingestor.md
[spider]: https://github.com/y-scope/spider

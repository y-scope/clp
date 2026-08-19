# Using `compression-coordinator`

`compression-coordinator` is a new component that coordinates compression jobs scheduled via
[Spider][spider]. `compression-scheduler` is designed to replace the existing
`compression-scheduler` which schedules compression jobs using Celery.

To use `compression-coordinator`, enable the Spider scheduling framework when starting the CLP
package.

TODO: Link to Chenxing's doc.

:::{note}
Currently, `compression-coordinator` can be deployed via Kubernetes and not via Docker Compose.
Support for Docker Compose is planned for a future release.
:::

Compared with the `compression-scheduler`-based architecture (which uses Celery),
the `compression-coordinator`-based architecture (which uses Spider) provides the following

* **Improved all-or-nothing semantics**: Compression jobs coordinated by `compression-coordinator`
  publish all archive metadata to the `clp_<dataset>_archives` table in a single dedicated commit
  operation. The commit operation is both transactional and idempotent, so:
  * a job-level failure doesn't result in partial updates.
  * internal task retries don't result in duplicate updates.

  :::{warning}
  The **all-or-nothing semantics** do not apply to the column metadata table
  (`<dataset>_column_metadata`), because it is updated during compression job execution. If a
  compression job fails, this table may contain partial updates. This known limitation is tracked in
  [this GitHub issue][column-metadata-issue].
  :::

* **Improved failure recovery**:
  * After a failure and restart, `compression-coordinator` can resume in-progress jobs, whereas
    `compression-scheduler` assumes that in-progress jobs have hung and so it kills them.
  * After a failure and restart, Spider can resume in-progress jobs, whereas Celery cannot since
    the compression tasks aren't idempotent.
* **Easier failure handling**: With `compression-coordinator` and Spider, if a compression task
  fails, it can automatically be retried, allowing tasks to automatically recover from transient
  failures.
  * TODO: Link to #2457.
* **Improved resource utilization**: `compression-scheduler` processes tasks in batches, where each
  batch must wait for its slowest task to finish. `compression-coordinator` instead schedules
  individual tasks through Spider, allowing resources to be reassigned as soon as individual tasks
  finish.
* **Improved fairness between concurrent compression jobs**: Both architectures process jobs in
  round-robin order, but the `compression-coordinator`-based architecture does so at the granularity
  of tasks whereas the `compression-scheduler`-based architecture does so at the granularity of
  batches of tasks. If `max_concurrent_tasks_per_job` is set to 1, the
  `compression-scheduler`-based architecture can achieve the same level of fairness as the
  `compression-coordinator`-based architecture, but with higher overhead since Spider is more
  efficient than `compression-scheduler`.
* **Bounded job concurrency**: `compression-coordinator` limits the number of compression jobs that
  can be submitted to Spider concurrently, whereas `compression-scheduler` doesn't; the latter can
  cause significantly high scheduling overheads when there are many concurrent jobs.

:::{note}
`compression-coordinator` currently has the following functional limitations:

* It's only available when using `clp-json`.
* It only handles compression jobs created by `log-ingestor`.
* It only supports writing archives to S3.

Additional capabilities will be introduced in future releases as it moves toward feature parity with
`compression-scheduler`.
:::

:::{note}
`compression-scheduler` is planned for deprecation and will eventually be fully replaced by
`compression-coordinator`. Currently, `compression-scheduler` continues to run alongside
`compression-coordinator` to handle compression jobs that `compression-coordinator` doesn't yet
support.
:::

[column-metadata-issue]: https://github.com/y-scope/clp/issues/2480
[spider]: https://github.com/y-scope/spider

# Using `compression-coordinator`

`compression-coordinator` is a new component that coordinates compression jobs scheduled via
[Spider][spider]. `compression-coordinator` is designed to replace the existing
`compression-scheduler` which schedules compression jobs using Celery.

:::{note}
`compression-coordinator` has not yet reached feature-parity with `compression-scheduler` (see
the [limitations](#limitations) section for details). Until then, the two are meant to run
side-by-side, with `compression-scheduler` handling any jobs that `compression-coordinator` doesn't
yet support.
:::

To use `compression-coordinator`, enable the Spider scheduling framework when starting the CLP
package. See [Using Spider scheduler][using-spider-scheduler] for details.

## Benefits

Compared with the `compression-scheduler`-based architecture (which uses Celery),
the `compression-coordinator`-based architecture (which uses Spider) provides the following
user-experience, reliability, and performance improvements:

* **Improved all-or-nothing semantics**: Compression jobs coordinated by `compression-coordinator`
  publish all archive metadata to the `clp_<dataset>_archives` table in a single commit operation.
  The commit operation is both transactional and idempotent, so:
  * a job-level failure doesn't result in partial updates.
  * internal task retries don't result in duplicate updates.

  :::{warning}
  The **all-or-nothing semantics** do not apply to the column metadata table
  (`clp_<dataset>_column_metadata`), since it's still updated during compression job execution. So
  if a compression job fails, this table may contain partial updates. This limitation is tracked in
  [y-scope/clp#2480][column-metadata-issue].
  :::

* **Improved failure recovery**:
  * After `compression-coordinator` fails and restarts, it can resume in-progress jobs, whereas
    `compression-scheduler` assumes that in-progress jobs have hung and so it kills them.
  * After Spider fails and restarts, it can resume in-progress jobs, whereas Celery cannot since the
    compression tasks aren't idempotent.
* **Easier failure handling**: With `compression-coordinator` and Spider, if a compression task
  fails, it can automatically be retried, allowing tasks to automatically recover from transient
  failures.
  * To configure the maximum number of retries for compression and commit tasks, set
    `compression_task_max_retry` and `commit_task_max_retry`, respectively, in the
    `compression-coordinator` configuration.
* **Improved horizontal scalability**: With Celery, tasks are dispatched to the task queue serially,
  whereas with Spider, tasks are dispatched in parallel. Thus, if we add more workers, at some
  point, Spider scales better whereas Celery gets bottlenecked by its task-dispatching overhead.
* **Improved fairness between concurrent compression jobs**: Both architectures process jobs in
  round-robin order, but the `compression-coordinator`-based architecture does so at the granularity
  of *tasks* whereas the `compression-scheduler`-based architecture does so at the granularity of
  *batches of tasks*. If `max_concurrent_tasks_per_job` is set to `1`, the
  `compression-scheduler`-based architecture can achieve the same level of fairness as the
  `compression-coordinator`-based architecture, but with higher overhead since Spider is more
  efficient than `compression-scheduler`.
  * To configure the maximum number of jobs Spider schedules concurrently, set
    {confval}`spider.spiderConfig.scheduler.config.active_job_queue_size`.
* **Bounded job concurrency**: `compression-coordinator` limits the number of compression jobs that
  can be submitted to Spider concurrently, whereas `compression-scheduler` doesn't; the latter can
  cause significantly high scheduling overheads when there are many concurrent jobs.
  * To configure the maximum number of compression jobs that `compression-coordinator` submits to
    Spider concurrently, set `max_concurrent_jobs` in the `compression-coordinator` configuration.

## Limitations

`compression-coordinator` currently has the following limitations.

### Functional limitations

* It's only available when using `clp-json`.
* It only handles compression jobs created by `log-ingestor`.
* It only supports writing archives to S3.

Additional capabilities will be introduced in future releases as it moves toward feature parity with
`compression-scheduler`.

### Deployment limitations

Currently, `compression-coordinator` can be deployed via Kubernetes and not via Docker Compose.
Support for Docker Compose is planned for a future release.

[column-metadata-issue]: https://github.com/y-scope/clp/issues/2480
[spider]: https://github.com/y-scope/spider
[using-spider-scheduler]: guides-using-spider-scheduler.md

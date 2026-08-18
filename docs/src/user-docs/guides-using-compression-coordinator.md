# Using `compression-coordinator`

`compression-coordinator` is a new component that coordinates compression jobs scheduled via
[Spider][spider]. It's designed to replace the existing `compression-scheduler`, which schedules
compression jobs using Celery.

To use `compression-coordinator`, enable the Spider scheduling framework when starting the CLP
package.

TODO: Link to Chenxing's doc.

:::{note}
Currently, `compression-coordinator` can be deployed only on Kubernetes using the CLP Helm chart.
Support for Docker Compose deployments is planned for a future release.
:::

Compared with `compression-scheduler`, `compression-coordinator` provides the following
user-experience, reliability, and performance improvements:

* **Automatic failure recovery**:
  * `compression-coordinator` automatically resumes previously submitted jobs (users don't need to
    restart them manually).
  * Spider's fault-tolerance model allows it to recover transparently from internal failures without
    requiring user intervention.
* **Easier failure handling via a configurable retry policy**: `compression-coordinator` allows
  users to configure the maximum number of retries for each compression task (retries are
  unconditional). This can help tasks recover automatically from transient failures, such as
  temporary network connectivity issues.
  * TODO: Link to #2457.
* **Improved resource utilization**: `compression-scheduler` processes tasks in batches, where each
  batch must wait for its slowest task to finish. `compression-coordinator` instead schedules
  individual tasks through Spider, allowing resources to be reassigned as soon as individual tasks
  finish.
* **Bounded job concurrency**: `compression-coordinator` limits the number of compression jobs that
  can be submitted to Spider concurrently. In contrast, `compression-scheduler` does not bound the
  number of concurrent jobs, which can cause scheduling overhead to grow significantly at high
  levels of job concurrency.
* **Improved fairness between concurrent compression jobs**: Compression jobs submitted by
  `compression-coordinator` are admitted into a bounded set of active jobs. Tasks from these active
  jobs are scheduled in round-robin order, while additional submitted jobs remain pending until an
  active slot becomes available.
  * `compression-scheduler` achieves similar job-level fairness only when
    `max_concurrent_tasks_per_job` is set to 1, effectively scheduling one task per job in
    round-robin order. However, this round-robin scheduling spans an unbounded number of concurrent
    jobs and can incur significant overhead at high concurrency. Spider provides similar fairness
    with much lower scheduling overhead through a more efficient architecture and a bounded set of
    active jobs.
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

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

* **Automatic failure recovery**: If a service fails or restarts, `compression-coordinator`
  automatically resumes previously submitted jobs (users don't need to restart them manually).
* **Improved resource utilization**: Compression jobs run in a Spider-managed execution environment,
  allowing computational resources to be shared more effectively across all configured workers.
* **Easier failure handling via a configurable retry policy**: `compression-coordinator` allows
  users to configure a simple retry policy for failed compression tasks. Automatic retries can help
  recover from transient issues, such as temporary network interruptions, without requiring user
  configure a simple retry policy for failed compression tasks. Automatic retries can help recover
  from transient issues, such as temporary network interruptions, without requiring user
  intervention.
  * TODO: Configure retry policy through <LINK>
* **Improved fairness between concurrent compression jobs**: `compression-coordinator` provides
  two levels of concurrency control to improve fairness among compression jobs that run
  concurrently:
  * Coordinator-side rate limit: TODO, depends on #2435.
  * Spider-side active job limit: Controls the maximum number of jobs that can make progress
    concurrently while sharing compute resources in Spider. Jobs are admitted on a first-come,
    first-served basis.
* **Improved all-or-nothing semantics**: Compression jobs coordinated by `compression-coordinator`
  publish their results all at once via a dedicated commit operation. The commit operation is both
  transactional and idempotent, so:
  * a job-level failure doesn't result in partial updates.
  * internal task retries don't result in duplicate updates.

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

[spider]: https://github.com/y-scope/spider

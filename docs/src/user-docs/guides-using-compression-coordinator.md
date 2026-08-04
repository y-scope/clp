# Using `compression-coordinator`

`compression-coordinator` is a new component that coordinates compression jobs on [Spider][spider].
It is designed to replace the existing `compression-scheduler`, which schedules compression jobs
using Celery.

To use `compression-coordinator`, enable the Spider scheduling framework when starting the CLP
package.

TODO: Link to Chenxing's doc.

:::{note}
Currently, `compression-coordinator` can be deployed only on Kubernetes using the CLP Helm chart.
Support for Docker Compose deployments is planned for a future release.
:::

Compared with `compression-scheduler`, `compression-coordinator` provides the following improvements
to the user experience, reliability, and performance:

* **Automatic failure recovery**: If a service fails or restarts, `compression-coordinator`
  automatically resumes previously submitted jobs. Users do not need to restart them manually.
* **Improved resource utilization**: Compression jobs run in a Spider-managed execution environment,
  allowing computational resources to be shared more effectively across all configured workers.
* **Configurable retries for compression failures**: `compression-coordinator` allows users to
  configure a simple retry policy for failed compression tasks. Automatic retries can help recover
  from transient issues, such as temporary network interruptions, without requiring user
  intervention.
  * TODO: Configure retry policy through <LINK>
* **Improved fairness across concurrent compression jobs**: `compression-coordinator` provides
  two levels of concurrency control to improve fairness among compression jobs running concurrently:
  * Coordinator-side rate limit: TODO, depends on #2435.
  * Spider-side active job limit: Controls the maximum number of jobs that can make progress
    concurrently while sharing compute resources in Spider. Jobs are admitted on a first-come,
    first-served basis.
* **Data integrity**: Compression jobs coordinated by `compression-coordinator` publish their
  results to the rest of the system through a dedicated commit stage. The commit operation is both
  transactional and idempotent.
  * A job-level failure does not result in partial updates.
  * Internal retries do not result in duplicate updates

:::{note}
In this release, `compression-coordinator` has the following functional limitations:

* It supports only the `clp-json` package.
* It handles only compression jobs created by `log-ingestor`.
* It supports only S3 archive output.

We are actively expanding the functionality of `compression-coordinator`. Additional capabilities
will be introduced in future releases as it moves toward feature parity with
`compression-scheduler`.
:::

:::{note}
`compression-scheduler` is planned for deprecation and will eventually be fully replaced by
`compression-coordinator`. In this release, however, `compression-scheduler` continues to run
alongside `compression-coordinator` to handle compression jobs that `compression-coordinator` does
not yet support.
:::

[spider]: https://github.com/y-scope/spider

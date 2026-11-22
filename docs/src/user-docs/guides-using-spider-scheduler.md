# Configuring Spider

CLP uses [Spider][spider] to schedule and run compression and search jobs. `compression-coordinator`
submits compression jobs to Spider, and `query-coordinator` submits search jobs to Spider. Spider
then runs the tasks of each job on its workers.

CLP puts the jobs of each coordinator in a separate Spider resource group: `compression` for
compression jobs, and `query` for search jobs. Spider workers run in three worker pools:

* `general`: Runs tasks from all resource groups.
* `compression`: Runs tasks from the `compression` resource group only.
* `query`: Runs tasks from the `query` resource group only.

This guide describes the Spider settings that are commonly adjusted in a CLP deployment, followed by
options for more advanced performance tuning.

## Common settings

### Worker pools

Each worker pool has a configurable number of Spider workers (replicas). By default, CLP deploys two
`general` workers, two `compression` workers, and four `query` workers. Increase the number of
workers in a pool to allow more of its tasks to run concurrently.

::::{tab-set}
:::{tab-item} Docker Compose
:sync: docker

Set the number of workers in each pool in `etc/clp-config.yaml`:

```yaml
spider:
  worker:
    pools:
      general:
        replicas: 2
      compression:
        replicas: 2
      query:
        replicas: 4
```

:::
:::{tab-item} Kubernetes (Helm)
:sync: k8s

Set the number of workers in each pool in the Helm values file. Helm replaces lists instead of
merging them, so you must specify all three pools with all of their fields:

```yaml
spider:
  spiderConfig:
    worker:
      pools:
        - name: "general"
          replicas: 2
          resource_group: null
        - name: "compression"
          replicas: 2
          resource_group:
            external_id: "compression"
            password:
              secretKeyRef:
                name: "{{ include \"clp.fullname\" . }}-spider-resource-groups"
                key: "compression"
        - name: "query"
          replicas: 4
          resource_group:
            external_id: "query"
            password:
              secretKeyRef:
                name: "{{ include \"clp.fullname\" . }}-spider-resource-groups"
                key: "query"
```

:::
::::

### Database

Spider stores metadata about its jobs in a database.

* Docker Compose deployments always use the database bundled with Spider.
* Kubernetes deployments use the database that the Spider subchart deploys by default. To use an
  external database instead, see [Using an external database with Spider][external-db-spider].

## Advanced tuning

The following settings provide additional control over the Spider scheduler's runtime behavior and
performance. Adjust them as needed to suit your workload and deployment environment.

Configure these settings under the following key:

* Docker Compose: `spider.scheduler.config` in `etc/clp-config.yaml`.
* Kubernetes (Helm): `spider.spiderConfig.scheduler.runtime.scheduler.config` in the Helm values
  file.

For example, to set {confval}`active_job_list_capacity` to `32`:

::::{tab-set}
:::{tab-item} Docker Compose
:sync: docker

```yaml
spider:
  scheduler:
    config:
      active_job_list_capacity: 32
```

:::
:::{tab-item} Kubernetes (Helm)
:sync: k8s

```yaml
spider:
  spiderConfig:
    scheduler:
      runtime:
        scheduler:
          config:
            active_job_list_capacity: 32
```

:::
::::

:::{confval} active_job_list_capacity
:type: `int`
:default: 16

Sets the maximum number of jobs in each resource group that can actively make progress at the same
time. The limit applies to each resource group separately. For example, with the default value, up
to 16 compression jobs and up to 16 search jobs can make progress at the same time. Jobs beyond
this limit remain queued until an active slot in their resource group becomes available. Currently,
queued jobs are admitted in first-in, first-out (FIFO) order.

The scheduler takes tasks from the resource groups in round-robin order, and within each resource
group, from the active jobs in round-robin order. As a result, this setting controls the trade-off
between job-level concurrency and the amount of compute capacity available to each active job.

Consider increasing this value when you want more jobs to make progress concurrently and sufficient
worker capacity is available. Decrease it when you prefer to concentrate compute resources on fewer
jobs at a time.
:::

:::{confval} dispatch_queue_capacity
:type: `int`
:default: 32

Sets the capacity of the scheduler's task dispatch buffer. This buffer holds tasks that are ready to
be dispatched to Spider workers. All resource groups share the buffer, and the scheduler limits the
share of each resource group so that one resource group can't fill the buffer.

As a general guideline, configure this value to approximately two to four times the total number of
Spider workers in all pools. If you increase the number of workers, consider increasing this value
as well so that the scheduler can keep enough tasks available for dispatch and avoid underutilizing
workers.
:::

:::{confval} ready_task_capacity
:type: `int`
:default: 1048576

Sets the maximum number of ready tasks that the Spider scheduler can buffer for scheduling, in all
resource groups.

A larger capacity allows the scheduler to consider more ready tasks at once, which can improve
fairness across active jobs and better accommodate workloads with high job submission rates.
However, buffering more tasks also increases the scheduler's memory usage.

Consider:

* Increasing this value for workloads that produce large numbers of ready tasks or have high job
  submission rates.
* Decreasing this value if scheduler memory usage is a concern and the workload does not require a
  large ready-task buffer.
:::

[external-db-spider]: guides-external-database.md#using-an-external-database-with-spider
[spider]: https://github.com/y-scope/spider

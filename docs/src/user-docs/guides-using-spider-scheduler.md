# Using Spider scheduler

[Spider][spider] is a distributed task scheduling and execution framework. It schedules tasks across
configured workers and manages their execution lifecycle.

CLP uses Spider as its next-generation framework for orchestrating compression and search jobs,
providing the following benefits:

* **Improved fault tolerance**
* **Minimized scheduling overhead**
* **Maximized utilization of compute resources**

:::{note}
In the current release, CLP uses Spider only for compression jobs coordinated by
`compression-coordinator`. Support for orchestrating search jobs with Spider is planned for a future
release.

Spider is planned to become the default job execution framework for CLP, eventually replacing the
existing Celery-based job orchestration components.
:::

## Deployment

When Spider is enabled, compatible CLP coordinators are automatically deployed and submit supported
jobs to Spider for scheduling and execution. Currently, Spider and its related CLP components can be
enabled only in [Kubernetes deployments][k8s-deployment] using the CLP Helm chart.

:::{note}
Spider is not supported in Docker Compose deployments in the current release. Support for Docker
Compose is planned for a future release.
:::

### Kubernetes (Helm)

When deploying CLP on Kubernetes using Helm, enable Spider by setting `spider.enabled` to `true`.
The CLP Helm chart will then automatically deploy the following components:

* **Spider subchart**: Deploys all the Spider services required to schedule and execute jobs.
* **compression-coordinator**: Coordinates CLP compression jobs and submits them to Spider for
  execution.

  :::{note}
  In the current release, `compression-coordinator` supports only compression jobs that use certain
  CLP package configurations. See "Using compression-coordinator" for the current limitations.
  :::

#### Set up

1. Create a values file to enable Spider and its related CLP components.

   ```{code-block} yaml
   :caption: spider-values.yaml

   spider:
     enabled: true

     # Optional Spider configuration passed directly to the Spider subchart.
     # The values below are the defaults and can be adjusted as needed.
     spiderConfig:
       storage:
         log_level: "INFO"
         port: 50051
       scheduler:
         log_level: "INFO"
       worker:
         replicas: 4

   clpConfig:
     # Optional `compression-coordinator` configuration.
     # The values below are the defaults and can be adjusted as needed.
     compression_coordinator:
       logging_level: "INFO"
       # How often the coordinator checks for newly created compression jobs.
       job_polling_interval_millisecs: 100
       # Maximum number of times a failed compression task can be retried.
       compression_task_max_retry: 1
       # Maximum number of times a failed commit task can be retried.
       commit_task_max_retry: 1
       # Maximum number of compression jobs that can be submitted to Spider concurrently.
       max_concurrent_jobs: 1000
   
     logs_input:
       # In the current release, compression jobs coordinated by `compression-coordinator` require
       # input logs to be stored in S3.
       type: "s3"
       aws_authentication:
         type: "credentials"
         credentials:
           access_key_id: "<access-key-id>"
           secret_access_key: "<secret-access-key>"

     archive_output:
       storage:
         # In the current release, `compression-coordinator` supports only S3 archive output.
         type: "s3"
         s3_config:
           region_code: "us-east-2"
           bucket: "<archives-bucket>"
           key_prefix: "archives/"
           aws_authentication:
             type: "credentials"
             credentials:
               access_key_id: "<access-key-id>"
               secret_access_key: "<secret-access-key>"
   ```

2. Install (or upgrade) the Helm chart with the Spider values:

   ```bash
   helm install clp clp/clp DOCS_VAR_HELM_VERSION_FLAG -f spider-values.yaml
   ```

3. Verify that `compression-coordinator` and Spider pods are running:

   ```bash
   kubectl get pods | grep -E "spider|compression-coordinator"
   ```

   The pods may take a few minutes to become ready. During startup, some Spider pods may restart
   while the Spider database is being initialized. A small number of restarts during this period is
   expected.

   The output should eventually include entries similar to the following:

   ```text
   NAME                                       READY   STATUS    RESTARTS   AGE
   clp-compression-coordinator-...            1/1     Running   0          5m
   clp-spider-database-0                      1/1     Running   0          5m
   clp-spider-scheduler-...                   1/1     Running   2          5m
   clp-spider-storage-...                     1/1     Running   2          5m
   clp-spider-worker-...                      1/1     Running   2          5m
   ```

## Configure Spider

Spider can be configured to meet the resource, storage, and authentication requirements of your
workload. This section describes commonly adjusted settings for Spider in a CLP deployment, followed
by options for more advanced performance tuning.

Configure these settings in the CLP Helm values file.

### Common settings

The following settings are commonly adjusted when deploying Spider with CLP.

:::{confval} spider.enabled
:type: `bool`
:default: `false`

Enables Spider and deploys its related CLP components.
:::

:::{confval} spider.spiderConfig.database
:type: `map`
:default: The Spider subchart's bundled database

Configures the database that Spider uses for metadata storage. By default, the Spider subchart
deploys a bundled database and connects to it. To use an external database instead, see
[Using an external database with Spider][external-db-spider].
:::

:::{confval} spider.spiderConfig.worker.replicas
:type: `int`
:default: 4

Sets the number of Spider worker pods to deploy. Increase this value to allow more tasks to run
concurrently.
:::

### Advanced tuning

The following settings provide additional control over Spider's runtime behavior and performance.
Adjust them as needed to suit your workload and deployment environment.

:::{confval} spider.spiderConfig.scheduler.config.active_job_queue_size
:type: `int`
:default: 8

Sets the maximum number of jobs that can actively make progress in Spider at the same time. Jobs
beyond this limit remain queued until an active slot becomes available. Currently, queued jobs are
admitted in first-in, first-out (FIFO) order.

Active jobs share the available Spider workers in a round-robin fashion. As a result, this setting
controls the trade-off between job-level concurrency and the amount of compute capacity available
to each active job.

In the current release, Spider executes only compression jobs, so this setting effectively limits
the number of compression jobs that can make progress concurrently.

Consider increasing this value when you want more jobs to make progress concurrently and sufficient
worker capacity is available. Decrease it when you prefer to concentrate compute resources on fewer
jobs at a time.
:::

:::{confval} spider.spiderConfig.scheduler.config.dispatch_queue_capacity
:type: `int`
:default: 16

Sets the capacity of the scheduler's task dispatch queue. This queue buffers tasks that are ready to
be dispatched to Spider workers.

As a general guideline, configure this value to approximately two to four times the number of
Spider workers. If you increase the number of workers, consider increasing this value as well so
that the scheduler can keep enough tasks available for dispatch and avoid underutilizing workers.
:::

:::{confval} spider.spiderConfig.scheduler.config.ready_task_capacity
:type: `int`
:default: 1048576

Sets the maximum number of ready tasks that the Spider scheduler can buffer for scheduling.

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
[k8s-deployment]: guides-k8s-deployment.md
[spider]: https://github.com/y-scope/spider

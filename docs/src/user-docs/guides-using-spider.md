# Using Spider with CLP

[Spider] is a distributed system for executing user-defined tasks. CLP uses it to orchestrate compression and search
jobs. This guide describes how to set up and use Spider with CLP.

:::{warning}
Currently, only the [clp-json](quick-start/clp-json.md) flavor of CLP supports Spider-orchestrated
jobs, and only for logs ingested from S3 through [`log-ingestor`][log-ingestor-guide].
:::

:::{note}
The Spider integration currently covers compression only. Search jobs remain unaffected; they
continue to use Celery regardless of whether Spider is enabled.
:::

## Deployment options

Spider-orchestrated compression is currently only supported through the
[Kubernetes (Helm)][k8s-deployment] deployment method.

## Kubernetes (Helm)

When deploying CLP on Kubernetes using Helm, Spider can be enabled by setting `spider.enabled` to
`true`. When enabled, the chart additionally deploys:

* The [**compression coordinator**][compression-coordinator-guide], a CLP component that polls for
  compression jobs and drives them through Spider.
* The **Spider subchart**: Spider's storage service, scheduler, workers, and a bundled MariaDB for
  Spider's job state. The workers run a CLP-specific image
  (`ghcr.io/y-scope/clp/clp-spider-worker`) that bundles CLP's task library and core binaries, so
  they can execute CLP compression tasks.

Spider runs alongside the existing Celery-based compression pipeline; enabling it doesn't replace
the Celery components.

### Requirements

* A running CLP Kubernetes deployment (see the [Kubernetes deployment guide][k8s-deployment]) with
  the following configuration:
  * **S3 for both input and output**: `clpConfig.logs_input.type` and
    `clpConfig.archive_output.storage.type` must both be `"s3"` (see the
    [object storage guide][object-storage-guide]). The compression coordinator validates this at
    startup and will fail to start otherwise.
  * **clp-s storage engine**: `clpConfig.package.storage_engine` must be `"clp-s"` (the default).

### Set up

1. Create a values file to enable Spider:

   ```{code-block} yaml
   :caption: spider-values.yaml

   spider:
     enabled: true

     # Optional: Spider component settings (passed through to the Spider subchart), shown with
     # their default values. Adjust them to suit your usage patterns.
     spiderConfig:
       storage:
         log_level: "INFO"
         port: 50051
       scheduler:
         log_level: "INFO"
       worker:
         replicas: 4

   clpConfig:
     # Optional: compression coordinator settings, shown with their default values. Adjust them
     # to suit your usage patterns. For details about the coordinator, see the "Using
     # compression-coordinator" guide.
     compression_coordinator:
       job_polling_interval_millisecs: 100
       logging_level: "INFO"

     logs_input:
       # NOTE: Currently, only S3 input is supported. Support for ingestion from local
       # filesystems is planned for a future release.
       type: "s3"
       aws_authentication:
         type: "credentials"
         credentials:
           access_key_id: "<access-key-id>"
           secret_access_key: "<secret-access-key>"

     archive_output:
       storage:
         # NOTE: Currently, only S3 storage is supported. Support for filesystem compression is
         # planned for a future release.
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

3. Verify that the compression coordinator and Spider pods are running. Pods may take a few minutes
   to become ready, and Spider pods restarting a few times while Spider's database comes up is
   normal:

   ```bash
   kubectl get pods | grep -E "spider|compression"
   ```

   ```text
   NAME                                       READY   STATUS    RESTARTS   AGE
   clp-compression-coordinator-...            1/1     Running   0          5m
   clp-compression-scheduler-...              1/1     Running   0          5m
   clp-compression-worker-...                 1/1     Running   0          5m
   clp-spider-database-0                      1/1     Running   0          5m
   clp-spider-scheduler-...                   1/1     Running   2          5m
   clp-spider-storage-...                     1/1     Running   2          5m
   clp-spider-worker-...                      1/1     Running   2          5m
   ```

Once the pods are ready, compression jobs submitted through [`log-ingestor`][log-ingestor-guide]
will be orchestrated by Spider.

### Configuration

The values below are the most commonly adjusted settings. Keys under `spider.spiderConfig` that
are specific to the Kubernetes deployment are listed individually; all other keys map directly to
Spider's own configuration.

| Value | Default | Description |
|---|---|---|
| `spider.enabled` | `false` | Deploys Spider and the compression coordinator. |
| `spider.spiderConfig.worker.replicas` | `4` | Number of Spider worker pods. |
| `spider.spiderConfig.worker.extra_envs` | See `values.yaml` | Additional environment variables for the Spider worker pods. Overriding this **replaces** the chart's default list (CLP's config path and database credentials), so restate the defaults when adding entries. |
| `spider.spiderConfig.worker.extra_volumes` / `.extra_volume_mounts` | See `values.yaml` | Additional volumes and mounts for the Spider worker pods. The chart's defaults mount CLP's config and a staging directory; restate them when adding entries. |
| `spider.spiderConfig.worker.service_account_name` | CLP's ServiceAccount | The ServiceAccount that the Spider worker pods run as. |
| All other keys under `spider.spiderConfig` | See the Spider user guide (**null**) | Spider's own configuration; each key maps to the corresponding setting of the matching Spider component — e.g., `spiderConfig.storage.log_level` maps to the `log_level` setting under Spider's storage component. |



[compression-coordinator-guide]: guides-using-compression-coordinator.md
[k8s-deployment]: guides-k8s-deployment.md
[log-ingestor-guide]: guides-using-log-ingestor.md
[object-storage-guide]: guides-using-object-storage/index.md
[Spider]: https://github.com/y-scope/spider

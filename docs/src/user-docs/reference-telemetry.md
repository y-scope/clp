# Telemetry

CLP collects anonymous operational metrics via [OpenTelemetry](https://opentelemetry.io/) to help
improve the software. Telemetry is **enabled by default** and can be easily disabled through
multiple mechanisms.

## Why we collect telemetry

As an open-source project, we have limited visibility into how CLP is used in the community.
Anonymous metrics help us understand deployment patterns, prioritize platform support, and
make informed build target decisions.

## What we collect

### Metrics

The following OpenTelemetry metrics are emitted:

#### Operational counters

Emitted by long-running CLP services to track throughput:

| Component             | Metric                               | Type    | Description                                       |
| --------------------- | ------------------------------------ | ------- | ------------------------------------------------- |
| api-server            | `clp.service.event`                  | Counter | Service lifecycle events (e.g., startup)          |
| compression-scheduler | `clp.compression.tasks.completed`    | Counter | Number of completed compression tasks             |
| compression-scheduler | `clp.compression.tasks.failed`       | Counter | Number of failed compression tasks                |
| compression-worker    | `clp.compression.bytes_input_total`  | Counter | Total uncompressed bytes processed by compression |
| compression-worker    | `clp.compression.bytes_output_total` | Counter | Total compressed bytes output by compression      |
| log-ingestor          | `clp.ingest.total_num_bytes`         | Counter | Total bytes ingested                              |
| log-ingestor          | `clp.ingest.total_num_objects`       | Counter | Total objects (log events) ingested               |
| query-scheduler       | `clp.query.tasks.completed`          | Counter | Number of completed query tasks                   |
| query-scheduler       | `clp.query.tasks.failed`             | Counter | Number of failed query tasks                      |

#### Operational up-down counters

Emitted by long-running CLP services to track current workload state:

| Component             | Metric                              | Type          | Description                                                    |
| --------------------- | ----------------------------------- | ------------- | -------------------------------------------------------------- |
| compression-scheduler | `clp.compression.active_jobs`       | UpDownCounter | Number of active compression jobs                              |
| compression-scheduler | `clp.compression.outstanding_tasks` | UpDownCounter | Total number of outstanding compression tasks                  |
| query-scheduler       | `clp.query.active_jobs`             | UpDownCounter | Number of active query jobs                                    |
| query-scheduler       | `clp.query.outstanding_tasks`       | UpDownCounter | Total number of outstanding tasks across all active query jobs |

#### Operational histograms

Emitted by long-running CLP services to track duration and rate distributions:

| Component             | Metric                          | Type      | Description                                                |
| --------------------- | ------------------------------- | --------- | ---------------------------------------------------------- |
| compression-scheduler | `clp.compression.job.duration`  | Histogram | Duration of compression jobs                               |
| compression-scheduler | `clp.compression.task.duration` | Histogram | Duration of compression tasks                              |
| compression-worker    | `clp.compression.input_rate`    | Histogram | Rate of uncompressed bytes processed per task              |
| compression-worker    | `clp.compression.output_rate`   | Histogram | Rate of compressed bytes output per task                   |
| query-scheduler       | `clp.query.job.duration`        | Histogram | Duration of query jobs                                     |
| query-scheduler       | `clp.query.task.duration`       | Histogram | Duration of query tasks                                    |

#### Deployment topology gauges

Emitted once at startup by the controller to record deployment sizing:

| Metric                                          | Type  | Description                           |
| ----------------------------------------------- | ----- | ------------------------------------- |
| `clp.deployment.compression_worker_replicas`    | Gauge | Number of compression-worker replicas |
| `clp.deployment.compression_worker_concurrency` | Gauge | Compression-worker concurrency        |
| `clp.deployment.query_worker_replicas`          | Gauge | Number of query-worker replicas       |
| `clp.deployment.query_worker_concurrency`       | Gauge | Query-worker concurrency              |
| `clp.deployment.reducer_replicas`               | Gauge | Number of reducer replicas            |
| `clp.deployment.reducer_concurrency`            | Gauge | Reducer concurrency                   |

### Traces

The following OpenTelemetry traces are emitted.

#### Archive-level search

Emitted per-archive according to a configurable sampling probability.

| Attribute                                               | Type    | Description                                                               |
| ------------------------------------------------------- | ------- | ------------------------------------------------------------------------- |
| `clp.query.success`                                     | Boolean | Whether the search completed successfully                                 |
| `clp.query.error`                                       | String  | The error message, if a failure occurred                                  |
| `clp.query.query_hash`                                  | Int64   | A hash of the query string                                                |
| `clp.query.query_id`                                    | String  | The ID of the Job                                                         |
| `clp.query.task_id`                                     | String  | The ID of the task in the Job                                             |
| `clp.query.archive_id_hash`                             | Int64   | A hash of the archive ID                                                  |
| `clp.query.column_types.num_pure_wildcard`              | Int64   | Number of column descriptors in the query that are only a wildcard        |
| `clp.query.column_types.num_some_wildcard`              | Int64   | Number of column descriptors in the query that mix wildcards and literals |
| `clp.query.column_types.num_no_wildcard`                | Int64   | Number of column descriptors in the query that contain no wildcards       |
| `clp.query.predicate_types.num_string`                  | Int64   | Number of predicates comparing against a string literal with no wildcards |
| `clp.query.predicate_types.num_string_with_wildcard`    | Int64   | Number of predicates comparing against a string literal with wildcards    |
| `clp.query.predicate_types.num_int`                     | Int64   | Number of predicates comparing against an integer literal                 |
| `clp.query.predicate_types.num_float`                   | Int64   | Number of predicates comparing against a float literal                    |
| `clp.query.predicate_types.num_null`                    | Int64   | Number of predicates comparing against a null literal                     |
| `clp.query.predicate_types.num_exact_match`             | Int64   | Number of equality predicates                                             |
| `clp.query.predicate_types.num_range`                   | Int64   | Number of range predicates                                                |
| `clp.query.predicate_types.num_exists`                  | Int64   | Number of predicates on field existence                                   |
| `clp.query.num_predicates`                              | Int64   | Total number of predicates in the query                                   |
| `clp.query.contains_or_clause`                          | Boolean | Whether the query contains an OR clause                                   |
| `clp.query.time_range_millis`                           | Int64   | Time range of the query in milliseconds                                   |
| `clp.query.num_total_archive_records`                   | Int64   | Total number of records                                                   |
| `clp.query.num_candidate_records_after_schema_matching` | Int64   | Total number of candidate records after schema matching                   |
| `clp.query.num_records_matching_query`                  | Int64   | Total number of records matching the query                                |
| `clp.query.num_matched_schemas`                         | Int64   | Total number of schema matched by schema matching                         |
| `clp.query.num_schemas_with_matches`                    | Int64   | Total number of schemas containing at least one matching record           |
| `clp.query.termination_stage`                           | String  | The stage at which the query terminated                                   | 

The termination stage can be one of:

| Stage                                         | Description                                          |
| --------------------------------------------- | ---------------------------------------------------- |
| `range_index_matching`                        | Early termination after examining range index        |
| `time_range_matching`                         | Early termination after examining archive time-range |
| `schema_matching`                             | Early termination after schema matching              |
| `time_range_matching_after_column_resolution` | Early termination after re-examining time-range      |
| `dictionary_search`                           | Early termination after dictionary search            |
| `ert_scan`                                    | Termination after decompression and scan             |

### Resource attributes

Every metric carries the following resource attributes to identify and contextualize the deployment:

| Resource Attribute      | Example                                    | Purpose                                      |
| ----------------------- | ------------------------------------------ | -------------------------------------------- |
| `clp.deployment.id`     | `550e8400-e29b-41d4-a716-446655440000`     | Deduplicate metrics from the same deployment |
| `service.version`       | `0.9.1`                                    | Track version adoption                       |
| `clp.deployment.method` | `docker-compose` or `helm`                 | Understand deployment preferences            |
| `clp.storage.engine`    | `clp-s` or `clp`                           | Track feature adoption                       |
| `service.name`          | `log-ingestor`, `api-server`, `controller` | Identify the emitting component              |
| `host.arch`             | `x86_64`, `aarch64`                        | Inform build target priorities               |
| `host.cpu.family`       | `6`                                        | Inform build target priorities               |
| `host.cpu.model.id`     | `142`                                      | Inform build target priorities               |
| `host.cpu.vendor.id`    | `GenuineIntel`                             | Inform build target priorities               |

`service.name` is set via the `OTEL_SERVICE_NAME` environment variable for Rust and Python services
and hardcoded in the controller's topology metrics payload. `clp.deployment.id`,
`clp.deployment.method`, `clp.storage.engine`, and `service.version` are set via the
`OTEL_RESOURCE_ATTRIBUTES` environment variable (Docker Compose) or Helm template helpers
(Kubernetes). The `host.*` attributes are collected by the OpenTelemetry Collector's
resourcedetection processor. `host.name` is explicitly **disabled** in the collector config and is
not collected.

The `clp.deployment.id` is a random UUIDv4 generated on first run and stored locally at
`$CLP_HOME/var/log/instance-id`. It is never derived from hardware identifiers. One installation
directory equals one deployment ID. Separate `$CLP_HOME` directories on the same machine produce
separate IDs.

## What we do NOT collect

Telemetry does **not** include: log content, queries, hostnames, IP addresses, or any other
Personally Identifiable Information (PII).

## Telemetry endpoint

Metrics are exported via the [OpenTelemetry Protocol
(OTLP)](https://opentelemetry.io/docs/specs/otlp/) to:

`https://telemetry.yscope.io`

:::{note}
With the bundled OpenTelemetry Collector, `telemetry.endpoint` is the collector's export target. To
use your own collector, remove `otel_collector` from `bundled` and set `otel_collector.host` /
`otel_collector.port` to its OTLP/HTTP receiver.

When you use your own collector, the bundled collector's resource detection and batching are not
applied unless your collector config includes equivalent processors; your collector also controls
filtering, redaction, aggregation, and forwarding. For a matching baseline, start from
`components/package-template/src/etc/otel-collector/config.yaml` (Docker Compose) or the
`otel-collector-config.yaml` embedded in `tools/deployment/package-helm/templates/configmap.yaml`
(Helm).
:::

## Configuration Settings

You can configure the interval at which metrics are exported for each instrumented component. The
`telemetry_update_interval_ms` setting allows you to control the export frequency (in milliseconds)
and defaults to `60000` (60 seconds).

The supported components are `compression_scheduler`, `compression_worker`, `query_scheduler`, and
`query_worker`.

::::{tab-set}
:::{tab-item} Docker Compose
:sync: docker
Edit `clp-config.yaml` to set the interval per component:

```yaml
compression_scheduler:
  telemetry_update_interval_ms: 60000
```

:::
:::{tab-item} Kubernetes
:sync: k8s
Edit `values.yaml` to set the interval per component:

```yaml
clpConfig:
  compression_scheduler:
    telemetry_update_interval_ms: 60000
```

:::
::::

The `query_worker` component also supports exporting traces with detailed metrics about querying
individual archives; you can configure the sampling probability for these traces in the
configuration.

::::{tab-set}
:::{tab-item} Docker Compose
:sync: docker
Edit `clp-config.yaml` to set the sampling probability:

```yaml
query_worker:
  search_sampling_probability: 0.01
```

:::
:::{tab-item} Kubernetes
:sync: k8s
Edit `values.yaml` to set the sampling probability:

```yaml
clpConfig:
  query_worker:
    search_sampling_probability: 0.01
```

:::
::::

## How to disable telemetry

Any **one** of the following methods is sufficient:

### Environment variable

Set `CLP_DISABLE_TELEMETRY`, or `DO_NOT_TRACK` per the
[Console Do Not Track](https://consoledonottrack.com/) standard, before launching `start-clp.sh`.

```bash
export CLP_DISABLE_TELEMETRY=true
```

### Configuration file

::::{tab-set}
:::{tab-item} Docker Compose
:sync: docker
Edit `clp-config.yaml`:

```yaml
telemetry:
  disable: true
  endpoint: "https://telemetry.yscope.io"
```

:::
:::{tab-item} Kubernetes
:sync: k8s
Pass the config via `--set` (see the [quick-start guide](quick-start/index.md) for setup details):

```bash
helm install clp clp/clp --set clpConfig.telemetry.disable=true
```

:::
::::

### First-run prompt

When you run `start-clp.sh` for the first time in an interactive terminal, a consent prompt
appears. If you decline, `telemetry.disable: true` is written to `clp-config.yaml`.

### Network-level blocking

For network admins, block `telemetry.yscope.io` at your firewall or proxy. This is the simplest way
to disable telemetry for your entire organization.

### Interaction when multiple opt-out mechanisms are set

| Env var | Config file | First-run prompt | Network blocked | Telemetry sent?                                                        |
| ------- | ----------- | ---------------- | --------------- | ---------------------------------------------------------------------- |
| not set | not set     | Y (or default)   | no              | **Yes**                                                                |
| not set | not set     | N                | no              | **No** — prompt wrote `telemetry.disable: true` to config              |
| `true`  | `false`     | —                | no              | **No** — env var overrides config                                      |
| `false` | `true`      | —                | no              | **No** — `false` is not a recognized disable value; config disables it |
| not set | `false`     | —                | **yes**         | **No** — requests fail silently at the network level                   |
| `true`  | `true`      | —                | no              | **No** — both agree                                                    |
| not set | not set     | Y                | **yes**         | **No** — network blocking is independent of software settings          |

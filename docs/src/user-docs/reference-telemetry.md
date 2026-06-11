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

| Component | Metric | Type | Description |
| --- | --- | --- | --- |
| log-ingestor | `clp.ingest.total_num_bytes` | Counter | Total bytes ingested |
| log-ingestor | `clp.ingest.total_num_objects` | Counter | Total objects (log events) ingested |
| api-server | `clp.service.event` | Counter | Service lifecycle events (e.g., startup) |

#### Deployment topology gauges

Emitted once at startup by the controller to record deployment sizing:

| Metric | Type | Description |
| --- | --- | ----------- |
| `clp.deployment.compression_worker_replicas` | Gauge | Number of compression-worker replicas |
| `clp.deployment.compression_worker_concurrency` | Gauge | Compression-worker concurrency |
| `clp.deployment.query_worker_replicas` | Gauge | Number of query-worker replicas |
| `clp.deployment.query_worker_concurrency` | Gauge | Query-worker concurrency |
| `clp.deployment.reducer_replicas` | Gauge | Number of reducer replicas |
| `clp.deployment.reducer_concurrency` | Gauge | Reducer concurrency |

### Resource attributes

Every metric carries the following resource attributes to identify and contextualize the deployment:

| Resource Attribute | Example | Purpose |
| --- | --- | --- |
| `clp.deployment.id` | `550e8400-e29b-41d4-a716-446655440000` | Deduplicate metrics from the same deployment |
| `service.version` | `0.9.1` | Track version adoption |
| `clp.deployment.method` | `docker-compose` or `helm` | Understand deployment preferences |
| `clp.storage.engine` | `clp-s` or `clp` | Track feature adoption |
| `service.name` | `log-ingestor`, `api-server`, `controller` | Identify the emitting component |
| `host.arch` | `x86_64`, `aarch64` | Inform build target priorities |
| `host.cpu.family` | `6` | Inform build target priorities |
| `host.cpu.model.id` | `142` | Inform build target priorities |
| `host.cpu.vendor.id` | `GenuineIntel` | Inform build target priorities |

`service.name` is set via the `OTEL_SERVICE_NAME` environment variable for Rust services and
hardcoded in the controller's topology metrics payload. `clp.deployment.id`,
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

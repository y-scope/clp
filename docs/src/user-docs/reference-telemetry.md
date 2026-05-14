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

Each CLP component emits OpenTelemetry counters for its core operations:

| Component | Metric | Type | Description |
|-----------|--------|------|-------------|
| log-ingestor | `clp.ingest.bytes_total` | Counter | Total bytes ingested |
| log-ingestor | `clp.ingest.records_total` | Counter | Total records ingested |
| query-worker | `clp.query.bytes_scanned_total` | Counter | Total bytes scanned by queries |
| query-worker | `clp.query.bytes_output_total` | Counter | Total bytes returned by queries |
| compression-worker | `clp.compression.bytes_input_total` | Counter | Total bytes compressed |
| compression-worker | `clp.compression.bytes_output_total` | Counter | Total bytes after compression |

These metrics answer real operational questions:
- **How much data is CLP processing?** — ingest bytes + records counters
- **How efficiently?** — compression input/output ratio
- **How actively is it being queried?** — query bytes scanned/returned
- **How does this vary by deployment?** — correlated with resource attributes (see below)

### Resource attributes

Every metric carries the following resource attributes to identify and contextualize the deployment:

| Resource Attribute | Example | Purpose |
|---|---|---|
| `clp.deployment.id` | `550e8400-e29b-41d4-a716-446655440000` | Deduplicate metrics from the same deployment |
| `service.version` | `0.9.1` | Track version adoption |
| `clp.deployment.method` | `docker-compose` or `helm` | Understand deployment preferences |
| `clp.storage.engine` | `clp-s` or `clp` | Track feature adoption |
| `os.type` | `linux` | Inform platform support |
| `os.version` | `ubuntu-22.04` | Inform platform support |
| `host.arch` | `x86_64`, `aarch64` | Inform build target priorities |
| `service.name` | `log-ingestor`, `query-worker`, etc. | Identify the emitting component |

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
  endpoint: "https://telemetry.yscope.io"  # or "https://your-own-otel-collector"
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
|---------|-------------|------------------|-----------------|------------------------------------------------------------------------|
| not set | not set     | Y (or default)   | no              | **Yes**                                                                |
| not set | not set     | N                | no              | **No** — prompt wrote `telemetry.disable: true` to config              |
| `true`  | `false`     | —                | no              | **No** — env var overrides config                                      |
| `false` | `true`      | —                | no              | **No** — `false` is not a recognized disable value; config disables it |
| not set | `false`     | —                | **yes**         | **No** — requests fail silently at the network level                   |
| `true`  | `true`      | —                | no              | **No** — both agree                                                    |
| not set | not set     | Y                | **yes**         | **No** — network blocking is independent of software settings          |


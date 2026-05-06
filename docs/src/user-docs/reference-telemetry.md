# Telemetry

CLP collects anonymous operational metrics via [OpenTelemetry](https://opentelemetry.io/) to help
improve the software. Telemetry is **enabled by default** and can be easily disabled through
multiple mechanisms.

## Why we collect telemetry

As an open-source project, we have limited visibility into how CLP is used in the community.
Anonymous metrics help us:

- Understand how much data CLP deployments are processing
- Measure compression efficiency across real-world workloads
- Gauge how actively CLP is being queried
- Correlate performance and usage patterns with deployment configurations
- Prioritize platform support and build target decisions

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
| `clp.version` | `0.9.1` | Track version adoption |
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

- **Log content or query content** — never, under any circumstances
- **Personally identifiable information** — no usernames, emails, or organization names
- **IP addresses** — visible during network communication but **not logged or stored** in the
  telemetry backend
- **Credentials and secrets** — no passwords, API keys, or connection strings
- **Hostnames** — no server or container hostnames
- **Local timezone** — all timestamps are UTC; no timezone information is collected as it could
  reveal geographic region
- **Per-tenant or per-dataset metrics** — only aggregate counters per deployment

## Telemetry endpoint

Metrics are exported via the [OpenTelemetry Protocol
(OTLP)](https://opentelemetry.io/docs/specs/otlp/) to:

`https://telemetry.yscope.io:4318`

This is the standard OTLP/HTTP port. System administrators who do not want telemetry leaving their
network can block this endpoint at the firewall or proxy level.

## How to disable telemetry

Any **one** of the following methods is sufficient:

### Environment variable

```bash
export CLP_DISABLE_TELEMETRY=true
```

Or use the [Console Do Not Track](https://consoledonottrack.com/) standard:

```bash
export DO_NOT_TRACK=1
```

### Configuration file

Add to your `clp-config.yaml`:

```yaml
telemetry:
  disable: true
```

You can also override the collector endpoint:

```yaml
telemetry:
  disable: false
  collector_endpoint: "https://your-own-otel-collector:4318"
```

### First-run prompt

When running `start-clp.sh` for the first time in an interactive terminal, you will see a
consent prompt. Answering `n` will automatically set `telemetry.disable: true` in your config.

### Helm chart

Set in your `values.yaml`:

```yaml
clpConfig:
  telemetry:
    disable: true
```

### Network-level blocking

Block `telemetry.yscope.io` (port 4318) at your firewall or proxy. This is the simplest way to
disable telemetry for an entire organization.

### Interaction when multiple opt-out mechanisms are set

| Env var | Config file | First-run prompt | Network blocked | Telemetry sent? |
|---|---|---|---|---|
| not set | not set | Y (or default) | no | **Yes** |
| not set | not set | N | no | **No** — prompt wrote `telemetry.disable: true` to config |
| `true` | `false` | — | no | **No** — env var overrides config |
| `false` | `true` | — | no | **No** — config disables it |
| not set | `false` | — | **yes** | **No** — requests fail silently at the network level |
| `true` | `true` | — | no | **No** — both agree |
| not set | not set | Y | **yes** | **No** — network blocking is independent of software settings |

## Debug mode

To inspect exactly what metrics would be sent without actually sending them:

```bash
export CLP_TELEMETRY_DEBUG=true
```

When enabled, each component uses an OpenTelemetry `ConsoleExporter` that prints metrics to its
standard output (visible in container logs) instead of transmitting them to the remote endpoint.

## Source code

The telemetry implementation is fully open source:

- **Component instrumentation**:
  [components/log-ingestor/](https://github.com/y-scope/clp/tree/DOCS_VAR_CLP_GIT_REF/components/log-ingestor/),
  [components/api-server/](https://github.com/y-scope/clp/tree/DOCS_VAR_CLP_GIT_REF/components/api-server/)
  (and other Rust components with `opentelemetry` in their `Cargo.toml`)
- **Consent prompt**:
  [components/clp-package-utils/clp_package_utils/scripts/start_clp.py](https://github.com/y-scope/clp/blob/DOCS_VAR_CLP_GIT_REF/components/clp-package-utils/clp_package_utils/scripts/start_clp.py)
- **Configuration**: `telemetry` section in `clp-config.yaml`
- **Server**:
  [clp-telemetry-server](https://github.com/y-scope/clp-telemetry-server)

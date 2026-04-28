# Telemetry

CLP collects anonymous usage telemetry to help improve the software. Telemetry is **enabled
by default** and can be easily disabled through multiple mechanisms.

## Why we collect telemetry

As an open-source project, we have limited visibility into how CLP is used in the community.
Anonymous telemetry helps us:

- Understand how many deployments exist and what versions are running
- Identify common deployment methods and platform preferences
- Prioritize development and build target decisions
- Detect common failure modes

## What we collect

| Data point | Example | Purpose |
|---|---|---|
| Anonymous installation UUID | `550e8400-e29b-41d4-a716-446655440000` | Deduplicate events from the same deployment |
| CLP version | `0.9.1` | Track version adoption |
| Deployment method | `docker-compose` / `helm` | Understand deployment preferences |
| OS type and version | `linux`, `ubuntu-22.04` | Inform platform support |
| CPU architecture | `x86_64`, `aarch64` | Inform build target priorities |
| Storage engine | `clp-s` / `clp` | Track feature adoption |
| Metric Counters | `clp.api.deployment_start`, `clp.api.heartbeat` | Track lifecycle events |
| Performance Metrics | `clp.ingest.bytes_total`, `clp.query.scanned_bytes` | Understand processing scale |
| Timestamp (UTC) | `2025-01-15T10:30:00Z` | Time-series analysis |

## What we do NOT collect

- **Log content or query content** — never, under any circumstances
- **Personally identifiable information** — no usernames, emails, or organization names
- **IP addresses** — visible during network communication but **not logged or stored**
- **Credentials and secrets** — no passwords, API keys, or connection strings
- **Hostnames** — no server or container hostnames

## Telemetry endpoint

All telemetry data is sent to: `https://telemetry.yscope.io`

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

Block `https://telemetry.yscope.io` at your firewall or proxy. This is the simplest way to
disable telemetry for an entire organization.

## Debug mode

To inspect exactly what telemetry data would be sent without actually sending it to YScope, you can modify the `otel-collector-config.yaml` file to use a `debug` exporter:

```yaml
exporters:
  debug:
    verbosity: detailed

service:
  pipelines:
    metrics:
      receivers: [otlp]
      processors: [batch]
      exporters: [debug] # Replaced otlp/telemetry_server with debug
```

The OpenTelemetry metrics payload will be logged to the Collector's standard output instead of being transmitted over the network.

## Source code

The telemetry implementation is fully open source:

- **Clients**: Extends across multiple components including `api-server`, `log-ingestor`, and Python workers.
- **Consent prompt**:
  [components/package-template/src/sbin/start-clp.sh](https://github.com/y-scope/clp/blob/DOCS_VAR_CLP_GIT_REF/components/package-template/src/sbin/start-clp.sh)
- **Configuration**: `telemetry.disable` in `clp-config.yaml`
- **Server**:
  [clp-telemetry-server](https://github.com/y-scope/clp-telemetry-server)

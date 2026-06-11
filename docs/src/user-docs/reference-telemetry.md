# Telemetry

CLP collects anonymous operational metrics via [OpenTelemetry](https://opentelemetry.io/) to help
improve the software. Telemetry is **enabled by default** and can be easily disabled through
multiple mechanisms.

## Why we collect telemetry

As an open-source project, we have limited visibility into how CLP is used in the community.
Anonymous metrics help us understand deployment patterns, prioritize platform support, and
make informed build target decisions.

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

You can configure the interval at which metrics are exported for each instrumented component. The `telemetry_update_interval_ms` setting allows you to control the export frequency (in milliseconds) and defaults to `60000` (60 seconds).

::::{tab-set}
:::{tab-item} Docker Compose
:sync: docker
Edit `clp-config.yaml` to set the interval per component:

```yaml
compression_scheduler:
  telemetry_update_interval_ms: 60000

api_server:
  telemetry_update_interval_ms: 60000
```

:::
:::{tab-item} Kubernetes
:sync: k8s
Pass the config via `--set`:

```bash
helm install clp clp/clp --set clpConfig.compression_scheduler.telemetry_update_interval_ms=60000
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
|---------|-------------|------------------|-----------------|------------------------------------------------------------------------|
| not set | not set     | Y (or default)   | no              | **Yes**                                                                |
| not set | not set     | N                | no              | **No** — prompt wrote `telemetry.disable: true` to config              |
| `true`  | `false`     | —                | no              | **No** — env var overrides config                                      |
| `false` | `true`      | —                | no              | **No** — `false` is not a recognized disable value; config disables it |
| not set | `false`     | —                | **yes**         | **No** — requests fail silently at the network level                   |
| `true`  | `true`      | —                | no              | **No** — both agree                                                    |
| not set | not set     | Y                | **yes**         | **No** — network blocking is independent of software settings          |

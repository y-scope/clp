# Telemetry

CLP collects anonymous operational metrics via [OpenTelemetry](https://opentelemetry.io/) to help
improve the software. Telemetry is **enabled by default** and can be easily disabled through
multiple mechanisms.

## Why we collect telemetry

As an open-source project, we have limited visibility into how CLP is used in the community.
Anonymous metrics help us understand deployment patterns, prioritize platform support, and
make informed build target decisions.

## Telemetry endpoint

Metrics are exported via the [OpenTelemetry Protocol
(OTLP)](https://opentelemetry.io/docs/specs/otlp/) to:

`https://telemetry.yscope.io`

System administrators who do not want telemetry leaving their
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
 
::::{tab-set}
:::{tab-item} Docker Compose
:sync: docker
 
Add to your `clp-config.yaml`:
 
```yaml
telemetry:
  disable: true
```
 
You can also override the endpoint:
 
```yaml
telemetry:
  disable: false
  endpoint: "https://your-own-otel-collector"
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

When running `start-clp.sh` for the first time in an interactive terminal, you will see a
consent prompt. Your choice is persisted to `clp-config.yaml` as `telemetry.disable: true` (for "no")
or `telemetry.disable: false` (for "yes"), ensuring the consent decision survives independent of the
instance-id file.

### Network-level blocking

Block `telemetry.yscope.io` at your firewall or proxy. This is the simplest way to
disable telemetry for an entire organization.

### Interaction when multiple opt-out mechanisms are set

| Env var | Config file | First-run prompt | Network blocked | Telemetry sent? |
|---|---|---|---|---|
| not set | not set | Y (or default) | no | **Yes** |
| not set | not set | N | no | **No** — prompt wrote `telemetry.disable: true` to config |
| `true` | `false` | — | no | **No** — env var overrides config |
| `false` | `true` | — | no | **No** — config disables it; env vars can only disable, not re-enable |
| not set | `false` | — | **yes** | **No** — requests fail silently at the network level |
| `true` | `true` | — | no | **No** — both agree |
| not set | not set | Y | **yes** | **No** — network blocking is independent of software settings |

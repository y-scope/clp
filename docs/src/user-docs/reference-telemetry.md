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
| Event type | `deployment_start`, `heartbeat` | Understand usage patterns |
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

To inspect exactly what telemetry data would be sent without actually sending it:

```bash
export CLP_TELEMETRY_DEBUG=true
```

The JSON payload will be logged to the API server log file instead of being transmitted.

## Source code

The telemetry implementation is fully open source:

- **Client**: `components/api-server/src/telemetry.rs`
- **Consent prompt**: `components/package-template/src/sbin/start-clp.sh`
- **Configuration**: `telemetry.disable` in `clp-config.yaml`
- **Server**: `clp-telemetry-server/`

# Logging

The CLP package contains multiple runtime stacks, and each stack owns its logging setup. This page
summarizes the current behavior and the controls developers/operators should use.

## Component summary
This section explains the logging setup of the different components in CLP package. 
| Component family              | Examples                                                            | Logger                                                    | Format                           | Level control              |
|-------------------------------|---------------------------------------------------------------------|-----------------------------------------------------------|----------------------------------|----------------------------|
| Python orchestration services | Schedulers, workers, reducer wrapper, garbage collector, MCP server | [`clp_py_utils.clp_logging`][clp-py-logging]              | JSON                             | `CLP_LOGGING_LEVEL`        |
| Rust HTTP services            | API server, log ingestor                                            | [`clp_rust_utils::logging`][clp-rust-logging] / `tracing` | JSON                             | `RUST_LOG`                 |
| WebUI server                  | Fastify server                                                      | Fastify/Pino                                              | JSON in prod; pretty text in dev | `LOG_LEVEL`                |
| WebUI client                  | Browser app                                                         | Browser console                                           | Browser console output           | Browser/devtools dependent |
| Native core binaries          | `clp`, `clp-s`, `glt`, native `reducer_server`                      | `spdlog`                                                  | Text                             | Binary-specific            |
| Package/setup tools           | Package controller, DB initialization scripts                       | Python stdlib logging                                     | Text                             | Script-specific            |

There is no single project-wide JSON schema. Python, Rust, and Pino logs are all line-delimited JSON
in packaged non-interactive service runtimes, but each stack uses its own field names.

## Development guidance

* New Python orchestration services should use 
  [`structlog.get_logger`][clp-py-logging] and
  [`clp_py_utils.clp_logging.configure_logging`][clp-py-logging].
* New Rust services should use [`clp_rust_utils::logging::set_up_logging`][clp-rust-logging].
* WebUI server code should log through Fastify's logger (`request.log` or `app.log`).
* WebUI client `console.*` calls should stay limited to browser diagnostics.
* Native core binaries should keep their existing `spdlog` setup unless a separate change migrates
  them to structured logging.

## Timestamp convention

Structured service logs should use UTC or another unambiguous absolute timestamp format. Python
service logs use UTC ISO-8601 timestamps, Rust service logs use `tracing_subscriber` timestamps, and
WebUI server logs use Pino epoch-millisecond timestamps. Prefer converting to local time in the log
viewer or aggregator rather than emitting ambiguous local-time strings from services.

## Component details

The following sections describe the behavior and controls for each logging stack.

### Python orchestration services

These services use [`clp_py_utils.clp_logging`][clp-py-logging] and emit one JSON object per log
record:

* `compression_scheduler`
* `query_scheduler`
* `compression_worker`
* `query_worker`
* `reducer`
* `garbage_collector`
* `mcp_server`

Python service records include:

| Field        | Description                                              |
|--------------|----------------------------------------------------------|
| `timestamp`  | ISO-8601 UTC timestamp.                                  |
| `event`      | Formatted log message.                                   |
| `logger`     | Logger name.                                             |
| `level`      | Lowercase log level.                                     |
| `exception`  | Present when `exc_info` is logged.                       |
| `stack`      | Present when `stack_info` is logged.                     |
| Extra fields | Fields passed through stdlib logging's `extra` argument. |

Example:

<!-- markdownlint-disable MD013 -->
```json
{"timestamp":"2026-06-22T17:03:21.123456Z","event":"Compression job 1 submitted.","logger":"compression_scheduler","level":"info"}
```
<!-- markdownlint-enable MD013 -->

Controls:

* `CLP_LOGGING_LEVEL` supports `DEBUG`, `INFO`, `WARN`, `WARNING`, `ERROR`, and `CRITICAL`.
  Missing or invalid values default to `INFO`.
* `CLP_LOGS_DIR`, when set, adds a file handler at `<CLP_LOGS_DIR>/<component_name>.log` in
  addition to stdout.

### Rust HTTP services

`api_server` and `log_ingestor` use
[`clp_rust_utils::logging::set_up_logging`][clp-rust-logging], which configures
`tracing_subscriber` JSON output.

Rust service records include `timestamp`, `level`, `fields`, `filename`, and `line_number`.
Structured `tracing` fields are nested under `fields`.

Controls:

* `RUST_LOG` uses `tracing_subscriber::EnvFilter` syntax.
* `CLP_LOGS_DIR`, when set, adds an hourly non-blocking rolling file appender.

In the package manifests, `log_ingestor` exposes a deployment setting through
`CLP_LOG_INGESTOR_LOGGING_LEVEL` in Docker Compose and `clpConfig.log_ingestor.logging_level` in
Helm. `api_server` currently runs with `RUST_LOG=INFO`.

### WebUI

The WebUI server uses Fastify's Pino logger:

* Non-interactive runtimes emit Pino JSON to stdout.
* Interactive terminals use `pino-pretty` for developer-readable output.
* `LOG_LEVEL` controls the server log level and defaults to `info`.

The WebUI client logs to the browser console. Treat client `console.*` output as browser diagnostics,
not service logs.

### Native core binaries and package tools

Native core binaries use `spdlog` text output. Package controller commands and one-shot setup scripts
also use human-readable stdlib logging. These tools are not covered by the Python/Rust/WebUI service
JSON contracts.


## Deployment notes

* Docker Compose service stdout is available through `docker compose logs`.
  * Compose services that set `CLP_LOGS_DIR` also write under the mounted CLP log directory, which
    defaults to `./var/log` through `CLP_LOGS_DIR_HOST`.
* Helm deployments should primarily use pod stdout through `kubectl logs` or the cluster log
  collector. File logging is template-specific.

[clp-py-logging]: https://github.com/y-scope/clp/blob/DOCS_VAR_CLP_GIT_REF/components/clp-py-utils/clp_py_utils/clp_logging.py
[clp-rust-logging]: https://github.com/y-scope/clp/blob/DOCS_VAR_CLP_GIT_REF/components/clp-rust-utils/src/logging.rs

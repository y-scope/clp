# CLP Package Logging

The CLP package contains multiple components, and each component owns its logging setup. This page
summarizes the current behavior and the controls developers/operators should use.

## Developer guidance

This section gives guidance on how to set up and use loggers when writing a new component.

:::{note}
When modifying an existing service, follow its current logging setup.
:::

### Python
New Python orchestration services should use:
  ```python
  from clp_py_utils.clp_logging import get_structlog_logger

  log = get_structlog_logger("service_name")
  log.info("hello, %s!", "world")
  ```

### Rust
New Rust HTTP services should initialize `tracing` at process startup with
  [`clp_rust_utils::logging::set_up_logging`][clp-rust-logging] and keep the returned guard alive
  for the lifetime of the process:
  ```rust
  let _guard = clp_rust_utils::logging::set_up_logging("service_name.log");
  tracing::info!("Server started at {addr}");
  ```
Prefer structured logging over formatting values directly into the message field.

### WebUI
WebUI server code should use Fastify's Pino logger. Use `request.log` for request-scoped
logs and `app.log` for startup, shutdown, and application-level logs:
  ```typescript
  request.log.info({searchJobId}, "Search submitted");
  request.log.error(err, "Failed to submit search");
  ```

WebUI client `console.*` calls should stay limited to browser diagnostics. Do not rely on browser
console output for service logs, audit events, or telemetry that operators need to collect.

### Core
Native core binaries should continue using `spdlog` and their existing entry-point logger setup.
Prefer `spdlog` from core C++ code rather than introducing another logging stack.

:::{note}
Prefer UTC service timestamps. Convert to local time in log viewers or aggregation systems.
:::

## Component-specific logging
This section explains the logging setup for CLP package components.
| Component family              | Components                                                            | Logger                                                    | Format                           | Level control              |
|-------------------------------|---------------------------------------------------------------------|-----------------------------------------------------------|----------------------------------|----------------------------|
| Python orchestration services | `compression_scheduler`, `query_scheduler`, `compression_worker`, `query_worker`, `reducer`, `garbage_collector`, `mcp_server` | [`structlog` with stdlib logging compatibility][clp-py-logging]              | JSON                             | `CLP_LOGGING_LEVEL`        |
| Rust HTTP services            | `api_server`, `log_ingestor`                                        | [`clp_rust_utils::logging`][clp-rust-logging] / `tracing` | JSON                             | `RUST_LOG`                 |
| WebUI server                  | Fastify server                                                      | Fastify/Pino                                              | JSON in prod; pretty text in dev | `LOG_LEVEL`                |
| WebUI client                  | Browser app                                                         | Browser console                                           | Browser console output           | Browser/devtools dependent |
| Native core binaries          | `clp`, `clp-s`, `glt`, native `reducer_server`                      | `spdlog`                                                  | Text                             | Binary-specific            |
| Package/setup tools           | Package controller, DB initialization scripts                       | Python stdlib logging                                     | Text                             | Script-specific            |

There is no single project-wide JSON schema. Python, Rust, and Pino logs are all line-delimited JSON
in packaged non-interactive service runtimes, but each component family uses its own field names.

## Component details

The following sections describe the behavior and controls for each component family.

### Python orchestration services

These services use [`clp_py_utils.clp_logging`][clp-py-logging] and emit one JSON object per log
record. Each record includes the following fields:

| Field             | Description                                                          |
|-------------------|----------------------------------------------------------------------|
| `timestamp`       | ISO-8601 UTC timestamp.                                              |
| `event`           | Formatted log message.                                               |
| `logger`          | Logger name.                                                         |
| `level`           | Log level, emitted as values such as `info`, `warning`, and `error`. |
| `filename`        | Source filename for the log call.                                    |
| `func_name`       | Source function name for the log call.                               |
| `lineno`          | Source line number for the log call.                                 |
| `exception`       | Present when `exc_info` is logged.                                   |
| `stack`           | Present when `stack_info` is logged.                                 |
| Context variables | Fields bound through structlog context variables.                    |
| Extra fields      | Fields passed through stdlib logging's `extra` argument.             |

Example:

<!-- markdownlint-disable MD013 -->
```json
{"timestamp":"2026-06-22T17:03:21.123456Z","event":"Compression job 1 submitted.","logger":"compression_scheduler","level":"info","filename":"compression_scheduler.py","func_name":"main","lineno":616}
```
<!-- markdownlint-enable MD013 -->

Configuration:

* `CLP_LOGGING_LEVEL` supports `DEBUG`, `INFO`, `WARN`, `WARNING`, `ERROR`, and `CRITICAL`.
  Missing or invalid values default to `INFO`.
* `CLP_LOGS_DIR`, when set, adds a file handler at `<CLP_LOGS_DIR>/<component_name>.log` in
  addition to stdout.

### Rust HTTP services

These services use [`clp_rust_utils::logging::set_up_logging`][clp-rust-logging], which configures
`tracing_subscriber` to emit one JSON object per log record. Each record includes the following
fields:

| Field         | Description                                                |
|---------------|------------------------------------------------------------|
| `timestamp`   | Timestamp emitted by `tracing_subscriber`.                 |
| `level`       | Log level, emitted as values such as `INFO`, `WARN`, and `ERROR`. |
| `fields`      | Structured `tracing` fields, including the log message.    |
| `filename`    | Source filename for the log call.                          |
| `line_number` | Source line number for the log call.                       |

Example:

<!-- markdownlint-disable MD013 -->
```json
{"timestamp":"2026-06-26T13:06:48.621307","level":"INFO","fields":{"message":"Spawned SQS listener task.","job_id":"3","task_id":"0"},"filename":"components/log-ingestor/src/ingestion_job/sqs_listener.rs","line_number":320}
```
<!-- markdownlint-enable MD013 -->

Configuration:

* Configure log filtering with [tracing_subscriber::EnvFilter][EnvFilter].
  Filter directives are read from the `RUST_LOG` environment variable to determine which spans and
  events are enabled.
* In the package manifests, `log_ingestor` exposes a deployment setting through
  `CLP_LOG_INGESTOR_LOGGING_LEVEL` in Docker Compose and
  `clpConfig.log_ingestor.logging_level` in Helm. `api_server` currently runs with
  `RUST_LOG=INFO`.
* `CLP_LOGS_DIR`, when set, adds an hourly non-blocking rolling file appender.

### WebUI

The WebUI server uses Fastify's Pino logger:

* Non-interactive runtimes emit Pino JSON to stdout.
* Interactive terminals use `pino-pretty` for developer-readable output.
* `LOG_LEVEL` controls the server log level and defaults to `info`.

The WebUI client logs to the browser console. Treat client `console.*` output as browser diagnostics,
not service logs.

Example:

<!-- markdownlint-disable MD013 -->
```json
{"level":30,"time":1782480774533,"pid":1,"hostname":"webui","reqId":"req-1h","res":{"statusCode":200},"responseTime":1.0457000732421875,"msg":"request completed"}
```
<!-- markdownlint-enable MD013 -->

### Native core binaries and package tools

Native core binaries use `spdlog` text output. Package controller commands and one-shot setup scripts
also use human-readable stdlib logging. These tools are not covered by the Python/Rust/WebUI service
JSON contracts.


## Deployment notes

* Docker Compose service stdout is available through `docker compose logs`.
  * Compose services that set `CLP_LOGS_DIR` also write under the mounted CLP log directory, which
    defaults to `./var/log` through `CLP_LOGS_DIR_HOST`.
* Helm deployments should use pod stdout through `kubectl logs` or the cluster log
  collector. File logging is template-specific.

[EnvFilter]: https://docs.rs/tracing-subscriber/latest/tracing_subscriber/filter/struct.EnvFilter.html
[clp-py-logging]: https://github.com/y-scope/clp/blob/DOCS_VAR_CLP_GIT_REF/components/clp-py-utils/clp_py_utils/clp_logging.py
[clp-rust-logging]: https://github.com/y-scope/clp/blob/DOCS_VAR_CLP_GIT_REF/components/clp-rust-utils/src/logging.rs

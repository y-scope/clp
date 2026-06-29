# Operator guide: Consuming CLP logs

This guide details how to configure internal log levels, capture service logs,
and understand the component-specific log structures emitted by a running CLP deployment.
The table below provides a high-level overview of the logging behaviors
across all component families:

| Component family              | Components                                                            | Logger                                                    | Format                           | Level control              |
|-------------------------------|---------------------------------------------------------------------|-----------------------------------------------------------|----------------------------------|----------------------------|
| Python orchestration services | `compression_scheduler`, `query_scheduler`, `compression_worker`, `query_worker`, `reducer`, `garbage_collector`, `mcp_server` | [`structlog` with stdlib logging compatibility][clp-py-logging]              | JSON                             | `CLP_LOGGING_LEVEL`        |
| Rust HTTP services            | `api_server`, `log_ingestor`                                        | [`clp_rust_utils::logging`][clp-rust-logging] / `tracing` | JSON                             | `RUST_LOG`                 |
| WebUI server                  | Fastify server                                                      | [Fastify/Pino][pino]                                              | JSON or pretty text | `LOG_LEVEL`                |
| WebUI client                  | Browser app                                                         | Browser console                                           | Browser console output           | Browser/devtools dependent |
| Native core binaries          | `clp`, `clp-s`, `glt`, native `reducer_server`                      | `spdlog`                                                  | Text                             | Binary-specific            |
| Package/setup tools           | Package controller, DB initialization scripts                       | Python stdlib logging                                     | Text                             | Script-specific            |

## Log level configuration

This section covers how to modify logging verbosity for different components.
Use the following environment variables and settings to filter
logs for each component family:

* **Python orchestration services**: `CLP_LOGGING_LEVEL` supports `DEBUG`, `INFO`, `WARN`, `WARNING`, `ERROR`, and `CRITICAL`.
  Missing or invalid values default to `INFO`.
* **Rust HTTP services**: Configure log filtering with [tracing_subscriber::EnvFilter][EnvFilter].
  Filter directives are read from the `RUST_LOG` environment variable to determine which spans and
  events are enabled.
  * *`log-ingestor`*: Log level is exposed via `CLP_LOG_INGESTOR_LOGGING_LEVEL`
  (Docker Compose) or `clpConfig.log_ingestor.logging_level` (Helm), which is
  then passed to `RUST_LOG`.
  * *`api-server`*: Log level is hardcoded to `INFO` and does not expose a deployment
  variable for configuration.
* **WebUI**: `LOG_LEVEL` controls the Pino server log level (defaults to `info`).
:::{warning}
`LOG_LEVEL` is a generic environment variable. Be mindful of environment
variable collisions with other tools or container settings.
:::

## Log routing and collection

Access to CLP package logs is entirely dependent on where the component is executed.

### Containerized services

For continuously running deployed services such as Python orchestration services, Rust HTTP services, and WebUI server, log routing depends on the orchestrator:

* **Docker Compose**: Logs are available using `docker compose logs`. If `CLP_LOGS_DIR` is set,
logs are additionally written to `<CLP_LOGS_DIR>/<component_name>.log`. This directory mounts to the host via `CLP_LOGS_DIR_HOST` (defaults to `./var/log`).
:::{note}
For Rust services, setting `CLP_LOGS_DIR` enables file logging with hourly log rotation and a
non-blocking file writer.
:::
* **Kubernetes/Helm**: Logs are accessible using `kubectl logs` or a cluster log collector (e.g., Fluent Bit).
File logging is only enabled for templates that set `CLP_LOGS_DIR` and mount a log volume.

### Standalone tools

* **Core binaries**: Running standalone binaries (`clp`, `clp-s`, etc.) emits
unstructured `spdlog` text to `stdout`.
:::{note}
*When deployed*, Python orchestration services invoke these binaries as subprocesses.
Consequently, the core binaries' unstructured log text is captured and written to the
Python logger's output stream as a single multi-line JSON record. These appear alongside
normal container logs in Docker or Kubernetes.
:::
* **WebUI client**: WebUI client (`console.*`) logs remain local to the user's browser devtools
and are *not* captured by any backend telemetry service. They should be treated
purely as local diagnostics.
* **Package scripts**: Package controller commands and one-shot
setup scripts emit unstructured Python stdlib logs to `stdout`.

## Component-specific log schemas

There is no single project-wide JSON schema. Python, Rust, and Pino logs are all line-delimited JSON
in packaged non-interactive service runtimes, but each component family uses its own field names.
The following sections describe the log schema for each component family.

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

### WebUI server

The WebUI server uses Fastify's Pino logger. When the server runs without an interactive terminal,
such as in Docker Compose or Kubernetes, it emits one JSON object per log record. When `stdout` is an
interactive terminal, it uses `pino-pretty` for human-readable output.

Each JSON record includes the following fields:

| Field          | Description                                             |
|----------------|---------------------------------------------------------|
| `level`        | Numeric Pino log level.                                 |
| `time`         | Unix timestamp in milliseconds.                         |
| `pid`          | Process ID.                                             |
| `hostname`     | Hostname of the process emitting the log.               |
| `reqId`        | Fastify request ID, when the record is request-scoped.  |
| `req`, `res`   | Request and response metadata, when available.          |
| `responseTime` | Request duration in milliseconds, when available.       |
| `msg`          | Log message.                                            |
| Extra fields   | Structured fields passed to the Pino log call.          |

Example:

<!-- markdownlint-disable MD013 -->
```json
{"level":30,"time":1782480774533,"pid":1,"hostname":"webui","reqId":"req-1h","res":{"statusCode":200},"responseTime":1.0457000732421875,"msg":"request completed"}
```
<!-- markdownlint-enable MD013 -->

### Core and package tools

Native core binaries emit unstructured `spdlog` text logs. Package controller commands and one-shot
setup scripts also emit unstructured Python stdlib logs. These tools do not follow the JSON log
formats described for Python orchestration services, Rust HTTP services, or the WebUI server.

[clp-py-logging]: https://github.com/y-scope/clp/blob/DOCS_VAR_CLP_GIT_REF/components/clp-py-utils/clp_py_utils/clp_logging.py
[clp-rust-logging]: https://github.com/y-scope/clp/blob/DOCS_VAR_CLP_GIT_REF/components/clp-rust-utils/src/logging.rs
[pino]: https://fastify.dev/docs/v2.15.x/Documentation/Logging/
[EnvFilter]: https://docs.rs/tracing-subscriber/latest/tracing_subscriber/filter/struct.EnvFilter.html

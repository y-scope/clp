# Operator guide: consuming CLP logs

This guide details how to configure internal log levels, capture service logs, and understand the
component-specific log structures emitted by a running CLP deployment.

| Component family              | Components                                                                                                                     | Logger                                                          | Format                           | Level control              |
|-------------------------------|--------------------------------------------------------------------------------------------------------------------------------|-----------------------------------------------------------------|----------------------------------|----------------------------|
| Python orchestration services | `compression_scheduler`, `query_scheduler`, `compression_worker`, `query_worker`, `reducer`, `garbage_collector`, `mcp_server` | [`structlog` with stdlib logging compatibility][clp-py-logging] | JSON                             | `CLP_LOGGING_LEVEL`        |
| Rust HTTP services            | `api_server`, `log_ingestor`                                                                                                   | [`clp_rust_utils::logging`][clp-rust-logging] / `tracing`       | JSON                             | `RUST_LOG`                 |
| WebUI server                  | Fastify server                                                                                                                 | Fastify/Pino                                                    | JSON in prod; pretty text in dev | `LOG_LEVEL`                |
| WebUI client                  | Browser app                                                                                                                    | Browser console                                                 | Browser console output           | Browser/devtools dependent |
| Native core binaries          | `clp`, `clp-s`, `glt`, native `reducer_server`                                                                                 | `spdlog`                                                        | Text                             | Binary-specific            |
| Package/setup tools           | Package controller, DB initialization scripts                                                                                  | Python stdlib logging                                           | Text                             | Script-specific            |

:::{note}
There is no single project-wide JSON schema. Python, Rust, and Pino logs are all line-delimited JSON
in packaged non-interactive service runtimes, but each component family uses its own field names.
:::

## Configuration

### Log level configuration

* **Python orchestration services**: `CLP_LOGGING_LEVEL` supports `DEBUG`, `INFO`, `WARN`,
  `WARNING`, `ERROR`, and `CRITICAL`. Missing or invalid values default to `INFO`.
  * `CLP_LOGS_DIR`, when set, adds a file handler at `<CLP_LOGS_DIR>/<component_name>.log` in
    addition to stdout.
* **Rust HTTP services**: Configure log filtering with [tracing_subscriber::EnvFilter][EnvFilter].
  Filter directives are read from the `RUST_LOG` environment variable to determine which spans and
  events are enabled.
  * *Note on `log_ingestor`*: Level is exposed via `CLP_LOG_INGESTOR_LOGGING_LEVEL` (Docker Compose)
    or `clpConfig.log_ingestor.logging_level` (Helm), which is then passed to `RUST_LOG`.
  * *Note on `api_server`*: Currently runs hardcoded at INFO and does not expose a deployment
    setting.
* **WebUI**: `LOG_LEVEL` controls the Pino server log level (defaults to `info`).
  * *Warning*: Be mindful of environment variable collisions with LOG_LEVEL in shared container
    spaces.

### Deployment notes and log output

* **`docker-compose`**: Service stdout is available via `docker compose logs`. If `CLP_LOGS_DIR` is
  set, logs are additionally written to `<CLP_LOGS_DIR>/<component_name>.log` (which mounts to
  `./var/log` via `CLP_LOGS_DIR_HOST`). For Rust components, this adds an hourly non-blocking
  rolling file appender.
* **`helm`**: Rely on pod stdout via `kubectl logs` or a cluster log collector (e.g., Fluent Bit).
  File logging is template-specific.
* **WebUI**:
  * **Server**: In non-interactive deployments, the Fastify server emits Pino JSON directly to
    `stdout`. If run in an interactive terminal (e.g., local development), it uses `pino-pretty` for
    human-readable output.
  * **Client**: Browser logs (`console.*`) remain local to the user's browser devtools and are *not*
    captured by backend service telemetry. Treat these purely as local diagnostics.
* **Core**: Native binaries (clp, clp-s, etc.) emit standard human-readable text. Currently, Python
  orchestration services invoke native core binaries (clp, clp-s, etc.) as subprocesses and the core
  binaries' human-readable `spdlog` text is piped into the Python logger's output stream as a single
  multi-line JSON record.
* **Setup tools**: Package controller scripts emit standard human-readable text. They do not adhere
  to the JSON schemas used by the orchestration services and rely strictly on standard output
  streams.

## Component-specific logging details

The following sections describe the behavior for each component family.

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

```json
{
  "timestamp":"2026-06-22T17:03:21.123456Z",
  "event":"Compression job 1 submitted.",
  "logger":"compression_scheduler",
  "level":"info",
  "filename":"compression_scheduler.py",
  "func_name":"main",
  "lineno":616
}
```

### Rust HTTP services

These services use [`clp_rust_utils::logging::set_up_logging`][clp-rust-logging], which configures
`tracing_subscriber` to emit one JSON object per log record. Each record includes the following
fields:

| Field         | Description                                                       |
|---------------|-------------------------------------------------------------------|
| `timestamp`   | Timestamp emitted by `tracing_subscriber`.                        |
| `level`       | Log level, emitted as values such as `INFO`, `WARN`, and `ERROR`. |
| `fields`      | Structured `tracing` fields, including the log message.           |
| `filename`    | Source filename for the log call.                                 |
| `line_number` | Source line number for the log call.                              |

Example:

```json
{
  "timestamp":"2026-06-26T13:06:48.621307",
  "level":"INFO",
  "fields":{
    "message":"Spawned SQS listener task.",
    "job_id":"3",
    "task_id":"0"
  },
  "filename":"components/log-ingestor/src/ingestion_job/sqs_listener.rs",
  "line_number":320
}
```

### WebUI

The WebUI server uses Fastify's Pino logger where each record includes the
following fields:

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

```json
{
  "level":30,
  "time":1782480774533,
  "pid":1,
  "hostname":"webui",
  "reqId":"req-1h",
  "res":{"statusCode":200},
  "responseTime":1.0457000732421875,
  "msg":"request completed"
}
```

### Core and package tools

Native core binaries use `spdlog` text output. Package controller commands and one-shot setup
scripts also use human-readable stdlib logging. These tools are not covered by the Python/Rust/WebUI
service JSON contracts.

[EnvFilter]: https://docs.rs/tracing-subscriber/latest/tracing_subscriber/filter/struct.EnvFilter.html
[clp-py-logging]: https://github.com/y-scope/clp/blob/DOCS_VAR_CLP_GIT_REF/components/clp-py-utils/clp_py_utils/clp_logging.py
[clp-rust-logging]: https://github.com/y-scope/clp/blob/DOCS_VAR_CLP_GIT_REF/components/clp-rust-utils/src/logging.rs

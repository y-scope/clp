# Developer guide: writing logs

This guide provides language-specific setup instructions and standards for initializing loggers and
emitting diagnostic logs when developing or modifying CLP components.

:::{note}
Always prefer UTC service timestamps. Time zone conversions should be handled downstream in log
viewers or aggregation systems.
:::

## Python

Python orchestration services should use `structlog` for structured JSON logging and bind
context variables where appropriate.

```python
from clp_py_utils.clp_logging import get_structlog_logger

log = get_structlog_logger("service_name")
log.info("hello, %s!", "world")
```

:::{note}
Existing Python services use stdlib loggers whose handlers are configured with structlog's
`ProcessorFormatter`. Follow the established convention when modifying those services.
:::

## Rust

Rust HTTP services should initialize `tracing` at process startup using
[`clp_rust_utils::logging::set_up_logging`][clp-rust-logging] and keep the returned guard alive
for the lifetime of the process:

```rust
let _guard = clp_rust_utils::logging::set_up_logging("service_name.log");

// Choose structured logging over formatting values directly into the message field.
tracing::info!(server_address = %addr, "Server started.");
```

## WebUI

WebUI server code should use Fastify's Pino logger. Use `request.log` for request-scoped logs and
`app.log` for startup, shutdown, and application-level logs:

```typescript
// Request-scoped
request.log.info({searchJobId}, "Search submitted");
request.log.error(err, "Failed to submit search");

// Application-scoped
app.log.info("WebUI server listening on port 3000");
```

WebUI client code should use `console.*` for browser diagnostics:

```typescript
console.error("Failed to submit query:", err);
```

## Core and setup tools

* Native core binaries (`clp`, `clp-s`, `glt`, native `reducer_server`) should continue using
  [`spdlog`][spdlog] and the `spdlog` initialization already defined in each binary's startup code.
* Package/setup tools (DB initialization scripts, package controllers) should use the
  [standard Python logger][std-logger].

[clp-rust-logging]: https://github.com/y-scope/clp/blob/DOCS_VAR_CLP_GIT_REF/components/clp-rust-utils/src/logging.rs
[spdlog]: https://github.com/gabime/spdlog
[std-logger]: https://docs.python.org/3/library/logging.html

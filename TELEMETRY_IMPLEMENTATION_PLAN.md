# CLP Telemetry Implementation Plan

## Current State

The consent layer is fully implemented and merged to main:
- `Telemetry` config model in Python (`clp_config.py`) and Rust (`config.rs`) with `disable` and `endpoint` fields
- Consent priority chain in `start_clp.py`: env vars → config file → first-run prompt
- Instance UUID generation in `controller.py` (`get_or_create_instance_id()`)
- Config templates with telemetry sections

**What does NOT exist yet:** OTel SDK initialization, metrics emission code, OTel Collector deployment, telemetry env vars passed to containers, server-side infrastructure.

---

## Architectural Decisions

### AD-1: Resource attribute propagation — `OTEL_RESOURCE_ATTRIBUTES` env var

The spec originally proposed mounting a `clp-runtime-info.json` file only to the OTel Collector, with the collector injecting attributes. We use `OTEL_RESOURCE_ATTRIBUTES` instead — a single, standardized OTel env var that all OTel SDKs read natively.

**Why:** The spec's aversion to env vars was aimed at the original per-variable approach (`CLP_INSTANCE_ID`, `CLP_VERSION`, `CLP_DEPLOYMENT_METHOD`, etc. as separate env vars). `OTEL_RESOURCE_ATTRIBUTES` is fundamentally different: it's one env var with a standardized key=value format that the OTel SDK consumes automatically. No file-reading code, no JSON parsing, no custom collector processor, no template substitution. Every OTel SDK in every language supports it out of the box.

**Consequence:** The controller writes `OTEL_RESOURCE_ATTRIBUTES` and `OTEL_SERVICE_NAME` to `.env` (Docker Compose) or ConfigMap (Helm). Each service's SDK picks them up automatically. The collector is a simple pass-through — no resource processors needed.

### AD-2: OTLP/HTTP (not gRPC) for all exporters

**Why:** No protobuf compiler, no gRPC dependency, no tonic/prost in Rust. The telemetry volume is low — HTTP efficiency is fine. HTTP works through more proxies and firewalls.

### AD-3: `service.version` (standard OTel semantic convention) instead of `clp.version`

**Why:** `service.version` is the OTel standard for software version. Using it means standard tooling (Grafana, ClickHouse OTel integration) recognizes it automatically. Keep `clp.*` prefix only for truly custom attributes with no standard equivalent.

### AD-4: Let the OTel SDK auto-populate `service.instance.id`

**Why:** Some SDKs auto-assign this (hostname + PID). It lets the backend count unique running instances, distinguishing "2 pods each with concurrency 4" from "1 pod with concurrency 8." Don't override it manually.

---

## Phase 1: Foundation — Shared Telemetry Libraries

### Step 1.1: Add OTel dependencies to Rust crates

Add to `components/clp-rust-utils/Cargo.toml`:

```toml
[dependencies]
opentelemetry = "0.27"
opentelemetry_sdk = { version = "0.27", features = ["metrics", "rt-tokio"] }
opentelemetry-otlp = { version = "0.27", features = ["metrics", "http-proto", "reqwest-client"] }
```

We use `http-proto` + `reqwest-client`. `reqwest` is already in the dependency tree via AWS SDKs. This avoids adding `tonic` + `prost` + `protoc`, which the gRPC transport would require.

**Note on Rust OTel maturity:** The Rust `opentelemetry` metrics API (0.27+) is stable but less battle-tested than Python's. If stability issues arise during implementation, Step 3.3 describes a fallback approach (manual OTLP/HTTP JSON push) that uses fewer dependencies.

### Step 1.2: Add OTel dependencies to Python packages

Add to `components/clp-py-utils/pyproject.toml` (shared, so all Python services get it):

```toml
[dependencies]
opentelemetry-api = ">=1.0"
opentelemetry-sdk = ">=1.0"
opentelemetry-exporter-otlp-proto-http = ">=1.0"
```

Using HTTP transport for consistency with Rust. The `clp-py-utils` package is shared by all Python services (`job-orchestration`, `clp-package-utils`), so adding the dependency here means each service doesn't need to declare it separately. The MCP server also pulls it in transitively via `fastmcp`, so there's no new overhead for that package.

### Step 1.3: Create shared telemetry initialization module — Rust

Create `components/clp-rust-utils/src/telemetry.rs`:

```rust
/// Initialize the OTel metrics pipeline.
///
/// - Reads `OTEL_RESOURCE_ATTRIBUTES` and `OTEL_SERVICE_NAME` env vars automatically (SDK-native).
/// - If `telemetry.disable` is true or `CLP_DISABLE_TELEMETRY` is set, returns a no-op MeterProvider.
/// - Configures the OTLP/HTTP exporter pointing at `OTEL_EXPORTER_OTLP_ENDPOINT`.
/// - Returns the MeterProvider (caller stores it for the process lifetime).
pub fn init_telemetry(telemetry_config: &Telemetry) -> Result<MeterProvider>

/// Gracefully shut down the meter provider, flushing pending exports.
pub fn shutdown_telemetry(meter_provider: MeterProvider)

/// Create a u64 counter on the given meter.
pub fn get_u64_counter(meter: &Meter, name: &str, description: &str, unit: &str) -> Counter<u64>

/// Create an f64 gauge observer on the given meter.
pub fn get_f64_gauge(meter: &Meter, name: &str, description: &str, unit: &str) -> Gauge<f64>
```

The init function should:
1. Check `CLP_DISABLE_TELEMETRY` env var and `telemetry_config.disable` — if either is true, return a no-op `MeterProvider` (all metric operations become no-ops with zero overhead)
2. Build a `Resource` — the SDK automatically reads `OTEL_RESOURCE_ATTRIBUTES` and `OTEL_SERVICE_NAME` env vars, so no manual attribute construction is needed
3. Optionally add `service.version` to the resource (from the CLP version constant if available, or let the controller set it via `OTEL_RESOURCE_ATTRIBUTES`)
4. Create a `PeriodicReader` with an `OtlpHttpExporter` pointing at the `OTEL_EXPORTER_OTLP_ENDPOINT` env var
5. Return the `MeterProvider`

### Step 1.4: Create shared telemetry initialization module — Python

Create `components/clp-py-utils/clp_py_utils/telemetry.py`:

```python
def init_telemetry(telemetry_config: Telemetry) -> MeterProvider:
    """Initialize the OTel metrics pipeline.

    - Reads OTEL_RESOURCE_ATTRIBUTES and OTEL_SERVICE_NAME env vars automatically (SDK-native).
    - If telemetry.disable is true or CLP_DISABLE_TELEMETRY is set, returns a no-op MeterProvider.
    - Configures the OTLP/HTTP exporter pointing at OTEL_EXPORTER_OTLP_ENDPOINT.
    - Returns the MeterProvider (caller stores it for the process lifetime).
    """

def shutdown_telemetry() -> None:
    """Gracefully shut down the meter provider, flushing pending exports."""

def get_u64_counter(meter: Meter, name: str, description: str, unit: str = "") -> Counter:
    """Create a u64 counter on the given meter."""

def get_f64_gauge(meter: Meter, name: str, description: str, unit: str = "") -> ObservableGauge:
    """Create an f64 observable gauge on the given meter."""
```

Same logic as Rust: check disable flag, let the SDK read env vars automatically, configure OTLP/HTTP exporter.

For Celery workers, add a helper:
```python
def init_telemetry_for_celery_worker(telemetry_config: Telemetry) -> None:
    """Initialize telemetry in a Celery worker process.

    Must be called from a worker_init signal handler (post-fork),
    NOT at import time. Celery prefork workers share state before
    the fork — initializing the SDK at import time produces duplicate
    and incorrect metrics.
    """
```

---

## Phase 2: Client-Side OTel Collector

### Step 2.1: Create OTel Collector configuration

Create `tools/deployment/package/otel-collector-config.yaml`:

```yaml
receivers:
  otlp:
    protocols:
      http:
        endpoint: 0.0.0.0:4318

processors:
  batch:
    timeout: 30s
    send_batch_size: 1024

exporters:
  otlphttp:
    endpoint: ${TELEMETRY_ENDPOINT}
    # TLS is default for port 443

service:
  pipelines:
    metrics:
      receivers: [otlp]
      processors: [batch]
      exporters: [otlphttp]
```

This is intentionally simple. The collector is a pass-through: receive OTLP/HTTP from services, batch, forward to the server. No resource processors, no transforms — all resource attributes come from the services themselves via `OTEL_RESOURCE_ATTRIBUTES`.

The `TELEMETRY_ENDPOINT` env var is set to `https://telemetry.yscope.io` (from `clp_config.telemetry.endpoint`).

### Step 2.2: Add OTel Collector to Docker Compose

Add a `otel-collector` service to `docker-compose-all.yaml`:

```yaml
otel-collector:
  image: otel/opentelemetry-collector-contrib:0.96.0
  volumes:
    - ./otel-collector-config.yaml:/etc/otelcol-contrib/config.yaml
  environment:
    - TELEMETRY_ENDPOINT
  # Do NOT map port 4318 to the host (internal only)
  networks:
    - default
```

**Collector image:** Use the `contrib` distribution. It includes the `transform` and `resourcedetection` processors in case we need them later, and the image size difference vs. core is negligible.

**Network exposure:** Port 4318 is internal only — services reach it via the Docker network at `http://otel-collector:4318`. Do not expose to the host in production. Optionally, create a `docker-compose.dev.yaml` override that maps `4318:4318` for local debugging with `otel-cli`.

**Conditionally deploy:** The collector should only be included when telemetry is enabled. In `controller.py`, generate the compose command to include/exclude the collector service based on `clp_config.telemetry.disable`. Alternatively, use a compose profile:

```yaml
otel-collector:
  profiles: ["telemetry"]
  ...
```

Then `docker compose --profile telemetry up` only when telemetry is enabled.

### Step 2.3: Add OTel Collector to Helm chart

Add to `tools/deployment/package-helm/`:
- `templates/otel-collector-deployment.yaml` — Deployment (1 replica)
- `templates/otel-collector-service.yaml` — ClusterIP service on port 4318
- `templates/otel-collector-configmap.yaml` — Collector config from template
- Update `values.yaml` with collector settings (image, resources)

**Collector deployment pattern:** A single Deployment (1 replica) is sufficient. The telemetry volume is low — if the collector crashes, Kubernetes restarts it and services buffer retries via the OTLP exporter. A DaemonSet or sidecar pattern adds unnecessary complexity at this scale.

**Conditional deploy:** Wrap the collector templates in a conditional:
```yaml
{{- if not .Values.clpConfig.telemetry.disable }}
```

---

## Phase 3: Service Instrumentation

### Step 3.1: Instrument `log-ingestor` (Rust)

In `components/log-ingestor/src/bin/log_ingestor.rs`:

1. After config loading, call `clp_rust_utils::telemetry::init_telemetry(&config.telemetry)` — store the `MeterProvider` for the process lifetime
2. Create counters: `clp.ingest.bytes_total`, `clp.ingest.records_total`
3. Add `.increment(n)` calls in the ingestion code path — find where bytes and record counts are already tracked (in `ingestion_job_manager.rs` and `clp_ingestion.rs`) and add the OTel counter increment adjacent to existing tracking
4. On shutdown, call `clp_rust_utils::telemetry::shutdown_telemetry(meter_provider)`

**Where to increment counters:** The ingestion job manager already tracks bytes and records per job. Add the counter increment at the same point where these values are logged or reported to the scheduler. Don't double-count — increment only when a job completes successfully.

### Step 3.2: Instrument `api-server` (Rust)

The api-server doesn't have dedicated metrics in the spec, but it should emit:
- `service.name=api-server` in resource attributes (for presence/liveness) — automatic via `OTEL_SERVICE_NAME`
- A startup event (`clp.service.event` with `type=start` attribute) so the backend knows the service is running

Do not emit query metrics from the api-server. The query workers (Python/Celery) are the ones that scan data.

### Step 3.3: Rust instrumentation fallback — manual OTLP/HTTP push

If the Rust `opentelemetry` crate proves unstable (the metrics API went through a major rewrite between 0.20 and 0.27), a simpler fallback is to bypass the SDK entirely and push OTLP JSON directly:

```rust
struct OtlpMetricsPusher {
    endpoint: String,
    client: reqwest::Client,
    counters: Arc<Mutex<HashMap<String, AtomicU64>>>,
    resource_attrs: String,  // from OTEL_RESOURCE_ATTRIBUTES
    service_name: String,    // from OTEL_SERVICE_NAME
}

impl OtlpMetricsPusher {
    /// Serialize all counters to OTLP JSON format and POST to the collector.
    async fn push(&self) { ... }

    /// Spawn a tokio interval that pushes every 30 seconds.
    async fn start(self) -> JoinHandle<()> { ... }
}
```

This approach:
- Uses only `reqwest` (already in the tree) — no `opentelemetry-*` crates
- Constructs the OTLP/HTTP JSON payload manually (the format for metrics is straightforward)
- Implements simple batching and retry (exponential backoff)
- Reads `OTEL_RESOURCE_ATTRIBUTES` and `OTEL_SERVICE_NAME` at startup to include in the resource payload

**Decision point:** Start with the full OTel SDK (Step 1.1). If stability issues arise during testing, switch to this fallback. The SDK gives us batching, retry, and resource attribute propagation for free. The fallback gives us fewer dependencies and full control.

### Step 3.4: Instrument `compression-worker` (Python/Celery)

In `components/job-orchestration/job_orchestration/executor/compress/`:

1. Register a Celery `worker_init` signal handler that calls `init_telemetry_for_celery_worker(clp_config.telemetry)` — this initializes the SDK after the fork
2. Create counters: `clp.compression.bytes_input_total`, `clp.compression.bytes_output_total`
3. Increment in the compression task after each compression operation

**Important — Celery prefork:** Celery workers use prefork by default. The OTel SDK must be initialized AFTER the fork (in the `worker_init` signal), not at import time. Otherwise, SDK state is shared across forked processes and produces duplicate/incorrect metrics.

```python
from celery.signals import worker_process_init

@worker_process_init.connect
def on_worker_init(**kwargs):
    from clp_py_utils.telemetry import init_telemetry_for_celery_worker
    init_telemetry_for_celery_worker(clp_config.telemetry)
```

### Step 3.5: Instrument `query-worker` (Python/Celery)

In `components/job-orchestration/job_orchestration/executor/query/`:

1. Same `worker_init` signal pattern as compression-worker
2. Create counters: `clp.query.bytes_scanned_total`, `clp.query.bytes_output_total`
3. Increment in the query execution task

### Step 3.6: Emit deployment topology gauges

Topology gauges report replica counts and concurrency values — deployment configuration, not per-process state.

**Who emits them:** The controller (Docker Compose) or scheduler pods (Helm), since they know the deployment configuration.

**Docker Compose:** The controller emits topology gauges once at startup. It already knows the concurrency values (it sets `CLP_*_WORKER_CONCURRENCY` in `.env`) and replica counts (effectively 1 per container in Docker Compose — scaling means running multiple containers). The simplest approach is to have the controller write a small Python script or use the `curl`-based OTLP metric push to emit these gauges once after starting all services.

**Helm:** The compression-scheduler pod emits `clp.deployment.compression_worker_replicas` and `clp.deployment.compression_worker_concurrency` at startup. Same for the query-scheduler and reducer. The schedulers read these values from their environment (injected from `values.yaml` via the Helm chart).

**Decision — Who emits topology gauges?**

| Option | Emitter | Pros | Cons |
|--------|---------|------|------|
| A: Controller (Docker Compose) / Scheduler (Helm) | Knows deployment config; emits once at startup | Simple; accurate at deploy time | Not aware of runtime scaling changes (rare for CLP) |
| B: Each worker self-reports | Each worker emits `clp.worker.concurrency=N` | Decentralized; no single point of failure | Backend must aggregate; can't distinguish "1 worker concurrency 4" from "4 workers concurrency 1" without `service.instance.id` |
| C: OTel Collector `resource` processor | Inject topology centrally | Services are unaware | Requires template-substituted collector config; drifts from AD-1 |

**Recommendation: Option A.** The controller already has all the values. Docker Compose deployments rarely scale at runtime. Helm deployments that scale would need the scheduler to re-emit, which is a natural extension.

**Metrics to emit:**

| Metric | Type | Description |
|--------|------|-------------|
| `clp.deployment.compression_worker_replicas` | Gauge | Number of compression-worker instances |
| `clp.deployment.compression_worker_concurrency` | Gauge | Processes per compression-worker instance |
| `clp.deployment.query_worker_replicas` | Gauge | Number of query-worker instances |
| `clp.deployment.query_worker_concurrency` | Gauge | Processes per query-worker instance |
| `clp.deployment.reducer_replicas` | Gauge | Number of reducer instances |
| `clp.deployment.reducer_concurrency` | Gauge | Processes per reducer instance |

### Step 3.7: Add startup/shutdown event metrics

Each service should emit a startup counter when it starts and a shutdown counter when it stops gracefully. This answers "is this service running?" and "how often does it restart?"

**Design:** Single counter with `type` attribute (standard OTel pattern for event counting):

| Metric | Attribute | Example |
|--------|-----------|---------|
| `clp.service.event` | `type=start` | `clp.service.event{type="start", service.name="log-ingestor"} += 1` |
| `clp.service.event` | `type=stop` | `clp.service.event{type="stop", service.name="log-ingestor"} += 1` |

An UpDownCounter (tracking live instance count) is tempting but fragile — if a service crashes without graceful shutdown, the count stays inflated. The counter approach is simpler and more reliable.

### Step 3.8: Error summary metrics

**Design:** Counter with a small, predefined set of `error_type` attribute values:

| Metric | Attribute | Example |
|--------|-----------|---------|
| `clp.service.errors` | `error_type=db_connection` | `clp.service.errors{error_type="db_connection", service.name="log-ingestor"} += 1` |
| `clp.service.errors` | `error_type=queue_connection` | |
| `clp.service.errors` | `error_type=storage_write` | |
| `clp.service.errors` | `error_type=unknown` | |

**Cardinality constraint:** The set of `error_type` values is predefined and small. Never include error messages, stack traces, or any freeform text as attributes — these are high-cardinality and may contain PII. If an error doesn't fit a category, use `unknown`.

---

## Phase 4: Deployment Integration

### Step 4.1: Write `OTEL_RESOURCE_ATTRIBUTES` in `controller.py`

In `DockerComposeController.set_up_env()`, construct the resource attributes string and write it to `.env`:

```python
if not clp_config.telemetry.disable:
    resource_attrs = (
        f"clp.deployment.id={instance_id},"
        f"service.version={version},"
        f"clp.deployment.method=docker-compose,"
        f"clp.storage.engine={storage_engine},"
        f"os.type={os_type},"
        f"os.version={os_version},"
        f"host.arch={arch}"
    )
    env_vars["OTEL_RESOURCE_ATTRIBUTES"] = resource_attrs
    env_vars["OTEL_EXPORTER_OTLP_ENDPOINT"] = "http://otel-collector:4318"
else:
    env_vars["CLP_DISABLE_TELEMETRY"] = "true"
```

Notes:
- `instance_id` comes from `get_or_create_instance_id()` (already in `controller.py`)
- `version` comes from the package version (already available)
- `os_type`, `os_version`, `arch` come from `platform.system()`, distro detection, `platform.machine()`
- We use `service.version` (not `clp.version`) per AD-3

Each service also needs `OTEL_SERVICE_NAME` set individually. This is a standard OTel env var. Add it per-service in `docker-compose-all.yaml`:

```yaml
log-ingestor:
  environment:
    - OTEL_SERVICE_NAME=log-ingestor
api-server:
  environment:
    - OTEL_SERVICE_NAME=api-server
compression-worker:
  environment:
    - OTEL_SERVICE_NAME=compression-worker
query-worker:
  environment:
    - OTEL_SERVICE_NAME=query-worker
reducer:
  environment:
    - OTEL_SERVICE_NAME=reducer
compression-scheduler:
  environment:
    - OTEL_SERVICE_NAME=compression-scheduler
query-scheduler:
  environment:
    - OTEL_SERVICE_NAME=query-scheduler
```

### Step 4.2: Add telemetry env vars to Helm values and templates

In `values.yaml`:
```yaml
telemetry:
  collector:
    image: otel/opentelemetry-collector-contrib:0.96.0
    resources:
      requests:
        cpu: 100m
        memory: 128Mi
      limits:
        cpu: 500m
        memory: 256Mi
```

In each deployment template, add:
```yaml
env:
  - name: OTEL_EXPORTER_OTLP_ENDPOINT
    value: "http://otel-collector:4318"
  - name: OTEL_SERVICE_NAME
    value: "log-ingestor"  # per service
  - name: OTEL_RESOURCE_ATTRIBUTES
    value: "{{ include "clp.resourceAttributes" . }}"
```

Create a Helm helper in `_helpers.tpl`:
```yaml
{{- define "clp.resourceAttributes" -}}
clp.deployment.id={{ .Values.clpConfig.instanceId | default (uuidv4) }},
service.version={{ .Chart.AppVersion }},
clp.deployment.method=helm,
clp.storage.engine={{ .Values.clpConfig.package.storageEngine }},
os.type={{ .Capabilities.KubeVersion | default "linux" }},
host.arch={{ .Values.hostArch | default "amd64" }}
{{- end -}}
```

### Step 4.3: Propagate disable state to all services

When `telemetry.disable = true`:
- Set `CLP_DISABLE_TELEMETRY=true` on all service containers (this env var already exists from the consent layer)
- Do NOT set `OTEL_EXPORTER_OTLP_ENDPOINT` or `OTEL_RESOURCE_ATTRIBUTES`
- Do NOT deploy the OTel Collector (use compose profile or conditional template)
- Each service's OTel init code checks `CLP_DISABLE_TELEMETRY` and returns a no-op MeterProvider

This is explicit and ensures the SDK is never initialized. No wasted memory, no failed export warnings.

### Step 4.4: Handle first-run prompt before deploying the collector

The first-run prompt (already implemented in `start_clp.py`) runs before containers start. If the user declines telemetry:
- `telemetry.disable = true` is persisted in `clp-config.yaml`
- The controller reads this and skips the OTel Collector (compose profile exclusion)
- `CLP_DISABLE_TELEMETRY=true` is set on all services
- No `OTEL_*` env vars are passed

If the user accepts:
- `telemetry.disable = false` (default)
- Controller includes the OTel Collector (compose profile inclusion)
- `OTEL_EXPORTER_OTLP_ENDPOINT` and `OTEL_RESOURCE_ATTRIBUTES` are set on all services

This flow is already handled by the consent layer. The controller just needs to read `clp_config.telemetry.disable` before deciding whether to include the collector profile.

### Step 4.5: Pass `TELEMETRY_ENDPOINT` to the collector

The OTel Collector's `otlphttp` exporter needs the server endpoint. In `controller.py`:

```python
env_vars["TELEMETRY_ENDPOINT"] = clp_config.telemetry.endpoint  # "https://telemetry.yscope.io"
```

The collector config references `${TELEMETRY_ENDPOINT}`. This is the only env var the collector needs.

---

## Phase 5: Server-Side Telemetry Infrastructure

### Step 5.1: Create `telemetry_server/` directory structure

```
tools/deployment/telemetry_server/
├── docker-compose.yaml          # OTel Collector + ClickHouse + Grafana + Caddy
├── otel-collector-config.yaml   # Server-side collector config
├── caddy/
│   └── Caddyfile                # Reverse proxy with auto-TLS
├── clickhouse/
│   └── init.sql                 # Schema for metrics tables (optional; exporter can auto-create)
├── grafana/
│   ├── provisioning/
│   │   ├── datasources/
│   │   │   └── clickhouse.yaml  # ClickHouse data source
│   │   └── dashboards/
│   │       └── clp.yaml         # Dashboard provisioning
│   └── dashboards/
│       └── clp-overview.json    # Pre-built dashboard
└── README.md                    # Setup instructions
```

### Step 5.2: Server-side OTel Collector configuration

The server-side collector receives metrics from client collectors and writes to ClickHouse:

```yaml
receivers:
  otlp:
    protocols:
      http:
        endpoint: 0.0.0.0:4318

processors:
  batch:
    timeout: 10s
    send_batch_size: 5000
  # Optional: rate limiting to prevent abuse
  # ratelimit:
  #   rate_limiter:
  #     enabled: true

exporters:
  clickhouse:
    endpoint: tcp://clickhouse:9000
    database: clp_telemetry
    ttl: 90d
    metrics_table_name: otel_metrics
    # traces and logs tables must be specified but won't be used
    traces_table_name: otel_traces
    logs_table_name: otel_logs

service:
  pipelines:
    metrics:
      receivers: [otlp]
      processors: [batch]
      exporters: [clickhouse]
```

**ClickHouse exporter:** Use the `clickhouse` exporter from the contrib distribution. It's purpose-built for this pairing and supports TTL for automatic data expiration. The `sql` exporter is more flexible but doesn't use ClickHouse-specific optimizations. VictoriaMetrics as an intermediate adds unnecessary moving parts for low-volume telemetry.

### Step 5.3: ClickHouse schema

Start by letting the `clickhouse` exporter auto-create tables. Once query patterns are understood, migrate to a custom schema with materialized columns for common queries:

```sql
CREATE DATABASE IF NOT EXISTS clp_telemetry;

-- Optional: pre-create with optimized schema once query patterns are known
CREATE TABLE IF NOT EXISTS clp_telemetry.otel_metrics
(
    timestamp DateTime64(9),
    metric_name String,
    metric_type String,
    value Float64,
    attributes Map(String, String),
    resource_attributes Map(String, String),
    service_name String MATERIALIZED resource_attributes['service.name'],
    deployment_id String MATERIALIZED resource_attributes['clp.deployment.id'],
    service_version String MATERIALIZED resource_attributes['service.version'],
    storage_engine String MATERIALIZED resource_attributes['clp.storage.engine']
)
ENGINE = MergeTree()
PARTITION BY toYYYYMM(timestamp)
ORDER BY (deployment_id, service_name, metric_name, timestamp)
TTL toDateTime(timestamp) + INTERVAL 90 DAY;
```

The materialized columns (`service_name`, `deployment_id`, `service_version`, `storage_engine`) avoid repeated Map lookups in queries.

### Step 5.4: Grafana dashboards

Create at minimum:

1. **CLP Overview** — Deployment count, version distribution, storage engine split, OS/arch breakdown
2. **Ingest** — Bytes/records ingested over time, per-deployment breakdown
3. **Compression** — Compression ratio over time, bytes in vs out
4. **Query** — Bytes scanned/returned over time, query activity

These can start simple and be iterated once real data flows.

### Step 5.5: TLS/HTTPS via Caddy

Caddy acts as a reverse proxy in front of the collector, providing automatic Let's Encrypt certificate provisioning and renewal:

```
telemetry.yscope.io {
    reverse_proxy otel-collector:4318
}
```

This is simpler than having the collector manage TLS directly (which requires cert rotation and restarts) and avoids cloud-specific load balancer dependencies. Caddy is a single binary with minimal config.

### Step 5.6: Authentication

Start with no authentication + rate limiting. The metrics are anonymous and non-sensitive — rate limiting per source IP (using the `ratelimit` processor) prevents abuse. If spam becomes a problem later, add an API key header (`X-API-Key`) that the collector validates via `basicauth` or a `headers` check.

mTLS is overkill for anonymous metrics and would complicate the Docker Compose client experience.

---

## Phase 6: Documentation

### Step 6.1: Update user-facing telemetry docs

Update `docs/src/user-docs/reference-telemetry.md` with the actual implementation (the current doc is aspirational). Use the content from the spec's Documentation section:

- What we collect (metrics table, resource attributes table)
- What we do NOT collect (PII, log content, query content, IP addresses, etc.)
- How to opt out (env vars, config file, first-run prompt)
- Where the deployment ID comes from and why it's safe

### Step 6.2: Add developer docs for adding new metrics

Create a short guide for contributors:

1. Define the metric (name, type, description, which service emits it)
2. Add the counter/gauge in the service's init code (using the shared telemetry module)
3. Add increment/observe calls in the business logic
4. Update the documentation table

### Step 6.3: Add telemetry server setup guide

Document how to deploy the `telemetry_server/` Docker Compose stack, including:
- DNS configuration for `telemetry.yscope.io`
- Caddy TLS setup
- ClickHouse data persistence
- Grafana admin credentials

---

## Phase 7: Testing and Verification

### Step 7.1: Unit tests for telemetry initialization

Test that:
- OTel SDK initializes correctly when telemetry is enabled
- No-op MeterProvider is returned when `telemetry.disable = true`
- No-op MeterProvider is returned when `CLP_DISABLE_TELEMETRY=true`
- `OTEL_RESOURCE_ATTRIBUTES` is correctly parsed by the SDK
- `OTEL_SERVICE_NAME` is correctly applied
- OTLP exporter is configured with the right endpoint

### Step 7.2: Integration test with local collector

1. Start a local OTel Collector (via Docker, using the client-side config)
2. Start a CLP service with telemetry enabled
3. Verify metrics appear in the collector's `logging` exporter output
4. Test that disabling telemetry produces no metrics
5. Verify `OTEL_RESOURCE_ATTRIBUTES` values appear in the exported metrics

### Step 7.3: End-to-end test with full stack

1. Deploy the full telemetry stack (collector + ClickHouse + Grafana)
2. Send metrics from a CLP deployment
3. Verify metrics appear in ClickHouse
4. Verify Grafana dashboards render correctly

### Step 7.4: Test opt-out flow

1. Start with telemetry enabled — verify metrics flow
2. Set `CLP_DISABLE_TELEMETRY=true` — verify no metrics are emitted
3. Set `DO_NOT_TRACK=1` — verify no metrics are emitted
4. Set `telemetry.disable: true` in config — verify no metrics are emitted
5. Verify first-run prompt works correctly (already tested in consent layer)

### Step 7.5: Test graceful shutdown

Verify that when a service shuts down:
- Pending metrics are flushed (OTLP exporter flush)
- The shutdown event counter is emitted
- No metrics are lost for services that were running

### Step 7.6: Verify no PII leakage

Inspect all outgoing OTLP payloads (via collector debug logging or network capture) and confirm:
- No log content, query content, or file paths appear in metrics or attributes
- No IP addresses are stored (they're visible in transit but not logged by the collector)
- No hostnames appear (container hostnames in Docker are random, but confirm they're not in resource attributes)
- `clp.deployment.id` is a random UUID, not derived from hardware

---

## Implementation Order

Dependencies between steps determine the order:

1. **Phase 1.1-1.2**: Add OTel dependencies (Rust + Python) — unblocks all service instrumentation
2. **Phase 1.3-1.4**: Create shared telemetry init modules (Rust + Python) — unblocks all service instrumentation
3. **Phase 2.1-2.2**: Create collector config + add to Docker Compose — services need somewhere to send
4. **Phase 4.1**: Write `OTEL_RESOURCE_ATTRIBUTES` in `controller.py` — services need config
5. **Phase 3.1**: Instrument log-ingestor (Rust) — validate the Rust OTel pipeline end-to-end
6. **Phase 3.4**: Instrument compression-worker (Python/Celery) — validate the Python OTel pipeline end-to-end
7. **Phase 7.2**: Integration test with local collector — validate the full pipeline before continuing
8. **Phase 3.2**: Instrument api-server (Rust) — same pattern as log-ingestor
9. **Phase 3.5**: Instrument query-worker (Python/Celery) — same pattern as compression-worker
10. **Phase 3.6**: Emit topology gauges — separate concern, depends on Phase 4.1
11. **Phase 3.7-3.8**: Add startup/shutdown/error events — polish
12. **Phase 2.3 + 4.2**: Add collector to Helm chart + Helm telemetry env vars
13. **Phase 4.3-4.5**: Disable state propagation + first-run integration — cross-cutting
14. **Phase 5.1-5.6**: Server-side infrastructure — can be done in parallel but is last priority
15. **Phase 6.1-6.3**: Documentation
16. **Phase 7.1 + 7.3-7.6**: Full testing

**MVP boundary:** Phases 1 + 2 + 3 (ingest and compression metrics only) + 4 are the minimum viable telemetry. The first 7 steps above are the critical path to a working end-to-end pipeline. Everything else is extension and polish.

---

## Appendix: Complete Resource Attribute Mapping

All attributes are set via `OTEL_RESOURCE_ATTRIBUTES` (one env var, comma-separated key=value pairs) and `OTEL_SERVICE_NAME` (separate env var per service). The OTel SDK reads both automatically.

| Attribute | Source | Set by | Notes |
|-----------|--------|--------|-------|
| `clp.deployment.id` | UUIDv4 from `instance-id` file | Controller | Random; never hardware-derived |
| `service.version` | Package version | Controller | OTel standard (not `clp.version`) |
| `clp.deployment.method` | `docker-compose` or `helm` | Controller / Helm | Custom; no OTel standard |
| `clp.storage.engine` | `clp` or `clp-s` | Controller / Helm | Custom; no OTel standard |
| `os.type` | `platform.system()` | Controller / Helm | Matches OTel semantic convention |
| `os.version` | Distro detection | Controller / Helm | Matches OTel semantic convention |
| `host.arch` | `platform.machine()` | Controller / Helm | Matches OTel semantic convention |
| `service.name` | Per-service name | Docker Compose env / Helm template | Separate `OTEL_SERVICE_NAME` env var |
| `service.instance.id` | Auto-populated by SDK | SDK | Some SDKs use hostname+PID; don't override |

**Example `OTEL_RESOURCE_ATTRIBUTES` value:**
```
clp.deployment.id=550e8400-e29b-41d4-a716-446655440000,service.version=0.9.1,clp.deployment.method=docker-compose,clp.storage.engine=clp-s,os.type=linux,os.version=ubuntu-22.04,host.arch=x86_64
```

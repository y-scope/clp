# log-ingestor-benchmark

An end-to-end benchmark harness for the CLP log-ingestor's database ingestion path. It bypasses SQS
and drives the *real* `ClpIngestionState` -> buffer -> listener -> `CompressionJobSubmitter`
pipeline against a live MySQL database.

## Components

- **mock-ingestor** (`src/bin/mock_ingestor.rs`): Creates `num_jobs` ingestion jobs and drives
  synthetic S3 object metadata through the real ingestion pipeline from `tasks_per_job` concurrent
  tasks per job, throttled to a target per-job entry rate.
- **mock-scheduler** (`src/bin/mock_scheduler.rs`): Stands in for the CLP compression scheduler. It
  polls `compression_jobs` for newly submitted jobs (`status = 0`), mocks the scheduler's metadata
  read traffic by fetching and sorting each job's ingested object metadata, and marks the job
  succeeded (`status = 2`). This lets the ingestor's fire-and-forget completion path advance the
  ingested-object metadata to its terminal state.
- **clp-db**: A `mysql:8.4.0` container. `docker/init-db.sql` pre-creates the `compression_jobs`
  table that the ingestor connector foreign-key-references but does not create itself.

## Metrics

The mock ingestor reports three process-wide average database-call timings (pure database cost,
accumulated by `log_ingestor::telemetry`):

1. **Ingestion** — average ingestion database time *per entry* (reported in microseconds).
2. **Compression submission** — average time per `submit_for_compression` call (reported in
   milliseconds).
3. **Compression completion** — average time per completion database *write* only, excluding the
   polling wait for the compression job to finish (reported in milliseconds).

Set `metrics_report_interval_sec` in `config/ingestor.yaml` to control how often (in seconds) a
periodic report is logged; `0` disables periodic reports. A final summary line
(`final DB-call metrics: ...`) is always logged when the ingestor stops.

## Running with Docker Compose

From this directory:

```bash
docker compose up --build
```

All environment variables have defaults, so no setup is required. To override the database
credentials, set `CLP_DB_USER`, `CLP_DB_PASS`, and/or `CLP_DB_ROOT_PASS` before running.

Stop and remove the database volume with:

```bash
docker compose down -v
```

## Configuration

- `config/ingestor.yaml`: workload and buffer parameters for the mock ingestor.
- `config/scheduler.yaml`: polling and concurrency parameters for the mock scheduler.

Database credentials are supplied through the `CLP_DB_USER` and `CLP_DB_PASS` environment variables
(never in the YAML files).

## Building and running locally

```bash
cargo build -p log-ingestor-benchmark --release --bins

CLP_DB_USER=clp-user CLP_DB_PASS=clp-password \
  ./target/release/mock-scheduler --config components/log-ingestor-benchmark/config/scheduler.yaml
CLP_DB_USER=clp-user CLP_DB_PASS=clp-password \
  ./target/release/mock-ingestor --config components/log-ingestor-benchmark/config/ingestor.yaml
```

Both binaries retry the database connection until it is reachable, so start order does not matter.
Logging verbosity is controlled by `RUST_LOG` (defaults to `info`).

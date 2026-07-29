//! OpenTelemetry metrics for log-ingestor.

use std::sync::LazyLock;

use opentelemetry::metrics::Counter;

/// Records the ingestion of a chunk of S3 objects.
pub fn record_s3_ingestion(num_bytes: u64, num_objects: u64) {
    let metrics = &*METRICS;
    metrics.total_num_bytes.add(num_bytes, &[]);
    metrics.total_num_objects.add(num_objects, &[]);
}

/// Lazily-initialized global metrics instance.
static METRICS: LazyLock<LogIngestorMetrics> = LazyLock::new(|| {
    let meter = opentelemetry::global::meter("log-ingestor");
    LogIngestorMetrics {
        total_num_bytes: meter.u64_counter("clp.ingest.total_num_bytes").build(),
        total_num_objects: meter.u64_counter("clp.ingest.total_num_objects").build(),
    }
});

/// Telemetry metrics for tracking ingested S3 data.
struct LogIngestorMetrics {
    total_num_bytes: Counter<u64>,
    total_num_objects: Counter<u64>,
}

use std::env;

use opentelemetry::global;
use opentelemetry_otlp::WithExportConfig;
use opentelemetry_sdk::{
    Resource,
    metrics::{PeriodicReader, SdkMeterProvider},
};

use crate::{Error, clp_config::package::config::Telemetry};

/// Initializes OpenTelemetry metrics collection with the provided configuration.
///
/// Returns `Ok(None)` if telemetry is disabled, `Ok(Some(provider))` on success,
/// or an `Err` if the metric exporter cannot be built.
///
/// # Errors
///
/// Returns an error if the OTLP metric exporter fails to build (e.g., invalid
/// endpoint configuration or missing HTTP client support).
pub fn init_telemetry(telemetry_config: &Telemetry) -> Result<Option<SdkMeterProvider>, Error> {
    if telemetry_config.disable || env::var("CLP_DISABLE_TELEMETRY").is_ok() {
        return Ok(None);
    }

    let endpoint = env::var("OTEL_EXPORTER_OTLP_ENDPOINT")
        .unwrap_or_else(|_| telemetry_config.endpoint.clone());

    let exporter = opentelemetry_otlp::MetricExporter::builder()
        .with_http()
        .with_endpoint(endpoint)
        .build()
        .map_err(|e| Error::TelemetryExporterBuild(e.to_string()))?;

    let reader = PeriodicReader::builder(exporter, opentelemetry_sdk::runtime::Tokio).build();
    let provider = SdkMeterProvider::builder()
        .with_reader(reader)
        .with_resource(Resource::default())
        .build();

    global::set_meter_provider(provider.clone());
    Ok(Some(provider))
}

/// RAII guard that ensures [`shutdown_telemetry`] is called when the guard is
/// dropped, even if the process unwinds or returns early.
pub struct TelemetryGuard(Option<SdkMeterProvider>);

impl TelemetryGuard {
    /// Creates a new guard wrapping the given meter provider.
    ///
    /// When this guard is dropped, [`shutdown_telemetry`] will be called on the
    /// inner provider, flushing any pending metric exports.
    #[must_use]
    pub const fn new(provider: Option<SdkMeterProvider>) -> Self {
        Self(provider)
    }
}

impl Drop for TelemetryGuard {
    fn drop(&mut self) {
        shutdown_telemetry(self.0.take());
    }
}

pub fn shutdown_telemetry(provider: Option<SdkMeterProvider>) {
    if let Some(p) = provider
        && let Err(err) = p.shutdown()
    {
        eprintln!("Error shutting down telemetry: {err}");
    }
}

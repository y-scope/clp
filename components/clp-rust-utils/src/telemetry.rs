use std::env;

use opentelemetry::global;
use opentelemetry_sdk::{
    Resource,
    metrics::{PeriodicReader, SdkMeterProvider},
};

use crate::{Error, clp_config::package::config::Telemetry};

/// Values accepted by `CLP_DISABLE_TELEMETRY` and `DO_NOT_TRACK` to disable telemetry.
///
/// NOTE: This must be kept consistent with the Python implementation in
/// `clp_package_utils/scripts/start_clp.py`.
const TELEMETRY_DISABLE_VALUES: [&str; 4] = ["1", "true", "yes", "y"];

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
    if telemetry_config.disable
        || env::var("CLP_DISABLE_TELEMETRY")
            .is_ok_and(|v| TELEMETRY_DISABLE_VALUES.contains(&v.trim().to_lowercase().as_str()))
        || env::var("DO_NOT_TRACK")
            .is_ok_and(|v| TELEMETRY_DISABLE_VALUES.contains(&v.trim().to_lowercase().as_str()))
    {
        return Ok(None);
    }

    let exporter = opentelemetry_otlp::MetricExporter::builder()
        .with_http()
        .build()
        .map_err(|e| Error::TelemetryExporterBuild(e.to_string()))?;

    let reader = PeriodicReader::builder(exporter, opentelemetry_sdk::runtime::Tokio).build();

    let resource = Resource::builder().build();

    let provider = SdkMeterProvider::builder()
        .with_reader(reader)
        .with_resource(resource)
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

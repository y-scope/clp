//! OpenTelemetry metrics initialization and lifecycle management.

use std::env;

use opentelemetry_sdk::metrics::{PeriodicReader, SdkMeterProvider};

use crate::{Error, clp_config::package::config::Telemetry};

/// RAII guard that shuts down the meter provider and flushes pending metric exports when dropped.
pub struct TelemetryGuard {
    provider: Option<SdkMeterProvider>,
}

impl TelemetryGuard {
    /// Returns the meter provider for setting the global meter provider.
    ///
    /// # Returns
    ///
    /// The meter provider.
    #[must_use]
    pub fn provider(&self) -> Option<SdkMeterProvider> {
        self.provider.clone()
    }
}

impl Drop for TelemetryGuard {
    fn drop(&mut self) {
        if let Some(p) = self.provider.take()
            && let Err(err) = p.shutdown()
        {
            tracing::error!(err = ? err, "Failed to shutdown OpenTelemetry meter provider.");
        }
    }
}

impl From<Option<SdkMeterProvider>> for TelemetryGuard {
    fn from(provider: Option<SdkMeterProvider>) -> Self {
        Self { provider }
    }
}

/// Initializes OpenTelemetry metrics collection with the provided configuration.
///
/// This function is intended for application startup code and should be called exactly once during
/// process initialization.
///
/// # Returns
///
/// A [`TelemetryGuard`] on success. If telemetry is disabled, the guard's provider will be `None`.
/// The guard will shut down the meter provider and flush pending metric exports when dropped.
/// Callers should bind it to a variable (e.g., `let _guard = ...`) to keep it alive for the desired
/// scope.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`opentelemetry_otlp::MetricExporterBuilder::build`]'s return values on failure.
pub fn init_telemetry(telemetry_config: &Telemetry) -> Result<TelemetryGuard, Error> {
    if telemetry_config.disable
        || env::var("CLP_DISABLE_TELEMETRY")
            .is_ok_and(|v| TELEMETRY_DISABLE_VALUES.contains(&v.trim().to_lowercase().as_str()))
        || env::var("DO_NOT_TRACK")
            .is_ok_and(|v| TELEMETRY_DISABLE_VALUES.contains(&v.trim().to_lowercase().as_str()))
    {
        return Ok(TelemetryGuard::from(None));
    }

    let exporter = opentelemetry_otlp::MetricExporter::builder()
        .with_http()
        .build()?;

    let reader = PeriodicReader::builder(exporter).build();

    let provider = SdkMeterProvider::builder().with_reader(reader).build();

    opentelemetry::global::set_meter_provider(provider.clone());

    Ok(TelemetryGuard::from(Some(provider)))
}

/// Values accepted by `CLP_DISABLE_TELEMETRY` and `DO_NOT_TRACK` to disable telemetry.
///
/// NOTE: This must be kept consistent with the Python implementation in
/// `clp_package_utils/scripts/start_clp.py`.
const TELEMETRY_DISABLE_VALUES: [&str; 4] = ["1", "true", "yes", "y"];

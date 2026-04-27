use std::env;

use opentelemetry::KeyValue;
use opentelemetry_sdk::Resource;

/// Checks whether telemetry is disabled through any of the supported mechanisms.
fn is_telemetry_disabled() -> bool {
    // Check CLP_DISABLE_TELEMETRY env var
    if let Ok(val) = env::var("CLP_DISABLE_TELEMETRY")
        && (val.eq_ignore_ascii_case("true") || val == "1")
    {
        return true;
    }

    // Check DO_NOT_TRACK env var
    if let Ok(val) = env::var("DO_NOT_TRACK")
        && val == "1"
    {
        return true;
    }

    false
}

/// Initializes the global OpenTelemetry meter provider with the default OTLP exporter.
/// Should be called during application startup.
pub fn init_telemetry(service_name: &'static str) {
    if is_telemetry_disabled() {
        tracing::info!("Anonymous telemetry is disabled.");
        return;
    }

    tracing::info!("Anonymous telemetry is enabled. Set CLP_DISABLE_TELEMETRY=true to disable.");

    let provider = opentelemetry_otlp::new_pipeline()
        .metrics(opentelemetry_sdk::runtime::Tokio)
        .with_exporter(opentelemetry_otlp::new_exporter().tonic())
        .with_resource(Resource::new(vec![KeyValue::new(
            "service.name",
            service_name,
        )]))
        .build();

    match provider {
        Ok(p) => opentelemetry::global::set_meter_provider(p),
        Err(err) => tracing::warn!("Failed to build OTLP metrics pipeline: {}", err),
    }
}

use std::{env, time::Duration};

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
fn build_resource(service_name: &str) -> Resource {
    let telemetry_id = env::var("CLP_INSTANCE_ID").unwrap_or_else(|_| "unknown".to_owned());
    let clp_version = env::var("CLP_VERSION").unwrap_or_else(|_| "unknown".to_owned());
    let deployment_method =
        env::var("CLP_DEPLOYMENT_METHOD").unwrap_or_else(|_| "unknown".to_owned());
    let os = env::var("CLP_HOST_OS").unwrap_or_else(|_| std::env::consts::OS.to_owned());
    let os_version = env::var("CLP_HOST_OS_VERSION").unwrap_or_else(|_| "unknown".to_owned());
    let arch = env::var("CLP_HOST_ARCH").unwrap_or_else(|_| std::env::consts::ARCH.to_owned());
    let storage_engine = env::var("CLP_STORAGE_ENGINE").unwrap_or_else(|_| "clp".to_owned());

    Resource::new(vec![
        KeyValue::new("service.name", service_name.to_owned()),
        KeyValue::new("telemetry.id", telemetry_id),
        KeyValue::new("clp.version", clp_version),
        KeyValue::new("deployment.method", deployment_method),
        KeyValue::new("os.type", os),
        KeyValue::new("os.version", os_version),
        KeyValue::new("host.arch", arch),
        KeyValue::new("clp.storage_engine", storage_engine),
    ])
}

pub fn init_telemetry(service_name: &'static str) {
    if is_telemetry_disabled() {
        tracing::info!("Anonymous telemetry is disabled.");
        return;
    }

    tracing::info!("Anonymous telemetry is enabled. Set CLP_DISABLE_TELEMETRY=true to disable.");

    let provider = opentelemetry_otlp::new_pipeline()
        .metrics(opentelemetry_sdk::runtime::Tokio)
        .with_exporter(opentelemetry_otlp::new_exporter().tonic())
        .with_resource(build_resource(service_name))
        .build();

    match provider {
        Ok(p) => {
            opentelemetry::global::set_meter_provider(p);

            // Emit deployment_start counter exactly once on startup
            let meter = opentelemetry::global::meter("clp.api");
            let start_counter = meter.u64_counter("clp.api.deployment_start").init();
            start_counter.add(1, &[]);

            // Spawn heartbeat loop (every 24 hours)
            tokio::spawn(async move {
                // Initial wait for the first heartbeat
                let mut interval = tokio::time::interval(Duration::from_hours(24));
                // We don't want an immediate tick because deployment_start already signals the
                // start
                interval.tick().await;
                let heartbeat_counter = meter.u64_counter("clp.api.heartbeat").init();

                loop {
                    interval.tick().await;
                    heartbeat_counter.add(1, &[]);
                }
            });
        }
        Err(err) => tracing::warn!("Failed to build OTLP metrics pipeline: {}", err),
    }
}

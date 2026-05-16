use crate::clp_config::package::config::Telemetry;
use opentelemetry::global;
use opentelemetry_sdk::metrics::{PeriodicReader, SdkMeterProvider};
use opentelemetry_sdk::Resource;
use opentelemetry_otlp::{WithExportConfig, WithHttpConfig};
use std::env;

pub fn init_telemetry(telemetry_config: &Telemetry) -> Option<SdkMeterProvider> {
    if telemetry_config.disable || env::var("CLP_DISABLE_TELEMETRY").is_ok() {
        return None;
    }

    let endpoint = env::var("OTEL_EXPORTER_OTLP_ENDPOINT").unwrap_or_else(|_| telemetry_config.endpoint.clone());

    let exporter = opentelemetry_otlp::MetricExporter::builder()
        .with_http()
        .with_endpoint(endpoint)
        .build()
        .expect("Failed to build HTTP metric exporter");

    let reader = PeriodicReader::builder(exporter).build();
    let provider = SdkMeterProvider::builder()
        .with_reader(reader)
        .with_resource(Resource::default()) 
        .build();

    global::set_meter_provider(provider.clone());
    Some(provider)
}

pub fn shutdown_telemetry(provider: Option<SdkMeterProvider>) {
    if let Some(p) = provider {
        if let Err(err) = p.shutdown() {
            eprintln!("Error shutting down telemetry: {}", err);
        }
    }
}

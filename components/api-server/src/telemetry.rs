use std::{env, time::Duration};

use tokio_util::sync::CancellationToken;

use chrono::Utc;
use clp_rust_utils::clp_config::package::config::Config;
use serde::Serialize;

const TELEMETRY_ENDPOINT: &str = "https://telemetry.yscope.io/v1/events";
const TELEMETRY_SEND_INTERVAL: Duration = Duration::from_hours(24); // 24 hours
const TELEMETRY_HTTP_TIMEOUT: Duration = Duration::from_secs(5);

/// Schema version for the telemetry payload.
const SCHEMA_VERSION: u32 = 1;

#[derive(Debug, Serialize)]
struct TelemetryEvent {
    schema_version: u32,
    telemetry_id: String,
    timestamp: String,
    event_type: String,
    clp_version: String,
    deployment_method: String,
    os: String,
    os_version: String,
    arch: String,
    storage_engine: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    payload: Option<serde_json::Value>,
}

/// Checks whether telemetry is disabled through any of the supported mechanisms.
fn is_telemetry_disabled(config: &Config) -> bool {
    // Check config file setting
    if config.telemetry.disable {
        return true;
    }

    // Check CLP_DISABLE_TELEMETRY env var
    if let Ok(val) = env::var("CLP_DISABLE_TELEMETRY")
        && (val.eq_ignore_ascii_case("true") || val == "1")
    {
        return true;
    }

    // Check DO_NOT_TRACK env var (https://consoledonottrack.com/)
    if let Ok(val) = env::var("DO_NOT_TRACK")
        && val == "1"
    {
        return true;
    }

    false
}

const fn get_storage_engine_str(config: &Config) -> &'static str {
    match config.package.storage_engine {
        clp_rust_utils::clp_config::package::config::StorageEngine::Clp => "clp",
        clp_rust_utils::clp_config::package::config::StorageEngine::ClpS => "clp-s",
    }
}

fn build_event(event_type: &str, config: &Config) -> TelemetryEvent {
    let telemetry_id = env::var("CLP_INSTANCE_ID").unwrap_or_else(|_| "unknown".to_owned());
    let clp_version = env::var("CLP_VERSION").unwrap_or_else(|_| "unknown".to_owned());
    let deployment_method =
        env::var("CLP_DEPLOYMENT_METHOD").unwrap_or_else(|_| "unknown".to_owned());
    let os = env::var("CLP_HOST_OS").unwrap_or_else(|_| std::env::consts::OS.to_owned());
    let os_version = env::var("CLP_HOST_OS_VERSION").unwrap_or_else(|_| "unknown".to_owned());
    let arch = env::var("CLP_HOST_ARCH").unwrap_or_else(|_| std::env::consts::ARCH.to_owned());

    TelemetryEvent {
        schema_version: SCHEMA_VERSION,
        telemetry_id,
        timestamp: Utc::now().format("%Y-%m-%dT%H:%M:%SZ").to_string(),
        event_type: event_type.to_owned(),
        clp_version,
        deployment_method,
        os,
        os_version,
        arch,
        storage_engine: get_storage_engine_str(config).to_owned(),
        payload: None,
    }
}

async fn send_event(client: &reqwest::Client, event: &TelemetryEvent) {
    let is_debug =
        env::var("CLP_TELEMETRY_DEBUG").is_ok_and(|v| v.eq_ignore_ascii_case("true") || v == "1");

    if is_debug {
        match serde_json::to_string_pretty(event) {
            Ok(json) => tracing::info!("[telemetry-debug] Would send:\n{json}"),
            Err(e) => tracing::warn!("[telemetry-debug] Failed to serialize event: {e}"),
        }
        return;
    }

    match client.post(TELEMETRY_ENDPOINT).json(event).send().await {
        Ok(resp) => {
            tracing::debug!("Telemetry event sent, status: {}", resp.status());
        }
        Err(e) => {
            tracing::debug!("Failed to send telemetry event (this is not an error): {e}");
        }
    }
}

/// Runs the telemetry background loop. Sends a `deployment_start` event on startup
/// and a `heartbeat` event every 24 hours. All failures are silently ignored.
///
/// The loop respects the given [`CancellationToken`]: when the token is cancelled
/// the function returns promptly, allowing a clean shutdown.
///
/// This function is designed to be spawned as a background tokio task:
/// ```ignore
/// let cancel = CancellationToken::new();
/// tokio::spawn(telemetry::run_telemetry_loop(config, cancel.clone()));
/// // later, to stop:
/// cancel.cancel();
/// ```
pub async fn run_telemetry_loop(config: Config, cancel: CancellationToken) {
    if is_telemetry_disabled(&config) {
        tracing::info!("Anonymous telemetry is disabled.");
        return;
    }

    tracing::info!("Anonymous telemetry is enabled. Set CLP_DISABLE_TELEMETRY=true to disable.");

    let client = match reqwest::Client::builder()
        .timeout(TELEMETRY_HTTP_TIMEOUT)
        .build()
    {
        Ok(c) => c,
        Err(e) => {
            tracing::debug!("Failed to create telemetry HTTP client: {e}");
            return;
        }
    };

    // Send deployment_start event
    let start_event = build_event("deployment_start", &config);
    send_event(&client, &start_event).await;

    // Periodic heartbeat — exit when cancellation is requested
    loop {
        tokio::select! {
            () = cancel.cancelled() => {
                tracing::info!("Telemetry loop cancelled, shutting down.");
                break;
            }
            () = tokio::time::sleep(TELEMETRY_SEND_INTERVAL) => {
                let heartbeat_event = build_event("heartbeat", &config);
                send_event(&client, &heartbeat_event).await;
            }
        }
    }
}

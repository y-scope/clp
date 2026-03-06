use std::net::SocketAddr;
use std::sync::Arc;

use axum::extract::State;
use axum::http::StatusCode;
use axum::routing::post;
use axum::{Json, Router};
use serde::{Deserialize, Serialize};
use sqlx::PgPool;
use tower_http::limit::RequestBodyLimitLayer;

/// Maximum request body size (10 KB).
const MAX_BODY_SIZE: usize = 10 * 1024;

#[derive(Debug, Deserialize, Serialize)]
struct TelemetryEvent {
    schema_version: u32,
    telemetry_id: String,
    timestamp: String,
    event_type: String,
    #[serde(default)]
    clp_version: String,
    #[serde(default)]
    deployment_method: String,
    #[serde(default)]
    os: String,
    #[serde(default)]
    os_version: String,
    #[serde(default)]
    arch: String,
    #[serde(default)]
    storage_engine: String,
    #[serde(default)]
    payload: Option<serde_json::Value>,
}

struct AppState {
    db: PgPool,
}

async fn ingest_event(
    State(state): State<Arc<AppState>>,
    Json(event): Json<TelemetryEvent>,
) -> StatusCode {
    // Validate required fields
    if event.telemetry_id.is_empty() || event.event_type.is_empty() {
        return StatusCode::BAD_REQUEST;
    }

    // Validate telemetry_id is a plausible UUID (basic check)
    if event.telemetry_id.len() > 64 {
        return StatusCode::BAD_REQUEST;
    }

    let payload_json = event
        .payload
        .as_ref()
        .map(|p| serde_json::to_string(p).unwrap_or_default());

    let result = sqlx::query(
        r#"
        INSERT INTO telemetry_events (
            schema_version, telemetry_id, event_timestamp, event_type,
            clp_version, deployment_method, os, os_version, arch,
            storage_engine, payload
        )
        VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11)
        "#,
    )
    .bind(event.schema_version as i32)
    .bind(&event.telemetry_id)
    .bind(&event.timestamp)
    .bind(&event.event_type)
    .bind(&event.clp_version)
    .bind(&event.deployment_method)
    .bind(&event.os)
    .bind(&event.os_version)
    .bind(&event.arch)
    .bind(&event.storage_engine)
    .bind(payload_json)
    .execute(&state.db)
    .await;

    match result {
        Ok(_) => StatusCode::CREATED,
        Err(e) => {
            tracing::error!("Failed to insert telemetry event: {e}");
            StatusCode::INTERNAL_SERVER_ERROR
        }
    }
}

async fn health() -> StatusCode {
    StatusCode::OK
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    tracing_subscriber::fmt()
        .with_env_filter(
            tracing_subscriber::EnvFilter::try_from_default_env()
                .unwrap_or_else(|_| "info".into()),
        )
        .init();

    let database_url =
        std::env::var("DATABASE_URL").expect("DATABASE_URL environment variable must be set");

    let db = PgPool::connect(&database_url).await?;

    // Run migrations on startup
    sqlx::query(
        r#"
        CREATE TABLE IF NOT EXISTS telemetry_events (
            id BIGSERIAL PRIMARY KEY,
            schema_version INTEGER NOT NULL,
            telemetry_id TEXT NOT NULL,
            event_timestamp TEXT NOT NULL,
            event_type TEXT NOT NULL,
            clp_version TEXT NOT NULL DEFAULT '',
            deployment_method TEXT NOT NULL DEFAULT '',
            os TEXT NOT NULL DEFAULT '',
            os_version TEXT NOT NULL DEFAULT '',
            arch TEXT NOT NULL DEFAULT '',
            storage_engine TEXT NOT NULL DEFAULT '',
            payload TEXT,
            received_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
        );

        CREATE INDEX IF NOT EXISTS idx_telemetry_events_telemetry_id
            ON telemetry_events (telemetry_id);
        CREATE INDEX IF NOT EXISTS idx_telemetry_events_event_type
            ON telemetry_events (event_type);
        CREATE INDEX IF NOT EXISTS idx_telemetry_events_received_at
            ON telemetry_events (received_at);
        "#,
    )
    .execute(&db)
    .await?;

    let state = Arc::new(AppState { db });

    let app = Router::new()
        .route("/v1/events", post(ingest_event))
        .route("/health", axum::routing::get(health))
        .layer(RequestBodyLimitLayer::new(MAX_BODY_SIZE))
        .with_state(state);

    let addr: SocketAddr = "0.0.0.0:8080".parse()?;
    tracing::info!("Telemetry server listening on {addr}");

    let listener = tokio::net::TcpListener::bind(&addr).await?;
    axum::serve(listener, app).await?;

    Ok(())
}

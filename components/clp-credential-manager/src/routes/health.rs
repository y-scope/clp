use axum::{Router, http::StatusCode, routing::get};

use super::AppState;

/// Provides the `/health` endpoint used by readiness checks.
pub fn router() -> Router<AppState> {
    Router::new().route("/health", get(health_check))
}

/// Responds with `200 OK` to signal that the service is alive.
async fn health_check() -> StatusCode {
    StatusCode::OK
}

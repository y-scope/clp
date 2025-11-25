use axum::{Router, http::StatusCode, routing::get};

use super::AppState;

/// Provides the `/health` endpoint used by readiness checks.
///
/// # Returns:
///
/// A [`Router`] that mounts `/health` and delegates to [`health_check`].
pub fn router() -> Router<AppState> {
    Router::new().route("/health", get(health_check))
}

/// Responds with `200 OK` to signal that the service is alive.
///
/// # Returns:
///
/// [`StatusCode::OK`] regardless of request parameters.
async fn health_check() -> StatusCode {
    StatusCode::OK
}

use axum::{Router, http::StatusCode, routing::get};

use super::AppState;

pub fn router() -> Router<AppState> {
    Router::new().route("/health", get(health_check))
}

async fn health_check() -> StatusCode {
    StatusCode::OK
}

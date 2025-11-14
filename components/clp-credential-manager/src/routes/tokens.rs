use axum::{Json, Router, http::StatusCode, routing::post};
use serde::Serialize;

use super::AppState;

pub fn router() -> Router<AppState> {
    Router::new()
        .route("/credentials/issue-token", post(not_implemented))
        .route("/credentials/exchange-token", post(not_implemented))
}

#[derive(Serialize)]
struct PlaceholderResponse {
    message: &'static str,
}

async fn not_implemented() -> (StatusCode, Json<PlaceholderResponse>) {
    (
        StatusCode::NOT_IMPLEMENTED,
        Json(PlaceholderResponse {
            message: "token endpoints are not implemented yet",
        }),
    )
}

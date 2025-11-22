use axum::{Json, Router, http::StatusCode, routing::post};
use serde::Serialize;

use super::AppState;

/// Temporary placeholder router until token issuance/exchange is implemented.
pub fn router() -> Router<AppState> {
    Router::new()
        .route("/credentials/issue-token", post(not_implemented))
        .route("/credentials/exchange-token", post(not_implemented))
}

#[derive(Serialize)]
struct PlaceholderResponse {
    message: &'static str,
}

/// Returns a 501 response indicating the endpoint is still under construction.
async fn not_implemented() -> (StatusCode, Json<PlaceholderResponse>) {
    (
        StatusCode::NOT_IMPLEMENTED,
        Json(PlaceholderResponse {
            message: "token endpoints are not implemented yet",
        }),
    )
}

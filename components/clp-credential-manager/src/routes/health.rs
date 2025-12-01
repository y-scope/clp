use axum::http::StatusCode;

/// Responds with `200 OK` to signal that the service is alive.
///
/// # Returns:
///
/// [`StatusCode::OK`] regardless of request parameters.
pub async fn health_check() -> StatusCode {
    StatusCode::OK
}

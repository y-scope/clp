mod health;

use axum::{Router, routing::get};
use tower_http::trace::TraceLayer;

use crate::service::CredentialManagerServiceState;

/// Composes the service's HTTP surface, layering tracing instrumentation globally.
///
/// # Returns
///
/// An [`axum::Router`] containing the health endpoint and tracing middleware.
pub fn build_router() -> Router<CredentialManagerServiceState> {
    Router::new()
        .route("/health", get(health::health))
        .layer(TraceLayer::new_for_http())
}

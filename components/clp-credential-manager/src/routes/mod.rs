mod health;

use axum::{Router, routing::get};
use tower_http::trace::TraceLayer;

use crate::service::SharedService;

/// Shared Axum state for every route module.
pub type AppState = SharedService;

/// Composes the service's HTTP surface, layering tracing instrumentation globally.
///
/// # Returns:
///
/// An [`axum::Router`] wired with all route trees and middleware layers.
pub fn build_router() -> Router<AppState> {
    Router::new()
        .route("/health", get(health::health_check))
        .layer(TraceLayer::new_for_http())
}

mod health;

use axum::Router;
use tower_http::trace::TraceLayer;

use crate::service::SharedService;

/// Shared Axum state for every route module.
pub type AppState = SharedService;

/// Composes the service's HTTP surface, layering tracing instrumentation globally.
pub fn build_router() -> Router<AppState> {
    Router::new()
        .merge(health::router())
        .layer(TraceLayer::new_for_http())
}

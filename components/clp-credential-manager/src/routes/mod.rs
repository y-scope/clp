mod credentials;
mod health;
mod tokens;

use axum::Router;
use tower_http::trace::TraceLayer;

use crate::service::SharedService;

pub type AppState = SharedService;

pub fn build_router() -> Router<AppState> {
    Router::new()
        .merge(health::router())
        .merge(credentials::router())
        .merge(tokens::router())
        .layer(TraceLayer::new_for_http())
}

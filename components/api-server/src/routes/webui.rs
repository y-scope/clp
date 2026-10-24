//! Routes that back the `WebUI`.

mod metadata;

use utoipa_axum::router::OpenApiRouter;

use super::AppState;
use crate::error::ClientError;

/// Factory function.
///
/// # Returns
///
/// A newly created [`OpenApiRouter`] with all `WebUI` routes registered.
pub(super) fn router() -> OpenApiRouter<AppState> {
    OpenApiRouter::new().merge(metadata::router())
}

/// Records unexpected failures without logging request payloads or stored configuration.
fn log_unexpected_error(operation: &str, error: &ClientError) {
    let kind = match error {
        ClientError::Sql(_) => "sql",
        ClientError::Mongo(_) => "mongodb",
        ClientError::Io(_) => "filesystem",
        ClientError::Aws { .. } => "aws",
        ClientError::MalformedData => "malformed_data",
        ClientError::TaskJoin(_) => "background_task",
        _ => return,
    };
    tracing::error!(operation, kind, "WebUI API request failed.");
}

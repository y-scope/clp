//! Routes that back the `WebUI`.

mod metadata;

use utoipa_axum::router::OpenApiRouter;

use crate::error::ClientError;
use crate::routes::AppState;

/// Factory function.
///
/// # Returns
///
/// A newly created [`OpenApiRouter`] with all `WebUI` routes registered.
pub(super) fn router() -> OpenApiRouter<AppState> {
    OpenApiRouter::new().merge(metadata::router())
}

/// Logs `error` as a failure of `operation`, unless it converts to a non-500
/// [`HandlerError`](crate::routes::HandlerError) (i.e., the request itself is invalid).
///
/// The variants that aren't logged must be kept in sync with that conversion.
fn log_unexpected_error(operation: &str, error: &ClientError) {
    match error {
        ClientError::QueryJobNotFound(_)
        | ClientError::DatasetNotFound(_)
        | ClientError::QueryJobAlreadyTerminated(_)
        | ClientError::InvalidInput(_) => {}
        ClientError::Sql(_)
        | ClientError::Mongo(_)
        | ClientError::QueryNotSucceeded
        | ClientError::Io(_)
        | ClientError::MalformedData
        | ClientError::TaskJoin(_)
        | ClientError::Aws { .. }
        | ClientError::InvalidQueryJobConfig(_)
        | ClientError::Telemetry(_) => {
            tracing::error!(operation, error = ? error, "WebUI API request failed.");
        }
    }
}

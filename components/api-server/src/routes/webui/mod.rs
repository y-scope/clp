//! Routes that back the `WebUI`.

mod compression;
mod logs_input;
mod metadata;
mod stream_files;

use clp_rust_utils::logs_input::LogsInputError;
use utoipa_axum::router::OpenApiRouter;

use crate::error::ClientError;
use crate::routes::AppState;

/// Factory function.
///
/// # Returns
///
/// A newly created [`OpenApiRouter`] with all `WebUI` routes registered.
pub(super) fn router() -> OpenApiRouter<AppState> {
    OpenApiRouter::new()
        .merge(compression::router())
        .merge(logs_input::router())
        .merge(metadata::router())
        .merge(stream_files::router())
}

/// Logs `error` as a failure of `operation`, unless it converts to a 4xx
/// [`HandlerError`](crate::routes::HandlerError) (i.e., the request itself is invalid).
///
/// The variants that aren't logged must be kept in sync with that conversion.
fn log_unexpected_error(operation: &str, error: &ClientError) {
    match error {
        ClientError::QueryJobNotFound(_)
        | ClientError::DatasetNotFound(_)
        | ClientError::QueryJobAlreadyTerminated(_)
        | ClientError::QueryResultsNotStored(_)
        | ClientError::InvalidInput(_)
        | ClientError::LogEventNotFound { .. }
        | ClientError::LogsInput(
            LogsInputError::NotFound(_) | LogsInputError::OutsideMountPoint(_),
        ) => {}
        ClientError::Sql(_)
        | ClientError::Mongo(_)
        | ClientError::QueryNotSucceeded
        | ClientError::Io(_)
        | ClientError::MalformedData
        | ClientError::TaskJoin(_)
        | ClientError::Aws { .. }
        | ClientError::StreamExtractionTimeout(_)
        | ClientError::ExtractionJobFailed { .. }
        | ClientError::ExtractionJobNotFound(_)
        | ClientError::LogsInput(
            LogsInputError::MountPointUnavailable(_)
            | LogsInputError::RelativeContainerPath(_)
            | LogsInputError::UnsupportedName(_)
            | LogsInputError::Unresolvable { .. },
        )
        | ClientError::CompressionJobIdOutOfRange(_)
        | ClientError::Telemetry(_) => {
            tracing::error!(operation, error = ? error, "WebUI API request failed.");
        }
    }
}

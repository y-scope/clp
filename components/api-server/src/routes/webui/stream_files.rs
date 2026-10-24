//! Routes that extract stream files for the log viewer.

use axum::Json;
use axum::extract::State;
use utoipa_axum::router::OpenApiRouter;
use utoipa_axum::routes;

use super::log_unexpected_error;
use crate::routes::AppState;
use crate::routes::HandlerError;
use crate::webui_client::StreamFileExtraction;
use crate::webui_client::StreamFileMetadata;
use crate::webui_client::WebuiClient;

/// Factory function.
///
/// # Returns
///
/// A newly created [`OpenApiRouter`] with all stream-file routes registered.
pub(super) fn router() -> OpenApiRouter<AppState> {
    OpenApiRouter::new().routes(routes!(extract_stream_file))
}

/// The `OpenAPI` document of the stream-file routes.
#[derive(utoipa::OpenApi)]
#[openapi(
    paths(extract_stream_file),
    components(schemas(StreamFileExtraction, StreamFileMetadata))
)]
pub(super) struct ApiDoc;

#[utoipa::path(
    post,
    path = "/stream_files/extract",
    description = "Extracts a stream file containing the log event at the given index in the \
        stream with the given ID. If the stream has already been extracted, returns its \
        metadata directly; otherwise submits an extraction job and waits for it to complete.",
    request_body(content = StreamFileExtraction),
    responses(
        (status = OK, body = StreamFileMetadata),
        (status = BAD_REQUEST, description = "Invalid dataset name or extract job type"),
        (
            status = GATEWAY_TIMEOUT,
            body = String,
            content_type = "text/plain",
            description = "Stream extraction timed out"
        ),
        (status = INTERNAL_SERVER_ERROR)
    )
)]
async fn extract_stream_file(
    State(client): State<WebuiClient>,
    Json(extraction): Json<StreamFileExtraction>,
) -> Result<Json<StreamFileMetadata>, HandlerError> {
    let metadata = client
        .extract_stream_file(extraction)
        .await
        .inspect_err(|err| log_unexpected_error("extract_stream_file", err))?;
    Ok(Json(metadata))
}

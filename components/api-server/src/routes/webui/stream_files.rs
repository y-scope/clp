//! Routes that extract stream files for the `WebUI`'s log viewer.

use axum::Json;
use axum::extract::State;
use utoipa_axum::router::OpenApiRouter;
use utoipa_axum::routes;

use crate::routes::AppState;
use crate::routes::HandlerError;
use crate::routes::webui::log_unexpected_error;
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

#[utoipa::path(
    post,
    path = "/stream_files/extract",
    description = "Extracts a stream file that contains the log event at `log_event_idx` in the \
        stream with ID `stream_id`. If the results cache already has such a stream file, returns \
        its metadata directly; otherwise submits an extraction job, waits for it to finish, and \
        returns the metadata of the stream file it extracted. The wait is bounded by \
        `api_server.stream_file_extraction_timeout_secs`; the job keeps running when the timeout \
        is reached.",
    request_body(content = StreamFileExtraction),
    responses(
        (status = OK, body = StreamFileMetadata),
        (
            status = BAD_REQUEST,
            description = "The request body isn't valid JSON, or `extract_job_type` isn't a \
                string; `stream_id` is empty; `log_event_idx` is negative; `stream_id` isn't an \
                archive ID for JSON extraction; or the extraction job failed, was killed, was \
                cancelled, or finished without extracting a stream file that contains the log \
                event"
        ),
        (
            status = NOT_FOUND,
            description = "The extraction job disappeared from the query jobs table before it \
                finished"
        ),
        (
            status = UNSUPPORTED_MEDIA_TYPE,
            description = "The request's `Content-Type` isn't `application/json`"
        ),
        (
            status = UNPROCESSABLE_ENTITY,
            description = "The request body doesn't match the schema, including when \
                `extract_job_type` isn't `ExtractIr` or `ExtractJson`, `dataset` isn't a valid \
                dataset name, or the body has an unknown field"
        ),
        (status = INTERNAL_SERVER_ERROR),
        (
            status = GATEWAY_TIMEOUT,
            description = "The extraction job didn't finish within \
                `api_server.stream_file_extraction_timeout_secs`"
        )
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

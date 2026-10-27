//! Routes that submit compression jobs from the `WebUI`'s compression form.

use axum::Json;
use axum::extract::State;
use axum::http::StatusCode;
use utoipa_axum::router::OpenApiRouter;
use utoipa_axum::routes;

use crate::routes::AppState;
use crate::routes::HandlerError;
use crate::routes::webui::log_unexpected_error;
use crate::webui_client::CompressionJob;
use crate::webui_client::CompressionJobCreation;
use crate::webui_client::WebuiClient;

/// Factory function.
///
/// # Returns
///
/// A newly created [`OpenApiRouter`] with all compression routes registered.
pub(super) fn router() -> OpenApiRouter<AppState> {
    OpenApiRouter::new().routes(routes!(submit_compression_job))
}

#[utoipa::path(
    post,
    path = "/compression/jobs",
    description = "Submits a compression job for `paths` in the logs input, into `dataset`. The \
        paths are relative to the logs-input mount point, with a leading `/` that denotes the \
        mount point itself. The job compresses each path at its lexically normalized location \
        under the mount point (a symlink is compressed through its own path, not its target's), \
        and records the paths relative to the mount point. Only the clp-s storage engine with \
        filesystem logs input is supported.",
    request_body(content = CompressionJobCreation),
    responses(
        (status = CREATED, body = CompressionJob, description = "The submitted compression job"),
        (
            status = BAD_REQUEST,
            description = "The request body isn't valid JSON; the package's storage engine isn't \
                clp-s or its logs input isn't on the filesystem; `paths` is empty; or a path \
                resolves to outside the logs-input mount point through a symlink"
        ),
        (
            status = NOT_FOUND,
            description = "A path doesn't exist, or has a component that is too long"
        ),
        (
            status = UNSUPPORTED_MEDIA_TYPE,
            description = "The request's `Content-Type` isn't `application/json`"
        ),
        (
            status = UNPROCESSABLE_ENTITY,
            description = "The request body doesn't match the schema, including when a field has \
                the wrong JSON type, `paths` is missing, a path doesn't start with `/`, has a `..` \
                component, or contains a NUL character, `dataset` isn't a valid dataset name, \
                `timestamp_key` is empty, or the body has an unknown field"
        ),
        (
            status = INTERNAL_SERVER_ERROR,
            description = "The logs-input mount point can't be resolved, a path can't be resolved \
                (e.g., it's a symlink loop, or due to a permission error), the database request \
                failed, or the inserted job's ID is out of range"
        )
    )
)]
async fn submit_compression_job(
    State(client): State<WebuiClient>,
    Json(creation): Json<CompressionJobCreation>,
) -> Result<(StatusCode, Json<CompressionJob>), HandlerError> {
    let job = client
        .submit_compression_job(creation)
        .await
        .inspect_err(|err| log_unexpected_error("submit_compression_job", err))?;
    Ok((StatusCode::CREATED, Json(job)))
}

use axum::Json;
use axum::extract::Path;
use axum::extract::Query;
use axum::extract::State;
use axum::http::StatusCode;
use serde::Deserialize;
use utoipa::IntoParams;
use utoipa_axum::router::OpenApiRouter;
use utoipa_axum::routes;

use super::AppState;
use super::HandlerError;
use crate::webui_client::CompressionJob;
use crate::webui_client::CompressionJobCreation;
use crate::webui_client::CompressionMetadata;
use crate::webui_client::DirEntry;
use crate::webui_client::IngestionDetails;
use crate::webui_client::QuerySpeed;
use crate::webui_client::SpaceSavings;
use crate::webui_client::StreamFileExtraction;
use crate::webui_client::StreamFileMetadata;
use crate::webui_client::TimeRange;

/// Factory method to create an [`OpenApiRouter`] configured with all metadata,
/// compression, file-listing, and stream-file routes.
///
/// # Returns
///
/// A newly created [`OpenApiRouter`] instance with the routes registered.
pub(super) fn router() -> OpenApiRouter<AppState> {
    OpenApiRouter::default()
        .routes(routes!(datasets))
        .routes(routes!(time_range))
        .routes(routes!(space_savings))
        .routes(routes!(ingestion_details))
        .routes(routes!(query_speed))
        .routes(routes!(timestamp_column_names))
        .routes(routes!(compression_metadata))
        .routes(routes!(list_files))
        .routes(routes!(compression_job))
        .routes(routes!(extract_stream_file))
}

/// Query parameters for the dataset-scoped metadata endpoints.
#[derive(Deserialize, IntoParams)]
#[into_params(parameter_in = Query)]
struct DatasetsParams {
    /// Comma-separated list of dataset names (CLP-S only). Ignored for the CLP storage engine.
    #[serde(default)]
    dataset: Option<String>,
}

/// Parses a comma-separated `dataset` query parameter into a list of dataset names.
///
/// # Returns
///
/// The trimmed, non-empty dataset names; empty when the parameter is absent.
fn parse_datasets(dataset: Option<String>) -> Vec<String> {
    dataset
        .map(|s| {
            s.split(',')
                .map(str::trim)
                .filter(|t| !t.is_empty())
                .map(str::to_owned)
                .collect()
        })
        .unwrap_or_default()
}

#[utoipa::path(
    get,
    path = "/metadata/datasets",
    description = "Gets the names of all datasets.",
    responses(
        (status = OK, body = Vec<String>),
        (status = INTERNAL_SERVER_ERROR)
    )
)]
async fn datasets(State(state): State<AppState>) -> Result<Json<Vec<String>>, HandlerError> {
    Ok(Json(state.webui_client.get_dataset_names().await?))
}

#[utoipa::path(
    get,
    path = "/metadata/time_range",
    description = "Gets the earliest and latest log entry timestamps across the given \
        datasets. For the CLP storage engine, the `dataset` parameter is ignored. Returns \
        `null` when no archives exist yet.",
    params(DatasetsParams),
    responses(
        (status = OK, body = Option<TimeRange>),
        (status = BAD_REQUEST, description = "Invalid dataset name"),
        (status = INTERNAL_SERVER_ERROR)
    )
)]
async fn time_range(
    State(state): State<AppState>,
    Query(params): Query<DatasetsParams>,
) -> Result<Json<Option<TimeRange>>, HandlerError> {
    let datasets = parse_datasets(params.dataset);
    Ok(Json(state.webui_client.get_time_range(&datasets).await?))
}

#[utoipa::path(
    get,
    path = "/metadata/space_savings",
    description = "Gets aggregated space-savings statistics (total uncompressed and \
        compressed sizes) across all datasets.",
    responses(
        (status = OK, body = SpaceSavings),
        (status = INTERNAL_SERVER_ERROR)
    )
)]
async fn space_savings(State(state): State<AppState>) -> Result<Json<SpaceSavings>, HandlerError> {
    Ok(Json(state.webui_client.get_space_savings().await?))
}

#[utoipa::path(
    get,
    path = "/metadata/ingestion_details",
    description = "Gets ingestion details (timestamp range, file count, message count) \
        across all datasets. Returns `null` when no data has been ingested yet.",
    responses(
        (status = OK, body = Option<IngestionDetails>),
        (status = INTERNAL_SERVER_ERROR)
    )
)]
async fn ingestion_details(
    State(state): State<AppState>,
) -> Result<Json<Option<IngestionDetails>>, HandlerError> {
    Ok(Json(state.webui_client.get_ingestion_details().await?))
}

/// Query parameters for the query speed endpoint.
#[derive(Deserialize, IntoParams)]
#[into_params(parameter_in = Query)]
struct QuerySpeedParams {
    /// Comma-separated list of dataset names (CLP-S only). Ignored for the CLP storage engine.
    #[serde(default)]
    dataset: Option<String>,
    /// The search job ID whose scan speed should be computed.
    search_job_id: i64,
}

#[utoipa::path(
    get,
    path = "/metadata/query_speed",
    description = "Gets the query speed (total uncompressed bytes scanned and job duration) \
        for a search job across the given datasets. Returns `null` when the job hasn't \
        scanned any archives or hasn't finished yet.",
    params(QuerySpeedParams),
    responses(
        (status = OK, body = Option<QuerySpeed>),
        (status = BAD_REQUEST, description = "Invalid dataset name"),
        (status = INTERNAL_SERVER_ERROR)
    )
)]
async fn query_speed(
    State(state): State<AppState>,
    Query(params): Query<QuerySpeedParams>,
) -> Result<Json<Option<QuerySpeed>>, HandlerError> {
    let datasets = parse_datasets(params.dataset);
    Ok(Json(
        state
            .webui_client
            .get_query_speed(&datasets, params.search_job_id)
            .await?,
    ))
}

#[utoipa::path(
    get,
    path = "/metadata/column_metadata/{dataset_name}/timestamp",
    description = "Gets the timestamp column names for a given dataset (CLP-S only).",
    responses(
        (status = OK, body = Vec<String>),
        (status = BAD_REQUEST, description = "Invalid dataset name"),
        (status = NOT_FOUND, description = "Dataset not found"),
        (status = INTERNAL_SERVER_ERROR)
    )
)]
async fn timestamp_column_names(
    State(state): State<AppState>,
    Path(dataset_name): Path<String>,
) -> Result<Json<Vec<String>>, HandlerError> {
    Ok(Json(
        state
            .webui_client
            .get_timestamp_column_names(&dataset_name)
            .await?,
    ))
}

#[utoipa::path(
    get,
    path = "/metadata/compression_jobs",
    description = "Gets recent compression-job metadata (most recent first), with the \
        decoded CLP IO config for each job.",
    responses(
        (status = OK, body = Vec<CompressionMetadata>),
        (status = INTERNAL_SERVER_ERROR)
    )
)]
async fn compression_metadata(
    State(state): State<AppState>,
) -> Result<Json<Vec<CompressionMetadata>>, HandlerError> {
    Ok(Json(
        state.webui_client.get_compression_metadata().await?,
    ))
}

/// Query parameters for the file-listing endpoint.
#[derive(Deserialize, IntoParams)]
#[into_params(parameter_in = Query)]
struct ListFilesParams {
    /// The absolute filesystem path to list.
    path: String,
}

#[utoipa::path(
    get,
    path = "/os/ls",
    description = "Lists files and directories at the specified path.",
    params(ListFilesParams),
    responses(
        (status = OK, body = Vec<DirEntry>),
        (status = NOT_FOUND, description = "Path not found"),
        (status = INTERNAL_SERVER_ERROR)
    )
)]
async fn list_files(
    State(state): State<AppState>,
    Query(params): Query<ListFilesParams>,
) -> Result<Json<Vec<DirEntry>>, HandlerError> {
    Ok(Json(state.webui_client.list_files(params.path).await?))
}

#[utoipa::path(
    post,
    path = "/compression/jobs",
    description = "Submits a compression job.",
    request_body(content = CompressionJobCreation),
    responses(
        (status = CREATED, body = CompressionJob, description = "The created compression job."),
        (status = BAD_REQUEST, description = "Invalid dataset name"),
        (status = INTERNAL_SERVER_ERROR)
    )
)]
async fn compression_job(
    State(state): State<AppState>,
    Json(creation): Json<CompressionJobCreation>,
) -> Result<(StatusCode, Json<CompressionJob>), HandlerError> {
    let job = state
        .webui_client
        .submit_compression_job(creation)
        .await?;
    Ok((StatusCode::CREATED, Json(job)))
}

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
        (status = INTERNAL_SERVER_ERROR)
    )
)]
async fn extract_stream_file(
    State(state): State<AppState>,
    Json(extraction): Json<StreamFileExtraction>,
) -> Result<Json<StreamFileMetadata>, HandlerError> {
    Ok(Json(
        state
            .webui_client
            .extract_stream_file(extraction)
            .await?,
    ))
}

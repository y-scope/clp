#![allow(clippy::needless_for_each)]
use std::str::FromStr;

use axum::{
    Json,
    Router,
    extract::{Path, State},
    response::IntoResponse,
    routing::get,
};
use clp_rust_utils::job_config::ingestion::s3::{S3ScannerConfig, SqsListenerConfig};
use serde::Serialize;
use tower_http::cors::{Any, CorsLayer};
use utoipa::{OpenApi, ToSchema};
use utoipa_axum::{router::OpenApiRouter, routes};

use crate::ingestion_job_manager::{Error as IngestionJobManagerError, IngestionJobManagerState};

#[derive(utoipa::OpenApi)]
#[openapi(
    info(
        title = "log-ingestor",
        description = "log-ingestor for CLP",
        contact(name = "YScope")
    ),
    tags(
        (name = "Health", description = "Health check endpoint"),
        (name = "IngestionJob", description = "Ingestion job orchestration endpoints")
    ),
    paths(
        health,
        create_s3_scanner_job,
        create_sqs_listener_job,
        stop_and_delete_job
    )
)]
pub struct ApiDoc;

/// Factory method to create an Axum router configured with all log ingestor routes.
///
/// # Returns
///
/// A newly created router instance configured without setting the state.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`OpenApi::to_json`]'s return values on failure.
pub fn create_router() -> Result<Router<IngestionJobManagerState>, serde_json::Error> {
    let (router, api) = OpenApiRouter::with_openapi(ApiDoc::openapi())
        .route("/", get(health))
        .routes(routes!(health))
        .routes(routes!(create_s3_scanner_job))
        .routes(routes!(create_sqs_listener_job))
        .routes(routes!(stop_and_delete_job))
        .split_for_parts();

    let api_json = api.to_json()?;
    let router = router
        .route(
            "/openapi.json",
            get(|| async { (axum::http::StatusCode::OK, api_json) }),
        )
        .layer(CorsLayer::new().allow_origin(Any));
    Ok(router)
}

#[derive(thiserror::Error, Debug)]
enum Error {
    #[error("{0}")]
    IngestionJobManagerError(#[from] IngestionJobManagerError),

    #[error("Invalid job ID: {0}")]
    InvalidJobId(String),
}

impl IntoResponse for Error {
    fn into_response(self) -> axum::response::Response {
        let (status_code, error_message) = match &self {
            Self::IngestionJobManagerError(e) => match e {
                IngestionJobManagerError::InternalError(_) => (
                    axum::http::StatusCode::INTERNAL_SERVER_ERROR,
                    "Internal server error".to_string(),
                ),
                IngestionJobManagerError::JobNotFound(_) => {
                    (axum::http::StatusCode::NOT_FOUND, self.to_string())
                }
                IngestionJobManagerError::PrefixConflict(_) => {
                    (axum::http::StatusCode::CONFLICT, self.to_string())
                }
                IngestionJobManagerError::CustomEndpointUrlNotSupported(_)
                | IngestionJobManagerError::InvalidConfig(_)
                | IngestionJobManagerError::MissingRegionCode => {
                    (axum::http::StatusCode::BAD_REQUEST, self.to_string())
                }
            },
            Self::InvalidJobId(_) => (axum::http::StatusCode::BAD_REQUEST, self.to_string()),
        };
        let body = ErrorResponse {
            error: error_message,
        };
        (status_code, Json(body)).into_response()
    }
}

#[derive(Clone, Serialize, ToSchema)]
struct CreationResponse {
    /// The unique ID of the created ingestion job.
    id: String,
}

#[derive(Clone, Serialize, ToSchema)]
struct ErrorResponse {
    /// The error message.
    error: String,
}

#[utoipa::path(
    get,
    path = "/health",
    tags = ["Health"],
    responses((status = OK, body = String))
)]
async fn health() -> &'static str {
    "log-ingestor is running"
}

#[utoipa::path(
    post,
    path = "/s3_scanner",
    tags = ["IngestionJob"],
    description = "Creates an ingestion job that periodically scans the specified S3 bucket and \
        key prefix for new objects to ingest.\n\n\
        To ensure correct and efficient ingestion, the scanner relies on the following \
        assumptions:\n\n\
        1. Lexicographical Order: New objects are added in lexicographical order to the bucket \
        based on their keys. For example, objects with keys `log1` and `log2` will be ingested \
        sequentially. If a new object with key `log0` is added after `log2`, it will be ignored \
        because it is not lexicographically greater than the last ingested key.\n\n\
        2. Immutability: Objects under the specified prefix are immutable. Once an object is \
        created, it is not modified or overwritten.",
    responses(
        (status = OK, body = CreationResponse, description = "The ID of the created job."),
        (
            status = CONFLICT,
            body = ErrorResponse,
            description = "A prefix conflict was detected. For the same bucket and dataset, only \
                one ingestion job may monitor a given S3 prefix or any of its ancestor or \
                descendant prefixes."
        ),
        (
            status = INTERNAL_SERVER_ERROR,
            body = ErrorResponse,
            description = "Internal server failure."
        ),
        (
            status = BAD_REQUEST,
            body = ErrorResponse,
            description = "A region code is not provided when using the default AWS S3 endpoint."
        )
    )
)]
async fn create_s3_scanner_job(
    State(ingestion_job_manager_state): State<IngestionJobManagerState>,
    Json(config): Json<S3ScannerConfig>,
) -> Result<Json<CreationResponse>, Error> {
    tracing::info!(config = ? config, "Create S3 scanner ingestion job.");
    let job_id = ingestion_job_manager_state
        .create_s3_scanner_job(config)
        .await
        .map_err(|err| {
            tracing::error!(err = ? err, "Failed to create S3 scanner ingestion job.");
            Error::IngestionJobManagerError(err)
        })?;
    tracing::info!(job_id = ? job_id, "Created S3 scanner ingestion job.");
    Ok(Json(CreationResponse {
        id: job_id.to_string(),
    }))
}

#[utoipa::path(
    post,
    path = "/sqs_listener",
    tags = ["IngestionJob"],
    description = "Creates an ingestion job that monitors an SQS queue. The queue receives \
        notifications whenever new objects are added to the specified S3 bucket and key prefix.\n\n\
        The specified SQS queue must be dedicated to this ingestion job. Upon successful \
        ingestion, the job deletes the corresponding message from the queue to ensure objects are \
        not ingested multiple times.\n\n\
        To maintain correctness and avoid backpressure, the job may also delete messages that are \
        irrelevant to this ingestion job (for example, messages referring to objects outside the \
        configured bucket or key prefix).",
    responses(
        (status = OK, body = CreationResponse, description = "The ID of the created job."),
        (
            status = CONFLICT,
            body = ErrorResponse,
            description = "A prefix conflict was detected. For the same bucket and dataset, only \
                one ingestion job may monitor a given S3 prefix or any of its ancestor or \
                descendant prefixes."
        ),
        (
            status = INTERNAL_SERVER_ERROR,
            body = ErrorResponse,
            description = "Internal server failure."
        ),
        (
            status = BAD_REQUEST,
            body = ErrorResponse,
            description = "Custom endpoint URLs are not supported for SQS listener jobs, or the \
                specified number of concurrent listener tasks is invalid."
        )
    )
)]
async fn create_sqs_listener_job(
    State(ingestion_job_manager_state): State<IngestionJobManagerState>,
    Json(config): Json<SqsListenerConfig>,
) -> Result<Json<CreationResponse>, Error> {
    tracing::info!(config = ? config, "Create SQS listener ingestion job.");
    let job_id = ingestion_job_manager_state
        .create_sqs_listener_job(config)
        .await
        .map_err(|err| {
            tracing::error!(err = ? err, "Failed to create SQS listener ingestion job.");
            Error::IngestionJobManagerError(err)
        })?;
    tracing::info!(job_id = ? job_id, "Created SQS listener ingestion job.");
    Ok(Json(CreationResponse {
        id: job_id.to_string(),
    }))
}

#[utoipa::path(
    delete,
    path = "/job/{job_id}",
    tags = ["IngestionJob"],
    description = "Deletes an existing ingestion job by its ID. This operation stops the job if it \
        is currently running and removes all associated resources.",
    params(
        (
            "job_id" = String,
            Path,
            description = "The unique identifier of the ingestion job to delete."
        )
    ),
    responses(
        (status = OK, body = ()),
        (
            status = NOT_FOUND,
            body = ErrorResponse,
            description = "The specified job ID does not correspond to any existing ingestion job."
        ),
        (
            status = BAD_REQUEST,
            body = ErrorResponse,
            description = "The provided job ID is not in a valid format."
        ),
        (
            status = INTERNAL_SERVER_ERROR,
            body = ErrorResponse,
            description = "Internal server failure."
        )
    )
)]
async fn stop_and_delete_job(
    State(ingestion_job_manager_state): State<IngestionJobManagerState>,
    Path(job_id): Path<String>,
) -> Result<(), Error> {
    tracing::info!(job_id = ? job_id, "Stop and delete ingestion job.");
    let job_id = uuid::Uuid::from_str(&job_id).map_err(|err| {
        tracing::error!(err = ? err, "Invalid job ID format.");
        Error::InvalidJobId(err.to_string())
    })?;
    ingestion_job_manager_state
        .shutdown_and_remove_job(job_id)
        .await
        .map_err(|err| {
            tracing::error!(err = ? err, "Failed to stop and delete ingestion job.");
            Error::IngestionJobManagerError(err)
        })?;
    tracing::info!(job_id = ? job_id, "The ingestion job has been deleted.");
    Ok(())
}

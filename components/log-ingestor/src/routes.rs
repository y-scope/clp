use std::str::FromStr;

use axum::{
    Json,
    Router,
    extract::{Path, State},
    response::IntoResponse,
    routing::{delete, get, post},
};
use clp_rust_utils::job_config::ingestion::s3::{S3ScannerConfig, SqsListenerConfig};
use serde::Serialize;

use crate::ingestion_job_manager::{Error as IngestionJobManagerError, IngestionJobManagerState};

/// Factory method to create an Axum router configured with all log ingestor routes.
///
/// # Returns
///
/// A newly created router instance configured without setting the state.
pub fn create_router() -> Router<IngestionJobManagerState> {
    Router::new()
        .route("/", get(health))
        .route("/health", get(health))
        .route("/s3_scanner", post(create_s3_scanner_job))
        .route("/sqs_listener", post(create_sqs_listener_job))
        .route("/job/{job_id}", delete(stop_and_delete_job))
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
            },
            Self::InvalidJobId(_) => (axum::http::StatusCode::BAD_REQUEST, self.to_string()),
        };
        let body = serde_json::json!({
            "error": error_message
        });
        (status_code, Json(body)).into_response()
    }
}

#[derive(Clone, Serialize)]
struct CreationResponse {
    /// The unique ID of the created ingestion job.
    id: String,
}

async fn health() -> &'static str {
    "log-ingestor is running"
}

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

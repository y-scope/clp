use std::str::FromStr;

use axum::{
    Json,
    Router,
    extract::{Path, State},
    response::IntoResponse,
    routing::{delete, get, post},
};
use clp_rust_utils::job_config::ingestion::s3::{S3ScannerConfig, SqsListenerConfig};
use serde::{Deserialize, Serialize};

use crate::ingestion_job_manager::{Error as IngestionJobManagerError, IngestionJobManagerState};

/// Factory method to create an Axum router configured with all log ingestor routes.
///
/// # Returns
///
/// A newly created router instance configured without setting the state.
pub fn create_routes() -> Router<IngestionJobManagerState> {
    Router::new()
        .route("/", get(health))
        .route("/create_s3_scanner_job", post(create_s3_scanner_job))
        .route("/create_sqs_listener_job", post(create_sqs_listener_job))
        .route("/stop_and_delete_job/{job_id}", delete(stop_and_delete_job))
}

#[derive(thiserror::Error, Debug)]
enum Error {
    #[error("{0}")]
    IngestionJobManagerError(#[from] IngestionJobManagerError),

    #[error("Invalid job ID: {0}")]
    InvalidJobId(String),
}

#[derive(Clone, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
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
    let job_id = match ingestion_job_manager_state
        .create_s3_scanner_job(config)
        .await
    {
        Ok(job_id) => {
            tracing::info!(job_id = ? job_id, "Created S3 scanner ingestion job.");
            job_id
        }
        Err(err) => {
            tracing::error!(err = ? err, "Failed to create S3 scanner ingestion job.");
            return Err(Error::IngestionJobManagerError(err));
        }
    };
    Ok(Json(CreationResponse {
        id: job_id.to_string(),
    }))
}

async fn create_sqs_listener_job(
    State(ingestion_job_manager_state): State<IngestionJobManagerState>,
    Json(config): Json<SqsListenerConfig>,
) -> Result<Json<CreationResponse>, Error> {
    tracing::info!(config = ? config, "Create SQS listener ingestion job.");
    let job_id = match ingestion_job_manager_state
        .create_sqs_listener_job(config)
        .await
    {
        Ok(job_id) => {
            tracing::info!(job_id = ? job_id, "Created SQS listener ingestion job.");
            job_id
        }
        Err(err) => {
            tracing::error!(err = ? err, "Failed to create SQS listener ingestion job.");
            return Err(Error::IngestionJobManagerError(err));
        }
    };
    Ok(Json(CreationResponse {
        id: job_id.to_string(),
    }))
}

async fn stop_and_delete_job(
    State(ingestion_job_manager_state): State<IngestionJobManagerState>,
    Path(job_id): Path<String>,
) -> Result<(), Error> {
    tracing::info!(job_id = ? job_id, "Stop and delete ingestion job.");
    let job_id = match uuid::Uuid::from_str(&job_id) {
        Ok(id) => id,
        Err(err) => {
            tracing::error!(err = ? err, "Invalid job ID format.");
            return Err(Error::InvalidJobId(err.to_string()));
        }
    };
    match ingestion_job_manager_state
        .shutdown_and_remove_job(job_id)
        .await
    {
        Ok(()) => {
            tracing::info!(job_id = ? job_id, "Stopped and deleted ingestion job.");
            Ok(())
        }
        Err(err) => {
            tracing::error!(err = ? err, "Failed to stop and delete ingestion job.");
            Err(Error::IngestionJobManagerError(err))
        }
    }
}

impl IntoResponse for Error {
    fn into_response(self) -> axum::response::Response {
        let status_code = match &self {
            Self::IngestionJobManagerError(e) => match e {
                IngestionJobManagerError::InternalError(_) => {
                    axum::http::StatusCode::INTERNAL_SERVER_ERROR
                }
                IngestionJobManagerError::JobNotFound(_) => axum::http::StatusCode::NOT_FOUND,
                IngestionJobManagerError::PrefixConflict(_) => axum::http::StatusCode::CONFLICT,
            },
            Self::InvalidJobId(_) => axum::http::StatusCode::BAD_REQUEST,
        };
        let body = serde_json::json!({
            "error": self.to_string()
        });
        (status_code, Json(body)).into_response()
    }
}

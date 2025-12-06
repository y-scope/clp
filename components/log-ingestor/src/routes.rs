use axum::{
    Json,
    Router,
    extract::State,
    response::IntoResponse,
    routing::{get, post},
};
use clp_rust_utils::job_config::ingestion::s3::{S3ScannerConfig, SqsListenerConfig};
use serde::{Deserialize, Serialize};

use crate::ingestion_job_manager::{Error, IngestionJobManagerState};

/// Factory method to create an Axum router configured with all log ingestor routes.
pub fn create_routes() -> Router<IngestionJobManagerState> {
    Router::new()
        .route("/", get(health))
        .route("/create_s3_scanner_job", post(create_s3_scanner_job))
        .route("/create_sqs_listener_job", post(create_sqs_listener_job))
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
            return Err(err);
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
            return Err(err);
        }
    };
    Ok(Json(CreationResponse {
        id: job_id.to_string(),
    }))
}

impl IntoResponse for Error {
    fn into_response(self) -> axum::response::Response {
        let status_code = match &self {
            Self::InternalError(_) => axum::http::StatusCode::INTERNAL_SERVER_ERROR,
            Self::JobNotFound(_) => axum::http::StatusCode::NOT_FOUND,
            Self::PrefixConflict(_) => axum::http::StatusCode::CONFLICT,
        };
        let body = serde_json::json!({
            "error": self.to_string()
        });
        (status_code, Json(body)).into_response()
    }
}

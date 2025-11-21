use axum::{
    Json,
    Router,
    extract::{Path, State},
    http::StatusCode,
    routing::get,
};
use tracing::instrument;

use super::AppState;
use crate::{
    error::ApiError,
    models::{CreateCredentialRequest, CredentialMetadata},
};

/// Registers CRUD endpoints for long-term credentials.
pub fn router() -> Router<AppState> {
    Router::new()
        .route(
            "/credentials",
            get(list_credentials).post(create_credential),
        )
        .route(
            "/credentials/:id",
            get(get_credential).delete(delete_credential),
        )
}

/// Lists every stored credential, returning metadata only.
#[instrument(skip_all)]
async fn list_credentials(
    State(service): State<AppState>,
) -> Result<Json<Vec<CredentialMetadata>>, ApiError> {
    let credentials = service.list_credentials().await.map_err(ApiError::from)?;
    Ok(Json(credentials))
}

/// Creates a new credential from the request payload.
#[instrument(skip_all)]
async fn create_credential(
    State(service): State<AppState>,
    Json(request): Json<CreateCredentialRequest>,
) -> Result<Json<CredentialMetadata>, ApiError> {
    let created = service
        .create_credential(request)
        .await
        .map_err(ApiError::from)?;
    Ok(Json(created))
}

/// Fetches a credential by identifier.
#[instrument(skip_all)]
async fn get_credential(
    State(service): State<AppState>,
    Path(id): Path<i64>,
) -> Result<Json<CredentialMetadata>, ApiError> {
    let credential = service.get_credential(id).await.map_err(ApiError::from)?;
    Ok(Json(credential))
}

/// Deletes a credential and returns `204 No Content` on success.
#[instrument(skip_all)]
async fn delete_credential(
    State(service): State<AppState>,
    Path(id): Path<i64>,
) -> Result<StatusCode, ApiError> {
    service
        .delete_credential(id)
        .await
        .map_err(ApiError::from)?;
    Ok(StatusCode::NO_CONTENT)
}

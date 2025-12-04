#![allow(dead_code)]

use axum::{
    Json,
    http::StatusCode,
    response::{IntoResponse, Response},
};
use clp_rust_utils::Error as UtilsError;
use serde::Serialize;
use thiserror::Error;

/// Canonical error enumeration for the credential manager.
#[derive(Debug, Error)]
pub enum ServiceError {
    #[error("configuration error: {0}")]
    Config(String),

    #[error("configuration file error: {0}")]
    ConfigFile(#[from] UtilsError),

    #[error("validation error: {0}")]
    Validation(String),

    #[error("`sqlx::Error`: {0}")]
    Database(#[source] sqlx::Error),

    #[error("resource conflict: {0}")]
    Conflict(String),

    #[error("resource not found: {0}")]
    NotFound(String),

    #[error("`std::io::Error`: {0}")]
    Io(#[from] std::io::Error),

    #[error("`serde_yaml::Error`: {0}")]
    Yaml(#[from] serde_yaml::Error),
}

impl From<sqlx::Error> for ServiceError {
    /// Maps raw [`sqlx::Error`] values into domain-specific variants so callers can react
    /// precisely.
    ///
    /// # Parameters:
    ///
    /// * `err`: The database error surfaced by `sqlx`.
    ///
    /// # Returns
    ///
    /// A [`ServiceError`] variant that best describes the provided database error.
    fn from(err: sqlx::Error) -> Self {
        if matches!(err, sqlx::Error::RowNotFound) {
            return Self::NotFound("requested record was not found".to_owned());
        }

        if let Some(db_err) = err.as_database_error()
            && let Some(mysql_err) = db_err.try_downcast_ref::<sqlx::mysql::MySqlDatabaseError>()
            && mysql_err.number() == 1062
        {
            return Self::Conflict(mysql_err.message().to_owned());
        }

        Self::Database(err)
    }
}

/// JSON payload emitted by error responses.
#[derive(Debug, Serialize)]
struct ErrorBody {
    error: String,
}

impl ServiceError {
    const fn status_code(&self) -> StatusCode {
        match self {
            Self::Validation(_) => StatusCode::BAD_REQUEST,
            Self::Conflict(_) => StatusCode::CONFLICT,
            Self::NotFound(_) => StatusCode::NOT_FOUND,
            Self::Config(_)
            | Self::ConfigFile(_)
            | Self::Database(_)
            | Self::Io(_)
            | Self::Yaml(_) => StatusCode::INTERNAL_SERVER_ERROR,
        }
    }
}

impl IntoResponse for ServiceError {
    /// Serializes the error payload into JSON schema so clients receive consistent bodies.
    ///
    /// # Returns
    ///
    /// An [`axum::response::Response`] containing the status code plus `{"error": "..."}` body.
    fn into_response(self) -> Response {
        let status = self.status_code();
        let body = Json(ErrorBody {
            error: self.to_string(),
        });

        (status, body).into_response()
    }
}

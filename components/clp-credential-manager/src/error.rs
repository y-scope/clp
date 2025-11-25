#![allow(dead_code)]

use std::fmt;

use axum::{
    Json,
    http::StatusCode,
    response::{IntoResponse, Response},
};
use jsonwebtoken::errors::Error as JwtError;
use serde::Serialize;
use thiserror::Error;

/// Convenience alias for functions that return a [`ServiceError`].
pub type ServiceResult<T> = Result<T, ServiceError>;

/// Canonical error enumeration for the credential manager.
#[derive(Debug, Error)]
pub enum ServiceError {
    #[error("configuration error: {0}")]
    Config(String),

    #[error("validation error: {0}")]
    Validation(String),

    #[error("database error: {0}")]
    Database(#[source] sqlx::Error),

    #[error("resource conflict: {0}")]
    Conflict(String),

    #[error("resource not found: {0}")]
    NotFound(String),

    #[error("jwt error: {0}")]
    Jwt(#[source] JwtError),

    #[error("i/o error: {0}")]
    Io(#[from] std::io::Error),

    #[error("serialization error: {0}")]
    Yaml(#[from] serde_yaml::Error),
}

impl From<sqlx::Error> for ServiceError {
    /// Maps raw [`sqlx::Error`] values into domain-specific variants so callers can react precisely.
    ///
    /// # Parameters:
    ///
    /// * `err`: The database error surfaced by `sqlx`.
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

/// HTTP-friendly representation of service failures.
#[derive(Debug)]
pub struct ApiError {
    status: StatusCode,
    message: String,
}

impl ApiError {
    /// Creates a new API error with the supplied HTTP status code.
    ///
    /// # Parameters:
    ///
    /// * `status`: HTTP status code that best represents the failure.
    /// * `message`: Human-readable diagnostic message.
    ///
    /// # Returns:
    ///
    /// A new [`ApiError`] ready to be converted into an HTTP response.
    pub fn new(status: StatusCode, message: impl Into<String>) -> Self {
        Self {
            status,
            message: message.into(),
        }
    }

    /// Convenience constructor for 500-class responses.
    ///
    /// # Parameters:
    ///
    /// * `message`: Human-readable error string for the response body.
    ///
    /// # Returns:
    ///
    /// A new [`ApiError`] that always maps to [`StatusCode::INTERNAL_SERVER_ERROR`].
    pub fn internal(message: impl Into<String>) -> Self {
        Self::new(StatusCode::INTERNAL_SERVER_ERROR, message)
    }
}

impl From<ServiceError> for ApiError {
    fn from(err: ServiceError) -> Self {
        match err {
            ServiceError::Validation(msg) => Self::new(StatusCode::BAD_REQUEST, msg),
            ServiceError::Conflict(msg) => Self::new(StatusCode::CONFLICT, msg),
            ServiceError::NotFound(msg) => Self::new(StatusCode::NOT_FOUND, msg),
            ServiceError::Config(msg) => Self::new(StatusCode::INTERNAL_SERVER_ERROR, msg),
            ServiceError::Database(source) => {
                Self::new(StatusCode::INTERNAL_SERVER_ERROR, source.to_string())
            }
            ServiceError::Jwt(source) => {
                Self::new(StatusCode::INTERNAL_SERVER_ERROR, source.to_string())
            }
            ServiceError::Io(source) => {
                Self::new(StatusCode::INTERNAL_SERVER_ERROR, source.to_string())
            }
            ServiceError::Yaml(source) => {
                Self::new(StatusCode::INTERNAL_SERVER_ERROR, source.to_string())
            }
        }
    }
}

/// JSON payload emitted by error responses.
#[derive(Debug, Serialize)]
struct ErrorBody {
    error: String,
}

impl IntoResponse for ApiError {
    /// Serializes the error payload into JSON schema so clients receive consistent bodies.
    ///
    /// # Returns:
    ///
    /// An [`axum::response::Response`] containing the status code plus `{"error": "..."}` body.
    fn into_response(self) -> Response {
        let status = self.status;
        let body = Json(ErrorBody {
            error: self.message,
        });

        (status, body).into_response()
    }
}

impl fmt::Display for ApiError {
    /// Formats only the message so higher layers can log without leaking sensitive details.
    ///
    /// # Returns:
    ///
    /// [`fmt::Result`] signaling whether writing to the formatter succeeded.
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.message)
    }
}

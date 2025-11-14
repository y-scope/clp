use std::fmt;

use axum::{
    Json,
    http::StatusCode,
    response::{IntoResponse, Response},
};
use jsonwebtoken::errors::Error as JwtError;
use serde::Serialize;
use thiserror::Error;

pub type ServiceResult<T> = Result<T, ServiceError>;

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
    fn from(err: sqlx::Error) -> Self {
        if matches!(err, sqlx::Error::RowNotFound) {
            return ServiceError::NotFound("requested record was not found".to_owned());
        }

        if let Some(db_err) = err.as_database_error() {
            if let Some(mysql_err) = db_err.try_downcast_ref::<sqlx::mysql::MySqlDatabaseError>() {
                if mysql_err.number() == 1062 {
                    return ServiceError::Conflict(mysql_err.message().to_owned());
                }
            }
        }

        ServiceError::Database(err)
    }
}

#[derive(Debug)]
pub struct ApiError {
    status: StatusCode,
    message: String,
}

impl ApiError {
    pub fn new(status: StatusCode, message: impl Into<String>) -> Self {
        Self {
            status,
            message: message.into(),
        }
    }

    pub fn internal(message: impl Into<String>) -> Self {
        Self::new(StatusCode::INTERNAL_SERVER_ERROR, message)
    }
}

impl From<ServiceError> for ApiError {
    fn from(err: ServiceError) -> Self {
        match err {
            ServiceError::Validation(msg) => ApiError::new(StatusCode::BAD_REQUEST, msg),
            ServiceError::Conflict(msg) => ApiError::new(StatusCode::CONFLICT, msg),
            ServiceError::NotFound(msg) => ApiError::new(StatusCode::NOT_FOUND, msg),
            ServiceError::Config(msg) => ApiError::new(StatusCode::INTERNAL_SERVER_ERROR, msg),
            ServiceError::Database(source) => {
                ApiError::new(StatusCode::INTERNAL_SERVER_ERROR, source.to_string())
            }
            ServiceError::Jwt(source) => {
                ApiError::new(StatusCode::INTERNAL_SERVER_ERROR, source.to_string())
            }
            ServiceError::Io(source) => {
                ApiError::new(StatusCode::INTERNAL_SERVER_ERROR, source.to_string())
            }
            ServiceError::Yaml(source) => {
                ApiError::new(StatusCode::INTERNAL_SERVER_ERROR, source.to_string())
            }
        }
    }
}

#[derive(Debug, Serialize)]
struct ErrorBody {
    error: String,
}

impl IntoResponse for ApiError {
    fn into_response(self) -> Response {
        let status = self.status;
        let body = Json(ErrorBody {
            error: self.message,
        });

        (status, body).into_response()
    }
}

impl fmt::Display for ApiError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.message)
    }
}

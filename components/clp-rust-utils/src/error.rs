//! Error type for CLP Rust utilities.

use thiserror::Error;

#[derive(Error, Debug)]
pub enum Error {
    #[error("`rmp_serde::encode::Error`: {0}")]
    MsgpackEncode(#[from] rmp_serde::encode::Error),

    #[error("`std::io::Error`: {0}")]
    Io(#[from] std::io::Error),

    #[error("`libzstd_rs_sys`: {0}")]
    Zstd(String),

    #[error("`yaml_serde::Error`: {0}")]
    SerdeYaml(#[from] yaml_serde::Error),

    #[error("`sqlx::Error`: {0}")]
    Sqlx(#[from] sqlx::Error),

    #[error(transparent)]
    TelemetryExporterBuildError(#[from] opentelemetry_otlp::ExporterBuildError),
}

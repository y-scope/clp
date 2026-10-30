//! Error types for the API server.

use aws_sdk_s3::error::DisplayErrorContext;
use aws_sdk_s3::error::SdkError;
use aws_sdk_s3::primitives::ByteStreamError;
use clp_rust_utils::dataset::DatasetName;
use clp_rust_utils::job_config::QueryJobId;
use clp_rust_utils::job_config::QueryJobStatus;
use clp_rust_utils::logs_input::LogsInputError;
use clp_rust_utils::types::ArchiveId;
use num_enum::TryFromPrimitive;
use thiserror::Error;

#[derive(Error, Debug)]
pub enum ClientError {
    #[error("`sqlx::Error`: {0}")]
    Sql(#[from] sqlx::Error),

    #[error("`mongodb::error::Error`: {0}")]
    Mongo(#[from] mongodb::error::Error),

    #[error("Query is not succeeded")]
    QueryNotSucceeded,

    #[error("`std::io::Error`: {0}")]
    Io(#[from] std::io::Error),

    #[error("Malformed data")]
    MalformedData,

    #[error("background task failed: {0}")]
    TaskJoin(#[from] tokio::task::JoinError),

    #[error("`AwsError`: {description}")]
    Aws { description: String },

    #[error("query job not found: {0}")]
    QueryJobNotFound(QueryJobId),

    #[error("query job already terminated: {0}")]
    QueryJobAlreadyTerminated(QueryJobId),

    #[error("Invalid input: {0}")]
    InvalidInput(String),

    #[error("stream extraction timed out after {0} seconds")]
    StreamExtractionTimeout(u64),

    #[error("extraction job {job_id} ended with status {status:?}")]
    ExtractionJobFailed {
        job_id: QueryJobId,
        status: QueryJobStatus,
    },

    #[error("extraction job {0} not found in the query jobs table")]
    ExtractionJobNotFound(QueryJobId),

    #[error("log event {log_event_idx} not found in the stream files of archive {archive_id}")]
    LogEventNotFound {
        archive_id: ArchiveId,
        log_event_idx: u64,
    },

    #[error("Dataset not found: {0}")]
    DatasetNotFound(DatasetName),

    #[error(transparent)]
    LogsInput(#[from] LogsInputError),

    #[error("compression job ID {0} is out of range")]
    CompressionJobIdOutOfRange(u64),

    #[error("invalid query job config: {0}")]
    InvalidQueryJobConfig(String),

    #[error(transparent)]
    Telemetry(#[from] opentelemetry_otlp::ExporterBuildError),
}

/// Empty trait to mark errors that indicate malformed data.
trait IsMalformedData {}

impl IsMalformedData for serde_json::Error {}

impl IsMalformedData for rmp_serde::encode::Error {}

impl IsMalformedData for rmp_serde::decode::Error {}

impl<Enum: TryFromPrimitive> IsMalformedData for num_enum::TryFromPrimitiveError<Enum> {}

impl<T: IsMalformedData> From<T> for ClientError {
    fn from(_value: T) -> Self {
        Self::MalformedData
    }
}

impl<AwsSdkErrorType: std::error::Error + 'static> From<SdkError<AwsSdkErrorType>> for ClientError {
    fn from(value: SdkError<AwsSdkErrorType>) -> Self {
        Self::Aws {
            description: DisplayErrorContext(&value).to_string(),
        }
    }
}

impl From<ByteStreamError> for ClientError {
    fn from(value: ByteStreamError) -> Self {
        Self::Aws {
            description: value.to_string(),
        }
    }
}

impl From<clp_rust_utils::Error> for ClientError {
    fn from(value: clp_rust_utils::Error) -> Self {
        match value {
            clp_rust_utils::Error::MsgpackEncode(_)
            | clp_rust_utils::Error::MsgpackDecode(_)
            | clp_rust_utils::Error::SerdeYaml(_)
            | clp_rust_utils::Error::Zstd(_) => Self::MalformedData,
            clp_rust_utils::Error::UnsupportedS3Endpoint(_) => {
                Self::InvalidInput(value.to_string())
            }
            clp_rust_utils::Error::Io(error) => error.into(),
            clp_rust_utils::Error::Sqlx(error) => error.into(),
            clp_rust_utils::Error::TelemetryExporterBuildError(error) => Self::Telemetry(error),
        }
    }
}

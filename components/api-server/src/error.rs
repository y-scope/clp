use aws_sdk_s3::{error::SdkError, primitives::ByteStreamError};
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

    #[error("`AwsError`: {description}")]
    Aws { description: String },

    #[error("api_server configuration is missing")]
    ConfigIsNone,

    #[error("Search job not found: {0}")]
    SearchJobNotFound(u64),
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

impl<AwsSdkErrorType> From<SdkError<AwsSdkErrorType>> for ClientError {
    fn from(value: SdkError<AwsSdkErrorType>) -> Self {
        Self::Aws {
            description: value.to_string(),
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
            clp_rust_utils::Error::MsgpackEncode(_) | clp_rust_utils::Error::SerdeYaml(_) => {
                Self::MalformedData
            }
            clp_rust_utils::Error::Io(error) => error.into(),
            clp_rust_utils::Error::Sqlx(error) => error.into(),
        }
    }
}

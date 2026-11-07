//! Error types for the query result listener.

use clp_rust_utils::types::ArchiveId;
use clp_rust_utils::types::ParseArchiveIdError;
use clp_rust_utils::types::ParseSessionTokenError;
use spider_core::task::TaskIndex;

/// Violations of the wire protocol.
#[derive(Debug, thiserror::Error)]
pub enum ProtocolError {
    #[error(
        "task {task_index} is bound to archive {expected}, but the connection streams archive \
         {received}"
    )]
    ArchiveIdMismatch {
        task_index: TaskIndex,
        expected: ArchiveId,
        received: ArchiveId,
    },

    #[error("bytes followed the trailer")]
    DataAfterTrailer,

    #[error(transparent)]
    InvalidArchiveId(#[from] ParseArchiveIdError),

    #[error(transparent)]
    InvalidSessionToken(#[from] ParseSessionTokenError),

    #[error("malformed frame: {0}")]
    MalformedFrame(String),

    #[error(
        "the trailer counts {trailer_num_results} results, but the connection streamed \
         {num_results}"
    )]
    ResultCountMismatch {
        trailer_num_results: u64,
        num_results: u64,
    },

    #[error("expected result index {expected} on the connection, got {received}")]
    UnexpectedResultIndex { expected: u64, received: u64 },

    #[error("unsupported protocol version {0}")]
    UnsupportedVersion(u64),
}

/// Reasons a connection from a query task ends without an acknowledgement.
#[derive(Debug, thiserror::Error)]
pub enum ConnectionError {
    #[error("I/O error: {0}")]
    Io(#[from] std::io::Error),

    #[error("the connection closed without a trailer")]
    MissingTrailer,

    #[error(transparent)]
    Protocol(#[from] ProtocolError),

    #[error("the connection closed in the middle of a frame")]
    TruncatedFrame,
}

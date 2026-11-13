//! Error types for the query result listener.

use std::time::Duration;

use clp_rust_utils::types::ArchiveId;
use clp_rust_utils::types::ParseArchiveIdError;
use clp_rust_utils::types::ParseSessionTokenError;
use clp_rust_utils::types::SessionToken;
use spider_core::task::TaskIndex;
use tokio::task::JoinError;

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
    #[error("the connection's results can't be encoded: {0}")]
    Encoding(String),

    #[error("no handshake arrived within {0:?}")]
    HandshakeTimeout(Duration),

    #[error("I/O error: {0}")]
    Io(#[from] std::io::Error),

    #[error("the connection closed without a trailer")]
    MissingTrailer,

    #[error(transparent)]
    Protocol(#[from] ProtocolError),

    #[error("the connection's session has ended")]
    SessionEnded,

    #[error("the connection's session has failed")]
    SessionFailed,

    #[error("the connection closed in the middle of a frame")]
    TruncatedFrame,

    #[error("unknown session token {0}")]
    UnknownSession(SessionToken),
}

/// Reasons a session can't be opened.
#[derive(Debug, thiserror::Error)]
pub enum SessionError {
    #[error("session token {0} is already registered")]
    TokenCollision(SessionToken),
}

/// Reasons the events of a session's query job end in a failure.
///
/// # Type Parameters
///
/// * `TerminationErrorType` - The error of a failure to wait for the job's termination.
#[derive(Debug, thiserror::Error)]
pub enum SessionEventError<TerminationErrorType> {
    #[error("the session's results can't be encoded: {0}")]
    Encoding(String),

    #[error("waiting for the query job's termination failed: {0}")]
    Termination(TerminationErrorType),

    #[error("the task waiting for the query job's termination failed: {0}")]
    TerminationPanicked(JoinError),
}

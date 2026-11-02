//! Error types for the query result listener.

use clp_rust_utils::types::ParseArchiveIdError;
use clp_rust_utils::types::ParseSessionTokenError;

/// Violations of the wire protocol.
#[derive(Debug, thiserror::Error)]
pub enum ProtocolError {
    #[error(transparent)]
    InvalidArchiveId(#[from] ParseArchiveIdError),

    #[error(transparent)]
    InvalidSessionToken(#[from] ParseSessionTokenError),

    #[error("malformed frame: {0}")]
    MalformedFrame(String),

    #[error("unsupported protocol version {0}")]
    UnsupportedVersion(u64),
}

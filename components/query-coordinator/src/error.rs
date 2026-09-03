//! The crate-level error type for the query coordinator.

/// Errors returned by the query coordinator.
#[derive(Debug, thiserror::Error)]
pub enum Error {
    #[error("spider request failure: {0}")]
    SpiderClient(#[from] spider_client::error::ClientError),
}

//! Receives the results that `clp-s` query tasks stream over TCP.

mod encoding;
mod error;
pub mod protocol;

pub use encoding::QueryResult;
pub use encoding::QueryResultEncoder;
pub use encoding::QueryResultEncoding;
pub use encoding::ResultEncoder;
pub use encoding::ResultEncoding;
pub use error::ProtocolError;

#[cfg(test)]
mod test_utils;

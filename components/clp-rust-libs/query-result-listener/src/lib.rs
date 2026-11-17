//! Receives the results that `clp-s` query tasks stream over TCP.

pub mod encoding;
mod error;
pub mod listener;
pub mod protocol;
pub mod session;

pub use error::ProtocolError;
pub use error::SessionError;

#[cfg(any(test, feature = "test-utils"))]
pub mod test_utils;

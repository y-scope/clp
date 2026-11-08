//! Receives the results that `clp-s` query tasks stream over TCP.

mod connection;
mod cursor;
pub mod encoding;
mod error;
pub mod listener;
pub mod protocol;
pub mod session;

pub use error::ProtocolError;
pub use error::SessionError;

#[cfg(test)]
mod test_utils;

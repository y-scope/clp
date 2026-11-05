//! Receives the results that `clp-s` query tasks stream over TCP.

pub mod encoding;
mod error;
pub mod protocol;

pub use error::ProtocolError;

#[cfg(test)]
mod test_utils;

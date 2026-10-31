//! Receives the results that `clp-s` query tasks stream over TCP.

mod error;
pub mod protocol;

pub use error::ProtocolError;

//! Receives the results that `clp-s` query tasks stream over TCP.

#[expect(
    dead_code,
    reason = "the TCP listener that runs the connection handler comes in a later change"
)]
mod connection;
#[expect(
    dead_code,
    reason = "the TCP listener that runs the connection handler comes in a later change"
)]
mod cursor;
pub mod encoding;
mod error;
pub mod protocol;
#[expect(
    dead_code,
    reason = "the TCP listener that runs the connection handler comes in a later change"
)]
mod session;

pub use error::ProtocolError;

#[cfg(test)]
mod test_utils;

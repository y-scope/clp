//! Receives the results that `clp-s` query tasks stream over TCP.

#[expect(
    dead_code,
    reason = "the connection handler will be added in a separate later change"
)]
mod cursor;
mod error;
pub mod protocol;

pub use error::ProtocolError;

#[cfg(test)]
mod test_utils;

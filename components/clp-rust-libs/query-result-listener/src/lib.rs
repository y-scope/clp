//! Receives the results that `clp-s` query tasks stream over TCP.

#[cfg_attr(
    not(test),
    expect(
        dead_code,
        reason = "the TCP listener that runs the connection handler comes in a later change"
    )
)]
mod connection;
#[cfg_attr(
    not(test),
    expect(
        dead_code,
        reason = "the TCP listener that runs the connection handler comes in a later change"
    )
)]
mod cursor;
pub mod encoding;
mod error;
pub mod protocol;
#[cfg_attr(
    not(test),
    expect(
        dead_code,
        reason = "the TCP listener that runs the connection handler comes in a later change"
    )
)]
mod session;

pub use error::ProtocolError;

#[cfg(test)]
mod test_utils;

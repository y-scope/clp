//! Receives the results that `clp-s` query tasks stream over TCP.

mod connection;
mod cursor;
pub mod encoding;
mod error;
mod listener;
pub mod protocol;
mod session;

pub use error::ProtocolError;
pub use listener::ListenerConfig;
pub use listener::ResultListener;
pub use session::ResultStream;
pub use session::Session;
pub use session::SessionConfig;

#[cfg(test)]
mod test_utils;

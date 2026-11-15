//! The configuration of a session.

use std::num::NonZeroUsize;

/// Configuration of a session.
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct SessionConfig {
    /// The number of batches buffered for the consumer before the session's connections stop
    /// reading from their sockets.
    pub channel_capacity: NonZeroUsize,
}

impl Default for SessionConfig {
    fn default() -> Self {
        Self {
            channel_capacity: NonZeroUsize::new(16).expect("16 is nonzero"),
        }
    }
}

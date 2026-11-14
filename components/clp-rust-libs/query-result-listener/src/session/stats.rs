//! Statistics of a session's connections.

use std::sync::atomic::AtomicU64;
use std::sync::atomic::Ordering;

/// Statistics of a session's connections.
#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub struct SessionStats {
    /// The number of results emitted to the consumer.
    pub num_results_emitted: u64,

    /// The number of results dropped because another attempt of their task had already emitted
    /// them.
    pub num_duplicates_dropped: u64,
}

/// The statistics of a session's connections, shared between the session's state and its result
/// stream.
#[derive(Default)]
pub(super) struct AtomicSessionStats {
    num_results_emitted: AtomicU64,
    num_duplicates_dropped: AtomicU64,
}

impl AtomicSessionStats {
    /// Adds `stats` to the statistics.
    pub fn add(&self, stats: &SessionStats) {
        self.num_results_emitted
            .fetch_add(stats.num_results_emitted, Ordering::Relaxed);
        self.num_duplicates_dropped
            .fetch_add(stats.num_duplicates_dropped, Ordering::Relaxed);
    }

    /// # Returns
    ///
    /// The statistics added so far.
    #[must_use]
    pub fn load(&self) -> SessionStats {
        SessionStats {
            num_results_emitted: self.num_results_emitted.load(Ordering::Relaxed),
            num_duplicates_dropped: self.num_duplicates_dropped.load(Ordering::Relaxed),
        }
    }
}

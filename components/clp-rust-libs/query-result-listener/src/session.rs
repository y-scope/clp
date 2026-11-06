//! Sessions, each of which collects the results streamed for one query job.

use std::sync::Arc;
use std::sync::atomic::AtomicU64;
use std::sync::atomic::Ordering;

use dashmap::DashMap;
use spider_core::task::TaskIndex;
use tokio::sync::mpsc;

use crate::cursor::TaskCursor;
use crate::encoding::ResultEncoder;

/// Statistics of a session's connections.
#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
#[expect(
    clippy::struct_field_names,
    reason = "`num_` marks a count throughout the crate, as in `Trailer::num_results`"
)]
pub struct SessionStats {
    /// The number of results emitted to the consumer.
    pub num_results_emitted: u64,

    /// The number of results dropped because another attempt of their task had already emitted
    /// them.
    pub num_duplicates_dropped: u64,

    /// The number of claimed results discarded because the consumer had dropped the receiver of
    /// the session's batches.
    pub num_results_discarded: u64,
}

/// The state of a session shared with its connections.
///
/// # Type Parameters
///
/// * `EncoderType` - The encoder of the results the session delivers.
pub struct State<EncoderType: ResultEncoder> {
    /// The cursors of the session's query tasks, keyed by task index.
    pub cursors: DashMap<TaskIndex, Arc<TaskCursor>>,

    /// The sender of the session's batches of encoded results.
    pub batches_sender: mpsc::Sender<EncoderType::Batch>,

    num_results_emitted: AtomicU64,
    num_duplicates_dropped: AtomicU64,
    num_results_discarded: AtomicU64,
}

impl<EncoderType: ResultEncoder> State<EncoderType> {
    /// Factory function.
    ///
    /// Creates the state of a session that sends its batches to `batches_sender`.
    ///
    /// # Returns
    ///
    /// The newly created state.
    #[must_use]
    pub fn new(batches_sender: mpsc::Sender<EncoderType::Batch>) -> Self {
        Self {
            cursors: DashMap::new(),
            batches_sender,
            num_results_emitted: AtomicU64::new(0),
            num_duplicates_dropped: AtomicU64::new(0),
            num_results_discarded: AtomicU64::new(0),
        }
    }

    /// Adds the statistics of a finished connection to the session's statistics.
    pub fn record(&self, stats: &SessionStats) {
        self.num_results_emitted
            .fetch_add(stats.num_results_emitted, Ordering::Relaxed);
        self.num_duplicates_dropped
            .fetch_add(stats.num_duplicates_dropped, Ordering::Relaxed);
        self.num_results_discarded
            .fetch_add(stats.num_results_discarded, Ordering::Relaxed);
    }

    /// # Returns
    ///
    /// The statistics recorded by the session's finished connections.
    #[must_use]
    pub fn stats(&self) -> SessionStats {
        SessionStats {
            num_results_emitted: self.num_results_emitted.load(Ordering::Relaxed),
            num_duplicates_dropped: self.num_duplicates_dropped.load(Ordering::Relaxed),
            num_results_discarded: self.num_results_discarded.load(Ordering::Relaxed),
        }
    }
}

//! The state of a session shared with its connections.

use std::sync::Arc;
use std::sync::OnceLock;

use clp_rust_utils::types::ArchiveId;
use dashmap::DashMap;
use spider_core::task::TaskIndex;
use tokio::sync::mpsc;
use tokio_util::sync::CancellationToken;

use crate::encoding::ResultEncoding;
use crate::error::ConnectionError;
use crate::error::ProtocolError;
use crate::session::SessionStats;
use crate::session::cursor::TaskCursor;
use crate::session::stats::AtomicSessionStats;

/// The state of a session shared with its connections.
///
/// # Type Parameters
///
/// * `EncodingType` - The encoding of the results the session delivers.
pub(super) struct State<EncodingType: ResultEncoding> {
    cursors: DashMap<TaskIndex, Arc<TaskCursor>>,
    batches_sender: mpsc::Sender<EncodingType::Batch>,
    pub(super) end_signal: CancellationToken,
    encoding_error: OnceLock<EncodingType::Error>,
    stats: Arc<AtomicSessionStats>,
}

impl<EncodingType: ResultEncoding> State<EncodingType> {
    /// Factory function.
    ///
    /// Creates the state of a session that sends its batches to `batches_sender`, and adds the
    /// statistics of its finished connections to `stats`.
    ///
    /// # Returns
    ///
    /// The newly created state.
    #[must_use]
    pub fn new(
        batches_sender: mpsc::Sender<EncodingType::Batch>,
        stats: Arc<AtomicSessionStats>,
    ) -> Self {
        Self {
            cursors: DashMap::new(),
            batches_sender,
            end_signal: CancellationToken::new(),
            encoding_error: OnceLock::new(),
            stats,
        }
    }

    /// Gets or creates the cursor of task `task_index`, checking that it is bound to `archive_id`.
    ///
    /// # Returns
    ///
    /// The cursor of task `task_index` on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ProtocolError::ArchiveIdMismatch`] if the cursor of task `task_index` is bound to
    ///   another archive.
    pub fn cursor(
        &self,
        task_index: TaskIndex,
        archive_id: ArchiveId,
    ) -> Result<Arc<TaskCursor>, ProtocolError> {
        let cursor = Arc::clone(
            self.cursors
                .entry(task_index)
                .or_insert_with(|| Arc::new(TaskCursor::new(archive_id)))
                .value(),
        );
        let expected = cursor.archive_id();
        if expected != archive_id {
            return Err(ProtocolError::ArchiveIdMismatch {
                task_index,
                expected,
                received: archive_id,
            });
        }
        Ok(cursor)
    }

    /// Waits until the session ends, either because it has failed or because it has been dropped.
    ///
    /// The session's connections stop reading once it ends, but still hand on the batches they have
    /// already claimed, as [`Session`](crate::session::registry::Session) explains.
    ///
    /// # Returns
    ///
    /// Why the session has ended:
    ///
    /// * [`SessionEnd::Failed`] if the session has failed.
    /// * [`SessionEnd::Dropped`] if the session has been dropped without failing.
    pub async fn ended(&self) -> SessionEnd {
        self.end_signal.cancelled().await;
        if self.encoding_error().is_some() {
            SessionEnd::Failed
        } else {
            SessionEnd::Dropped
        }
    }

    /// # Returns
    ///
    /// The error of the first failure to create the encoder of one of the session's connections or
    /// to encode one of its results, or `None` if the session hasn't failed.
    #[must_use]
    pub fn encoding_error(&self) -> Option<&EncodingType::Error> {
        self.encoding_error.get()
    }

    /// Fails the session with `error`, unless it has already failed, and signals its connections
    /// that it has ended.
    ///
    /// # Returns
    ///
    /// [`ConnectionError::Encoding`] carrying the message of `error`, which ends the connection
    /// whose encoder failed.
    #[must_use]
    pub fn fail(&self, error: EncodingType::Error) -> ConnectionError {
        let message = error.to_string();
        self.encoding_error.get_or_init(|| error);
        self.end_signal.cancel();
        ConnectionError::Encoding(message)
    }

    /// Hands `batch` to the session's consumer.
    ///
    /// # Returns
    ///
    /// Whether the consumer still holds the receiver of the session's batches, so that a connection
    /// can tell whether to count `batch` as emitted.
    pub async fn send(&self, batch: EncodingType::Batch) -> bool {
        self.batches_sender.send(batch).await.is_ok()
    }

    /// Adds the statistics of a finished connection to the session's statistics.
    pub fn record(&self, stats: &SessionStats) {
        self.stats.add(stats);
    }
}

/// Why a session has ended.
pub(super) enum SessionEnd {
    /// The session was dropped.
    Dropped,

    /// The session failed, as [`Session`](crate::session::registry::Session) defines it.
    Failed,
}

impl From<SessionEnd> for ConnectionError {
    fn from(end: SessionEnd) -> Self {
        match end {
            SessionEnd::Dropped => Self::SessionEnded,
            SessionEnd::Failed => Self::SessionFailed,
        }
    }
}

//! Sessions, each of which collects the results streamed for one query job.

use std::num::NonZeroU16;
use std::num::NonZeroUsize;
use std::pin::Pin;
use std::sync::Arc;
use std::sync::atomic::AtomicU64;
use std::sync::atomic::Ordering;
use std::task::Context;
use std::task::Poll;

use clp_rust_utils::job_config::NetworkOutput;
use clp_rust_utils::types::ArchiveId;
use clp_rust_utils::types::SessionToken;
use dashmap::DashMap;
use dashmap::Entry;
use futures::Stream;
use non_empty_string::NonEmptyString;
use spider_core::task::TaskIndex;
use tokio::sync::mpsc;
use tokio_util::sync::CancellationToken;

use crate::connection::Connection;
use crate::cursor::TaskCursor;
use crate::encoding::ResultEncoding;
use crate::error::ConnectionError;
use crate::error::ProtocolError;
use crate::error::SessionError;
use crate::protocol::Handshake;

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

/// A session that receives the results streamed for one query job.
///
/// The session accepts the connections that present its token until it is dropped. Dropping it also
/// stops its open connections from reading, which then end without an acknowledgement.
///
/// # Type Parameters
///
/// * `EncodingType` - The encoding of the results the session delivers.
pub struct Session<EncodingType: ResultEncoding> {
    _registration: Registration<EncodingType>,
    network_output: NetworkOutput,
}

impl<EncodingType: ResultEncoding> Session<EncodingType> {
    /// # Returns
    ///
    /// The network output that directs a query job's tasks to stream their results to this session.
    #[must_use]
    pub const fn network_output(&self) -> &NetworkOutput {
        &self.network_output
    }
}

/// The stream of a session's results, in the batches its connections encoded them into.
///
/// The stream ends once its session is dropped and every connection that joined the session has
/// finished, after which [`Self::stats`] reports the session's final statistics.
///
/// # Type Parameters
///
/// * `BatchType` - The type of a batch of encoded results.
pub struct ResultStream<BatchType> {
    batches_receiver: mpsc::Receiver<BatchType>,
    stats: Arc<AtomicSessionStats>,
}

impl<BatchType> ResultStream<BatchType> {
    /// # Returns
    ///
    /// The statistics recorded by the session's finished connections.
    #[must_use]
    pub fn stats(&self) -> SessionStats {
        self.stats.load()
    }
}

impl<BatchType> Stream for ResultStream<BatchType> {
    type Item = BatchType;

    fn poll_next(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Option<Self::Item>> {
        self.batches_receiver.poll_recv(cx)
    }
}

/// Statistics of a session's connections.
#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub struct SessionStats {
    /// The number of results emitted to the consumer.
    pub num_results_emitted: u64,

    /// The number of results dropped because another attempt of their task had already emitted
    /// them.
    pub num_duplicates_dropped: u64,
}

/// The sessions registered on a listener, keyed by their tokens.
///
/// # Type Parameters
///
/// * `EncodingType` - The encoding of the results the sessions deliver.
pub(crate) struct SessionRegistry<EncodingType: ResultEncoding> {
    sessions: DashMap<SessionToken, Arc<State<EncodingType>>>,
}

impl<EncodingType: ResultEncoding> SessionRegistry<EncodingType> {
    /// Factory function.
    ///
    /// Creates a registry without any session.
    ///
    /// # Returns
    ///
    /// The newly created registry.
    #[must_use]
    pub fn new() -> Self {
        Self {
            sessions: DashMap::new(),
        }
    }

    /// Opens a session with a fresh token, reachable at `host` and `port`, and registered before
    /// this method returns.
    ///
    /// # Returns
    ///
    /// On success, a tuple containing:
    ///
    /// * The newly opened session.
    /// * The stream of the session's results.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`SessionError::TokenCollision`] if the fresh token is already registered.
    pub fn open(
        self: &Arc<Self>,
        host: NonEmptyString,
        port: NonZeroU16,
        config: SessionConfig,
    ) -> Result<(Session<EncodingType>, ResultStream<EncodingType::Batch>), SessionError> {
        let token = SessionToken::generate();
        let Entry::Vacant(entry) = self.sessions.entry(token) else {
            return Err(SessionError::TokenCollision(token));
        };
        let (batches_sender, batches_receiver) = mpsc::channel(config.channel_capacity.get());
        let stats = Arc::new(AtomicSessionStats::default());
        entry.insert(Arc::new(State::new(batches_sender, Arc::clone(&stats))));
        let session = Session {
            _registration: Registration {
                session_registry: Arc::clone(self),
                token,
            },
            network_output: NetworkOutput {
                host,
                port,
                session_token: token,
            },
        };
        Ok((
            session,
            ResultStream {
                batches_receiver,
                stats,
            },
        ))
    }

    /// Joins a connection to the session and task cursor that `handshake` names.
    ///
    /// # Returns
    ///
    /// The joined connection on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ConnectionError::UnknownSession`] if no session is registered under the handshake's
    ///   token.
    /// * Forwards [`State::cursor`]'s return values on failure.
    pub fn join(&self, handshake: &Handshake) -> Result<Connection<EncodingType>, ConnectionError> {
        let session_state = self
            .sessions
            .get(&handshake.session_token)
            .map(|entry| Arc::clone(entry.value()))
            .ok_or(ConnectionError::UnknownSession(handshake.session_token))?;
        let cursor = session_state.cursor(handshake.task_index, handshake.archive_id)?;
        Ok(Connection::new(session_state, cursor))
    }

    /// Removes the session registered under `token`, and signals its connections that it has ended.
    /// Called only by [`Registration`]'s [`Drop`] implementation.
    ///
    /// # Panics
    ///
    /// Panics if no session is registered under `token`.
    fn unregister(&self, token: &SessionToken) {
        let (_, session_state) = self
            .sessions
            .remove(token)
            .expect("a session should stay registered until it is dropped");
        session_state.end_signal.cancel();
    }
}

/// The state of a session shared with its connections.
///
/// # Type Parameters
///
/// * `EncodingType` - The encoding of the results the session delivers.
pub(crate) struct State<EncodingType: ResultEncoding> {
    cursors: DashMap<TaskIndex, Arc<TaskCursor>>,
    batches_sender: mpsc::Sender<EncodingType::Batch>,
    end_signal: CancellationToken,
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

    /// # Returns
    ///
    /// The signal cancelled once the session has ended.
    #[must_use]
    pub const fn end_signal(&self) -> &CancellationToken {
        &self.end_signal
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

/// The statistics of a session's connections, shared between the session's state and its result
/// stream.
#[derive(Default)]
pub(crate) struct AtomicSessionStats {
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

/// A session's entry in a [`SessionRegistry`], unregistered when dropped.
///
/// # Type Parameters
///
/// * `EncodingType` - The encoding of the results the session delivers.
struct Registration<EncodingType: ResultEncoding> {
    session_registry: Arc<SessionRegistry<EncodingType>>,
    token: SessionToken,
}

impl<EncodingType: ResultEncoding> Drop for Registration<EncodingType> {
    fn drop(&mut self) {
        self.session_registry.unregister(&self.token);
    }
}

#[cfg(test)]
mod tests {
    use std::sync::Arc;

    use clp_rust_utils::types::ArchiveId;
    use clp_rust_utils::types::SessionToken;

    use super::Session;
    use super::SessionConfig;
    use super::SessionRegistry;
    use crate::error::ConnectionError;
    use crate::error::ProtocolError;
    use crate::protocol::Handshake;
    use crate::test_utils::ARCHIVE_ID;
    use crate::test_utils::QueryResultEncoding;

    /// Opens a session on `session_registry`, dropping the stream of its results.
    ///
    /// # Returns
    ///
    /// The newly opened session.
    ///
    /// # Panics
    ///
    /// Panics if the host fixture is empty, or the session's fresh token is already registered.
    fn open_session(
        session_registry: &Arc<SessionRegistry<QueryResultEncoding>>,
    ) -> Session<QueryResultEncoding> {
        use std::num::NonZeroU16;

        use non_empty_string::NonEmptyString;

        let (session, _results) = session_registry
            .open(
                NonEmptyString::new("127.0.0.1".to_owned())
                    .expect("the host fixture should be non-empty"),
                NonZeroU16::new(4000).expect("4000 is nonzero"),
                SessionConfig::default(),
            )
            .expect("a fresh session token shouldn't be registered");
        session
    }

    #[test]
    fn join_returns_unknown_session_for_an_unregistered_token() -> anyhow::Result<()> {
        use crate::test_utils::UNKNOWN_SESSION_TOKEN;

        let session_registry = Arc::new(SessionRegistry::<QueryResultEncoding>::new());
        let _session = open_session(&session_registry);
        let unknown_session_token = UNKNOWN_SESSION_TOKEN.parse::<SessionToken>()?;
        let Err(error) = session_registry.join(&Handshake {
            session_token: unknown_session_token,
            task_index: 0,
            archive_id: ARCHIVE_ID.parse::<ArchiveId>()?,
        }) else {
            anyhow::bail!("an attempt presenting an unknown token shouldn't join a session");
        };

        assert!(
            matches!(
                error,
                ConnectionError::UnknownSession(token) if unknown_session_token == token
            ),
            "unexpected error: {error:?}"
        );
        Ok(())
    }

    #[test]
    fn join_returns_archive_id_mismatch_for_a_different_archive_from_the_tasks_first_connection()
    -> anyhow::Result<()> {
        use crate::test_utils::OTHER_ARCHIVE_ID;

        let session_registry = Arc::new(SessionRegistry::<QueryResultEncoding>::new());
        let session = open_session(&session_registry);
        let archive_id = ARCHIVE_ID.parse::<ArchiveId>()?;
        let other_archive_id = OTHER_ARCHIVE_ID.parse::<ArchiveId>()?;
        let handshake = Handshake {
            session_token: session.network_output().session_token,
            task_index: 0,
            archive_id,
        };
        session_registry.join(&handshake)?;
        let Err(error) = session_registry.join(&Handshake {
            archive_id: other_archive_id,
            ..handshake
        }) else {
            anyhow::bail!("an attempt of another archive shouldn't join the session");
        };

        assert!(
            matches!(
                error,
                ConnectionError::Protocol(ProtocolError::ArchiveIdMismatch {
                    task_index: 0,
                    expected,
                    received,
                }) if archive_id == expected && other_archive_id == received
            ),
            "unexpected error: {error:?}"
        );
        Ok(())
    }
}

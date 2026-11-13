//! Sessions, each of which collects the results streamed for one query job.

use std::future::Future;
use std::num::NonZeroU16;
use std::num::NonZeroUsize;
use std::pin::Pin;
use std::sync::Arc;
use std::sync::OnceLock;
use std::sync::atomic::AtomicU64;
use std::sync::atomic::Ordering;
use std::task::Context;
use std::task::Poll;

use async_stream::stream;
use clp_rust_utils::job_config::NetworkOutput;
use clp_rust_utils::job_config::QueryJobStatus;
use clp_rust_utils::types::ArchiveId;
use clp_rust_utils::types::SessionToken;
use dashmap::DashMap;
use dashmap::Entry;
use futures::Stream;
use futures::StreamExt;
use non_empty_string::NonEmptyString;
use spider_core::task::TaskIndex;
use tokio::sync::mpsc;
use tokio_util::sync::CancellationToken;
use tokio_util::task::AbortOnDropHandle;

use crate::connection::Connection;
use crate::cursor::TaskCursor;
use crate::encoding::ResultEncoding;
use crate::error::ConnectionError;
use crate::error::ProtocolError;
use crate::error::SessionError;
use crate::error::SessionEventError;
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
/// The session accepts the connections that present its token until it ends. A session ends if:
///
/// * It is dropped.
/// * It fails, which happens if:
///   * A connection fails to create its encoder.
///   * A connection fails to encode a result it received.
///
/// Once a session ends, its open connections stop reading and end without an acknowledgement, but
/// still hand on the batches they've claimed.
///
/// # Type Parameters
///
/// * `EncodingType` - The encoding of the results the session delivers.
pub struct Session<EncodingType: ResultEncoding> {
    _registration: Registration<EncodingType>,
    network_output: NetworkOutput,
    state: Arc<State<EncodingType>>,
}

impl<EncodingType: ResultEncoding> Session<EncodingType> {
    /// # Returns
    ///
    /// The network output that directs a query job's tasks to stream their results to this session.
    #[must_use]
    pub const fn network_output(&self) -> &NetworkOutput {
        &self.network_output
    }

    /// Waits until the session fails, as [`Session`] defines it. Never returns for a session that
    /// doesn't fail.
    ///
    /// # Returns
    ///
    /// The error of the session's first failure.
    ///
    /// # Panics
    ///
    /// Panics if the session's end signal is cancelled without an encoding error recorded, which
    /// can't happen while the session is borrowed.
    pub async fn failed(&self) -> &EncodingType::Error {
        self.state.end_signal.cancelled().await;
        self.state
            .encoding_error()
            .expect("a session's end signal is cancelled only once it fails or is dropped")
    }
}

/// The stream of a session's results, in the batches its connections encoded them into.
///
/// The stream ends once its session is dropped and all its connections have ended.
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

/// An event of a session's query job.
///
/// # Type Parameters
///
/// * `BatchType` - The type of a batch of encoded results.
/// * `TerminationErrorType` - The error of a failure to wait for the job's termination.
#[derive(Debug)]
pub enum SessionEvent<BatchType, TerminationErrorType> {
    /// A batch of the job's results.
    Results(BatchType),

    /// The end of the events, once the job has terminated and the session's result stream has
    /// ended.
    Ended {
        /// The terminal status of the job.
        status: QueryJobStatus,

        /// The statistics of the session.
        stats: SessionStats,
    },

    /// The end of the events because the session failed or waiting for the job's termination
    /// failed. The job's cancellation has been requested.
    Failed(SessionEventError<TerminationErrorType>),
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

/// Streams the events of a query job from `session`, which the job's tasks stream their results to,
/// and its `results`.
///
/// The stream yields the batches of the job's results as they arrive, until `wait_for_termination`
/// resolves. Once `wait_for_termination` resolves with the job's terminal status, the stream ends
/// the session, yields the batches that the session's connections had already claimed until
/// `results` ends, and then yields a [`SessionEvent::Ended`]. Instead, if the session fails, or
/// `wait_for_termination` resolves with an error or panics, the stream requests the job's
/// cancellation and yields a [`SessionEvent::Failed`].
///
/// `wait_for_termination` runs in a task of its own on the current Tokio runtime from when this
/// function is called, so that it makes progress even while the stream isn't polled.
///
/// Dropping the stream ends the session and stops the task that runs `wait_for_termination`. If the
/// stream hasn't yet found the job terminated or yielded a [`SessionEvent::Failed`], dropping it
/// also requests the job's cancellation.
///
/// The stream requests the job's cancellation by spawning `request_cancellation` on the current
/// Tokio runtime. Outside a Tokio runtime, it only logs that the cancellation can't be requested.
///
/// # Type Parameters
///
/// * `EncodingType` - The encoding of the job's results.
/// * `TerminationErrorType` - The error of a failure to wait for the job's termination.
///
/// # Returns
///
/// The stream of the job's events.
///
/// # Panics
///
/// The returned stream panics if `results` ends while `session` is open, which the listener never
/// lets it do.
pub fn stream_events<EncodingType: ResultEncoding, TerminationErrorType: Send + 'static>(
    session: Session<EncodingType>,
    mut results: ResultStream<EncodingType::Batch>,
    wait_for_termination: impl Future<Output = Result<QueryJobStatus, TerminationErrorType>>
    + Send
    + 'static,
    request_cancellation: impl Future<Output = ()> + Send + 'static,
) -> impl Stream<Item = SessionEvent<EncodingType::Batch, TerminationErrorType>> + Send {
    let cancellation_guard = CancellationGuard::new(request_cancellation);
    let mut termination_task = AbortOnDropHandle::new(tokio::spawn(wait_for_termination));
    stream! {
        let end = loop {
            // `biased`, so that a session failure isn't missed because the job's status was seen
            // first, and a steady flow of batches doesn't keep the job's status from being noticed.
            tokio::select! {
                biased;
                error = session.failed() => {
                    break Err(SessionEventError::Encoding(error.to_string()));
                }
                status = &mut termination_task => {
                    break match status {
                        Ok(status) => status.map_err(SessionEventError::Termination),
                        Err(error) => Err(SessionEventError::TerminationPanicked(error)),
                    };
                }
                batch = results.next() => yield SessionEvent::Results(batch.expect(
                    "a session's result stream shouldn't end while the session is open",
                )),
            }
        };
        drop(session);
        match end {
            Ok(status) => {
                cancellation_guard.disarm();
                while let Some(batch) = results.next().await {
                    yield SessionEvent::Results(batch);
                }
                yield SessionEvent::Ended {
                    status,
                    stats: results.stats(),
                };
            }
            Err(error) => {
                drop(results);
                // Fires the cancellation before the failure is yielded, rather than once the stream
                // ends or is dropped.
                drop(cancellation_guard);
                yield SessionEvent::Failed(error);
            }
        }
    }
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
        let session_state = Arc::new(State::new(batches_sender, Arc::clone(&stats)));
        entry.insert(Arc::clone(&session_state));
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
            state: session_state,
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
    /// * [`ConnectionError::SessionFailed`] if the session has failed.
    /// * Forwards [`State::cursor`]'s return values on failure.
    /// * Forwards [`Connection::new`]'s return values on failure.
    pub fn join(&self, handshake: &Handshake) -> Result<Connection<EncodingType>, ConnectionError> {
        let session_state = self
            .sessions
            .get(&handshake.session_token)
            .map(|entry| Arc::clone(entry.value()))
            .ok_or(ConnectionError::UnknownSession(handshake.session_token))?;
        if session_state.encoding_error().is_some() {
            return Err(ConnectionError::SessionFailed);
        }
        let cursor = session_state.cursor(handshake.task_index, handshake.archive_id)?;
        Connection::new(session_state, cursor)
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

/// Why a session has ended.
pub(crate) enum SessionEnd {
    /// The session was dropped.
    Dropped,

    /// The session failed, as [`Session`] defines it.
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

/// The state of a session shared with its connections.
///
/// # Type Parameters
///
/// * `EncodingType` - The encoding of the results the session delivers.
pub(crate) struct State<EncodingType: ResultEncoding> {
    cursors: DashMap<TaskIndex, Arc<TaskCursor>>,
    batches_sender: mpsc::Sender<EncodingType::Batch>,
    end_signal: CancellationToken,
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
    /// already claimed, as [`Session`] explains.
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

/// A guard that fires the cancellation on drop.
///
/// # Type Parameters
///
/// * `CancellationType` - The type of the future that requests the cancellation.
struct CancellationGuard<CancellationType: Future<Output = ()> + Send + 'static> {
    cancellation: Option<CancellationType>,
}

impl<CancellationType: Future<Output = ()> + Send + 'static> CancellationGuard<CancellationType> {
    /// Factory function.
    ///
    /// # Returns
    ///
    /// A newly created guard that requests the cancellation by running `cancellation`.
    const fn new(cancellation: CancellationType) -> Self {
        Self {
            cancellation: Some(cancellation),
        }
    }

    /// Disarms the guard, dropping it without firing the cancellation.
    fn disarm(mut self) {
        self.cancellation = None;
    }
}

impl<CancellationType: Future<Output = ()> + Send + 'static> Drop
    for CancellationGuard<CancellationType>
{
    fn drop(&mut self) {
        let Some(cancellation) = self.cancellation.take() else {
            return;
        };
        let Ok(runtime) = tokio::runtime::Handle::try_current() else {
            tracing::warn!(
                "Can't request the cancellation of a session's query job outside a Tokio runtime."
            );
            return;
        };
        runtime.spawn(cancellation);
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
    use crate::encoding::ResultEncoding;
    use crate::error::ConnectionError;
    use crate::error::ProtocolError;
    use crate::protocol::Handshake;
    use crate::test_utils::ARCHIVE_ID;
    use crate::test_utils::FallibleQueryResultEncoding;
    use crate::test_utils::QueryResultEncoding;
    use crate::test_utils::TIMEOUT;
    use crate::test_utils::UNENCODABLE_ARCHIVE_ID;

    /// Opens a session on `session_registry`, dropping the stream of its results.
    ///
    /// # Type Parameters
    ///
    /// * `EncodingType` - The encoding of the results the session delivers.
    ///
    /// # Returns
    ///
    /// The newly opened session.
    ///
    /// # Panics
    ///
    /// Panics if the host fixture is empty, or the session's fresh token is already registered.
    fn open_session<EncodingType: ResultEncoding>(
        session_registry: &Arc<SessionRegistry<EncodingType>>,
    ) -> Session<EncodingType> {
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

    #[tokio::test]
    async fn join_fails_the_session_if_the_connections_encoder_cannot_be_created()
    -> anyhow::Result<()> {
        use crate::test_utils::EncodingError;

        let session_registry = Arc::new(SessionRegistry::<FallibleQueryResultEncoding>::new());
        let session = open_session(&session_registry);
        let unencodable_archive_id = UNENCODABLE_ARCHIVE_ID.parse::<ArchiveId>()?;
        let Err(error) = session_registry.join(&Handshake {
            session_token: session.network_output().session_token,
            task_index: 0,
            archive_id: unencodable_archive_id,
        }) else {
            anyhow::bail!("an attempt whose encoder can't be created shouldn't join the session");
        };

        let expected_error = EncodingError::UnencodableArchive(unencodable_archive_id);
        assert!(
            matches!(
                &error,
                ConnectionError::Encoding(message) if expected_error.to_string() == *message
            ),
            "unexpected error: {error:?}"
        );
        assert_eq!(
            tokio::time::timeout(TIMEOUT, session.failed()).await?,
            &expected_error
        );
        Ok(())
    }

    #[test]
    fn join_returns_session_failed_for_a_failed_session() -> anyhow::Result<()> {
        let session_registry = Arc::new(SessionRegistry::<FallibleQueryResultEncoding>::new());
        let session = open_session(&session_registry);
        let handshake = Handshake {
            session_token: session.network_output().session_token,
            task_index: 0,
            archive_id: UNENCODABLE_ARCHIVE_ID.parse::<ArchiveId>()?,
        };
        let Err(ConnectionError::Encoding(_)) = session_registry.join(&handshake) else {
            anyhow::bail!("an attempt whose encoder can't be created shouldn't join the session");
        };
        let Err(error) = session_registry.join(&Handshake {
            task_index: 0,
            archive_id: ARCHIVE_ID.parse::<ArchiveId>()?,
            ..handshake
        }) else {
            anyhow::bail!("an attempt shouldn't join a failed session");
        };

        assert!(
            matches!(error, ConnectionError::SessionFailed),
            "unexpected error: {error:?}"
        );
        Ok(())
    }

    #[tokio::test]
    async fn dropping_a_session_ends_its_connection_with_session_ended() -> anyhow::Result<()> {
        use bytes::BytesMut;

        let session_registry = Arc::new(SessionRegistry::<QueryResultEncoding>::new());
        let session = open_session(&session_registry);
        let mut connection = session_registry.join(&Handshake {
            session_token: session.network_output().session_token,
            task_index: 0,
            archive_id: ARCHIVE_ID.parse::<ArchiveId>()?,
        })?;
        let (_stream, mut handler_stream) = tokio::io::duplex(1024);
        let handler =
            tokio::spawn(async move { connection.run(&mut handler_stream, BytesMut::new()).await });

        drop(session);
        let result = tokio::time::timeout(TIMEOUT, handler).await??;

        assert!(
            matches!(result, Err(ConnectionError::SessionEnded)),
            "unexpected result: {result:?}"
        );
        Ok(())
    }
}

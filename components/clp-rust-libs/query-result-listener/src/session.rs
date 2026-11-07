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
use clp_rust_utils::types::SessionToken;
use dashmap::DashMap;
use futures::Stream;
use non_empty_string::NonEmptyString;
use spider_core::task::TaskIndex;
use tokio::sync::mpsc;

use crate::cursor::TaskCursor;
use crate::encoding::ResultEncoding;

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
/// The session accepts the connections that present its token until it is dropped.
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
    /// The network output that directs a query job's tasks to stream their results to this
    /// session.
    #[must_use]
    pub const fn network_output(&self) -> &NetworkOutput {
        &self.network_output
    }

    /// Factory function.
    ///
    /// Creates a session with a fresh token, reachable at `host` and `port`, and registers it in
    /// `sessions`.
    ///
    /// # Returns
    ///
    /// A tuple containing:
    ///
    /// * The newly created session.
    /// * The stream of the session's results.
    ///
    /// # Panics
    ///
    /// Panics if the fresh token is already registered, which a random UUID rules out.
    pub(crate) fn open(
        sessions: Arc<Registry<EncodingType>>,
        host: NonEmptyString,
        port: NonZeroU16,
        config: SessionConfig,
    ) -> (Self, ResultStream<EncodingType::Batch>) {
        let token = SessionToken::generate();
        let (batches_sender, batches_receiver) = mpsc::channel(config.channel_capacity.get());
        assert!(
            sessions
                .insert(token, Arc::new(State::new(batches_sender)))
                .is_none(),
            "a fresh session token shouldn't be registered"
        );
        let session = Self {
            _registration: Registration { sessions, token },
            network_output: NetworkOutput {
                host,
                port,
                session_token: token,
            },
        };
        (session, ResultStream { batches_receiver })
    }
}

/// The stream of a session's results, in the batches its connections encoded them into.
///
/// The stream ends once its session is dropped and every connection that joined the session has
/// finished.
///
/// # Type Parameters
///
/// * `BatchType` - The type of a batch of encoded results.
pub struct ResultStream<BatchType> {
    batches_receiver: mpsc::Receiver<BatchType>,
}

impl<BatchType> Stream for ResultStream<BatchType> {
    type Item = BatchType;

    fn poll_next(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Option<Self::Item>> {
        self.batches_receiver.poll_recv(cx)
    }
}

/// The registered sessions of a listener, keyed by their tokens.
///
/// # Type Parameters
///
/// * `EncodingType` - The encoding of the results the sessions deliver.
pub(crate) type Registry<EncodingType> = DashMap<SessionToken, Arc<State<EncodingType>>>;

/// Statistics of a session's connections.
#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
#[expect(
    clippy::struct_field_names,
    reason = "`num_` marks a count throughout the crate, as in `Trailer::num_results`"
)]
pub(crate) struct SessionStats {
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
/// * `EncodingType` - The encoding of the results the session delivers.
pub(crate) struct State<EncodingType: ResultEncoding> {
    /// The cursors of the session's query tasks, keyed by task index.
    pub cursors: DashMap<TaskIndex, Arc<TaskCursor>>,

    /// The sender of the session's batches of encoded results.
    pub batches_sender: mpsc::Sender<EncodingType::Batch>,

    num_results_emitted: AtomicU64,
    num_duplicates_dropped: AtomicU64,
    num_results_discarded: AtomicU64,
}

impl<EncodingType: ResultEncoding> State<EncodingType> {
    /// Factory function.
    ///
    /// Creates the state of a session that sends its batches to `batches_sender`.
    ///
    /// # Returns
    ///
    /// The newly created state.
    #[must_use]
    pub fn new(batches_sender: mpsc::Sender<EncodingType::Batch>) -> Self {
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
    #[cfg_attr(
        not(test),
        expect(
            dead_code,
            reason = "the session's outcome that reports it comes in a later change"
        )
    )]
    #[must_use]
    pub fn stats(&self) -> SessionStats {
        SessionStats {
            num_results_emitted: self.num_results_emitted.load(Ordering::Relaxed),
            num_duplicates_dropped: self.num_duplicates_dropped.load(Ordering::Relaxed),
            num_results_discarded: self.num_results_discarded.load(Ordering::Relaxed),
        }
    }
}

/// A session's entry in its listener's registry, removed when dropped.
///
/// # Type Parameters
///
/// * `EncodingType` - The encoding of the results the session delivers.
struct Registration<EncodingType: ResultEncoding> {
    sessions: Arc<Registry<EncodingType>>,
    token: SessionToken,
}

impl<EncodingType: ResultEncoding> Drop for Registration<EncodingType> {
    fn drop(&mut self) {
        self.sessions.remove(&self.token);
    }
}

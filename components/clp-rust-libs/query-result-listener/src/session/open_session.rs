//! Open sessions, each paired with the stream of its results.

use std::future::Future;
use std::pin::Pin;
use std::sync::Arc;
use std::task::Context;
use std::task::Poll;

use async_stream::stream;
use clp_rust_utils::job_config::NetworkOutput;
use clp_rust_utils::job_config::QueryJobStatus;
use futures::Stream;
use futures::StreamExt;
use tokio::sync::mpsc;
use tokio_util::task::AbortOnDropHandle;

use crate::encoding::ResultEncoding;
use crate::session::SessionEvent;
use crate::session::SessionEventError;
use crate::session::SessionStats;
use crate::session::cancellation_guard::CancellationGuard;
use crate::session::registry::Session;
use crate::session::stats::AtomicSessionStats;

/// A session paired with the stream of its results.
///
/// Dropping it ends the session.
///
/// # Type Parameters
///
/// * `EncodingType` - The encoding of the results the session delivers.
pub struct OpenSession<EncodingType: ResultEncoding> {
    pub(super) session: Session<EncodingType>,
    pub(super) results: ResultStream<EncodingType::Batch>,
}

impl<EncodingType: ResultEncoding> OpenSession<EncodingType> {
    /// # Returns
    ///
    /// The network output that directs a query job's tasks to stream their results to this session.
    #[must_use]
    pub const fn network_output(&self) -> &NetworkOutput {
        self.session.network_output()
    }

    /// Streams the events of the session's query job.
    ///
    /// The stream yields the job's results as [`SessionEvent::Results`].
    ///
    /// Once `wait_for_termination` resolves with the job's terminal status, the stream ends the
    /// session, yields the results its connections had already claimed, and then yields
    /// [`SessionEvent::Ended`].
    ///
    /// If the session fails, or `wait_for_termination` fails or panics, the stream requests the
    /// job's cancellation through `request_cancellation` and yields [`SessionEvent::Failed`]
    /// instead.
    ///
    /// Dropping the stream before it has seen the job terminate or failed requests the job's
    /// cancellation, so `request_cancellation` must tolerate a job that has already terminated.
    ///
    /// # Type Parameters
    ///
    /// * `TerminationErrorType` - The error of a failure to wait for the job's termination.
    ///
    /// # Returns
    ///
    /// The stream of the job's events.
    ///
    /// # Panics
    ///
    /// The returned stream panics if the session's result stream ends while the session is open,
    /// which should never happen in practice (assuming called inside a Tokio runtime).
    pub fn into_events<TerminationErrorType: std::error::Error + Send + Sync + 'static>(
        self,
        wait_for_termination: impl Future<Output = Result<QueryJobStatus, TerminationErrorType>>
        + Send
        + 'static,
        request_cancellation: impl Future<Output = ()> + Send + 'static,
    ) -> impl Stream<Item = SessionEvent<EncodingType::Batch>> + Send {
        let Self {
            session,
            mut results,
        } = self;
        let cancellation_guard = CancellationGuard::new(request_cancellation);
        let mut termination_task = AbortOnDropHandle::new(tokio::spawn(wait_for_termination));
        stream! {
            let end = loop {
                // `biased`, so that a session failure isn't missed because the job's status was
                // seen first, and a steady flow of batches doesn't keep the job's status from being
                // noticed.
                tokio::select! {
                    biased;
                    error = session.failed() => {
                        break Err(SessionEventError::Encoding(error.to_string()));
                    }
                    status = &mut termination_task => {
                        break match status {
                            Ok(status) => status
                                .map_err(|error| SessionEventError::Termination(Box::new(error))),
                            Err(error) => Err(SessionEventError::TerminationTaskFailed(error)),
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
                    drop(termination_task);
                    cancellation_guard.request().await;
                    yield SessionEvent::Failed(error);
                }
            }
        }
    }
}

/// The stream of a session's results, in the batches its connections encoded them into.
///
/// The stream ends once its session is dropped and all its connections have ended.
///
/// # Type Parameters
///
/// * `BatchType` - The type of a batch of encoded results.
pub(super) struct ResultStream<BatchType> {
    pub(super) batches_receiver: mpsc::Receiver<BatchType>,
    pub(super) stats: Arc<AtomicSessionStats>,
}

impl<BatchType> ResultStream<BatchType> {
    /// # Returns
    ///
    /// The statistics recorded by the session's finished connections.
    #[must_use]
    fn stats(&self) -> SessionStats {
        self.stats.load()
    }
}

impl<BatchType> Stream for ResultStream<BatchType> {
    type Item = BatchType;

    fn poll_next(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Option<Self::Item>> {
        self.batches_receiver.poll_recv(cx)
    }
}

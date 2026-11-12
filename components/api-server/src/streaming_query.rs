//! Streaming queries, each of which forwards the results that its query job's tasks stream to the
//! API server while the job runs, and ends with the job's terminal status.

use std::future::Future;

use async_stream::stream;
use clp_rust_utils::job_config::QueryJobStatus;
use futures::Stream;
use futures::StreamExt;
use query_result_listener::encoding::ResultEncoding;
use query_result_listener::session::ResultStream;
use query_result_listener::session::Session;
use query_result_listener::session::SessionStats;
use tokio_util::task::AbortOnDropHandle;

use crate::error::ClientError;

/// A streaming query whose job has been submitted.
///
/// # Type Parameters
///
/// * `EventStreamType` - The type of the stream of the query's events.
pub struct StreamingQuery<EventStreamType> {
    /// The ID of the query's job.
    pub query_job_id: u64,

    /// The stream of the query's events, as [`stream_events`] describes.
    pub events: EventStreamType,
}

/// An item of a streaming query's stream.
///
/// # Type Parameters
///
/// * `BatchType` - The type of a batch of encoded results.
#[derive(Debug)]
pub enum StreamingQueryEvent<BatchType> {
    /// A batch of the query's results.
    Results(BatchType),

    /// The end of the query, once its job has terminated and its session's result stream has ended.
    End(StreamingQueryEnd),

    /// The end of the query because its session failed or waiting for its job's termination failed.
    /// The job's cancellation has been requested.
    Error(ClientError),
}

/// The end of a streaming query whose job has terminated.
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct StreamingQueryEnd {
    /// The terminal status of the query's job.
    pub status: QueryJobStatus,

    /// The statistics of the query's session.
    pub stats: SessionStats,
}

/// Streams the events of a streaming query from `session`, which the tasks of the query's job
/// stream their results to, and its `results`.
///
/// The stream yields the batches of the query's results as they arrive, until `termination`
/// resolves. Once `termination` resolves with the job's terminal status, the stream ends the
/// session, yields the batches that the session's connections had already claimed until `results`
/// ends, and then yields a [`StreamingQueryEvent::End`]. Instead, if the session fails, or
/// `termination` resolves with an error or panics, the stream requests the job's cancellation and
/// yields a [`StreamingQueryEvent::Error`].
///
/// `termination` runs in a task of its own on the current Tokio runtime from when this function is
/// called, so that it makes progress even while the stream isn't polled.
///
/// Dropping the stream ends the session and stops the task that runs `termination`. If the stream
/// hasn't yet found the job terminated or yielded an error, dropping it also requests the job's
/// cancellation.
///
/// The stream requests the job's cancellation by spawning `cancellation` on the current Tokio
/// runtime. Outside a Tokio runtime, it only logs that the cancellation can't be requested.
///
/// # Type Parameters
///
/// * `EncodingType` - The encoding of the query's results.
///
/// # Returns
///
/// The stream of the query's events.
///
/// # Panics
///
/// The returned stream panics if `results` ends while `session` is open, which the listener never
/// lets it do.
pub fn stream_events<EncodingType: ResultEncoding>(
    session: Session<EncodingType>,
    mut results: ResultStream<EncodingType::Batch>,
    termination: impl Future<Output = Result<QueryJobStatus, ClientError>> + Send + 'static,
    cancellation: impl Future<Output = ()> + Send + 'static,
) -> impl Stream<Item = StreamingQueryEvent<EncodingType::Batch>> + Send {
    let mut cancellation = CancellationGuard::new(cancellation);
    let mut termination = AbortOnDropHandle::new(tokio::spawn(termination));
    stream! {
        let end = loop {
            // `biased`, so that a session failure isn't missed because the job's status was seen
            // first, and a steady flow of batches doesn't keep the job's status from being noticed.
            tokio::select! {
                biased;
                error = session.failed() => {
                    break Err(ClientError::QueryResultEncoding(error.to_string()));
                }
                status = &mut termination => {
                    break status.unwrap_or_else(|error| Err(ClientError::TaskJoin(error)));
                }
                batch = results.next() => yield StreamingQueryEvent::Results(batch.expect(
                    "a session's result stream shouldn't end while the session is open",
                )),
            }
        };
        drop(session);
        match end {
            Ok(status) => {
                cancellation.disarm();
                while let Some(batch) = results.next().await {
                    yield StreamingQueryEvent::Results(batch);
                }
                yield StreamingQueryEvent::End(StreamingQueryEnd {
                    status,
                    stats: results.stats(),
                });
            }
            Err(error) => {
                drop(results);
                cancellation.request();
                yield StreamingQueryEvent::Error(error);
            }
        }
    }
}

/// A guard that requests the cancellation of a streaming query's job if it's dropped before the
/// cancellation has been requested or the guard disarmed.
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

    /// Disarms the guard, so that dropping it doesn't request the cancellation.
    fn disarm(&mut self) {
        self.cancellation = None;
    }

    /// Requests the cancellation in a background task, unless it has already been requested or the
    /// guard disarmed. Outside a Tokio runtime, only logs that the cancellation can't be requested.
    fn request(&mut self) {
        let Some(cancellation) = self.cancellation.take() else {
            return;
        };
        let Ok(runtime) = tokio::runtime::Handle::try_current() else {
            tracing::warn!(
                "Can't request the cancellation of a streaming query's job outside a Tokio \
                 runtime."
            );
            return;
        };
        runtime.spawn(cancellation);
    }
}

impl<CancellationType: Future<Output = ()> + Send + 'static> Drop
    for CancellationGuard<CancellationType>
{
    fn drop(&mut self) {
        self.request();
    }
}

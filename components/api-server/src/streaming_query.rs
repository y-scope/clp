//! The event streams of streaming queries, each of which forwards the results that its query job's
//! tasks stream to the API server while the job runs, and ends with the job's terminal status.

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
/// The stream yields the batches of the query's results as they arrive, until
/// `wait_for_termination` resolves. Once `wait_for_termination` resolves with the job's terminal
/// status, the stream ends the session, yields the batches that the session's connections had
/// already claimed until `results` ends, and then yields a [`StreamingQueryEvent::End`]. Instead,
/// if the session fails, or `wait_for_termination` resolves with an error or panics, the stream
/// requests the job's cancellation and yields a [`StreamingQueryEvent::Error`].
///
/// `wait_for_termination` runs in a task of its own on the current Tokio runtime from when this
/// function is called, so that it makes progress even while the stream isn't polled.
///
/// Dropping the stream ends the session and stops the task that runs `wait_for_termination`. If the
/// stream hasn't yet found the job terminated or yielded an error, dropping it also requests the
/// job's cancellation.
///
/// The stream requests the job's cancellation by spawning `request_cancellation` on the current
/// Tokio runtime. Outside a Tokio runtime, it only logs that the cancellation can't be requested.
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
    wait_for_termination: impl Future<Output = Result<QueryJobStatus, crate::error::ClientError>>
    + Send
    + 'static,
    request_cancellation: impl Future<Output = ()> + Send + 'static,
) -> impl Stream<Item = StreamingQueryEvent<EncodingType::Batch>> + Send {
    let mut cancellation_guard = CancellationGuard::new(request_cancellation);
    let mut termination_task = AbortOnDropHandle::new(tokio::spawn(wait_for_termination));
    stream! {
        let end = loop {
            // `biased`, so that a session failure isn't missed because the job's status was seen
            // first, and a steady flow of batches doesn't keep the job's status from being noticed.
            tokio::select! {
                biased;
                error = session.failed() => {
                    break Err(ClientError::QueryResultEncoding(error.to_string()));
                }
                status = &mut termination_task => {
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
                cancellation_guard.disarm();
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
                cancellation_guard.request();
                yield StreamingQueryEvent::Error(error);
            }
        }
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

#[cfg(test)]
mod tests {
    use std::future::Future;
    use std::time::Duration;

    use anyhow::Result;
    use clp_rust_utils::job_config::NetworkOutput;
    use clp_rust_utils::job_config::QueryJobStatus;
    use futures::Stream;
    use futures::StreamExt;
    use futures::channel::mpsc;
    use query_result_listener::encoding::ResultEncoding;
    use query_result_listener::listener::ResultListener;
    use query_result_listener::session::SessionConfig;
    use query_result_listener::session::SessionStats;
    use query_result_listener::test_utils::ARCHIVE_ID;
    use query_result_listener::test_utils::FakeClpS;
    use query_result_listener::test_utils::QueryResult;
    use query_result_listener::test_utils::QueryResultEncoding;
    use query_result_listener::test_utils::TIMEOUT;
    use query_result_listener::test_utils::expected_results;
    use tokio::sync::oneshot;
    use tokio::task::yield_now;
    use tokio::time::sleep;
    use tokio::time::timeout;

    use super::StreamingQueryEnd;
    use super::StreamingQueryEvent;
    use super::stream_events;
    use crate::error::ClientError;

    /// The index of the query task whose results the tests' attempts stream.
    const TASK_INDEX: usize = 0;

    /// How long a test waits for the listener to read what an attempt has sent.
    const SETTLE_DELAY: Duration = Duration::from_millis(100);

    type TestEvent = StreamingQueryEvent<Vec<QueryResult>>;

    /// The items that a streaming query's stream yields from some point until its last item.
    struct Ending {
        results: Vec<QueryResult>,
        last_event: TestEvent,
    }

    impl Ending {
        /// Receives the items of `events` until its end or error, without polling it further.
        ///
        /// # Returns
        ///
        /// The results of the received batches, and the stream's last item.
        ///
        /// # Panics
        ///
        /// Panics if the stream doesn't yield its end or error within [`TIMEOUT`], or ends without
        /// one.
        async fn receive(events: &mut (impl Stream<Item = TestEvent> + Unpin)) -> Self {
            let mut results = Vec::new();
            let last_event = loop {
                match next_event(events)
                    .await
                    .expect("the query stream shouldn't end without an end or an error")
                {
                    StreamingQueryEvent::Results(batch) => results.extend(batch),
                    last_event => break last_event,
                }
            };
            Self {
                results,
                last_event,
            }
        }

        /// # Returns
        ///
        /// The end that the stream's last item carries.
        ///
        /// # Panics
        ///
        /// Panics if the stream's last item isn't an end.
        fn end(&self) -> StreamingQueryEnd {
            let StreamingQueryEvent::End(end) = &self.last_event else {
                panic!(
                    "the query stream should end with an end: {:?}",
                    self.last_event
                );
            };
            *end
        }

        /// # Returns
        ///
        /// The error that the stream's last item carries.
        ///
        /// # Panics
        ///
        /// Panics if the stream's last item isn't an error.
        fn error(&self) -> &ClientError {
            let StreamingQueryEvent::Error(error) = &self.last_event else {
                panic!(
                    "the query stream should end with an error: {:?}",
                    self.last_event
                );
            };
            error
        }
    }

    /// # Type Parameters
    ///
    /// * `EncodingType` - The encoding of the results that the listener's sessions deliver.
    ///
    /// # Returns
    ///
    /// A loopback listener whose handshake timeout outlasts [`TIMEOUT`].
    ///
    /// # Panics
    ///
    /// Panics if the advertised host fixture is empty, or the listener can't be bound.
    fn bind_listener<EncodingType: ResultEncoding>() -> ResultListener<EncodingType> {
        use std::net::Ipv4Addr;
        use std::net::SocketAddr;
        use std::num::NonZeroU64;

        use non_empty_string::NonEmptyString;
        use query_result_listener::listener::ListenerConfig;

        ResultListener::bind(ListenerConfig {
            bind_addr: SocketAddr::from((Ipv4Addr::LOCALHOST, 0)),
            advertised_host: NonEmptyString::new("127.0.0.1".to_owned())
                .expect("the advertised host fixture should be non-empty"),
            handshake_timeout_sec: NonZeroU64::new(6 * TIMEOUT.as_secs())
                .expect("the handshake timeout should be nonzero"),
        })
        .expect("binding a loopback listener should succeed")
    }

    /// # Returns
    ///
    /// A tuple containing:
    ///
    /// * The sender of the result that the returned future resolves with.
    /// * A future that waits for a query job's termination, which resolves with the result that the
    ///   sender sends.
    ///
    /// # Panics
    ///
    /// The returned future panics if the sender is dropped without sending a result.
    fn wait_for_termination() -> (
        oneshot::Sender<Result<QueryJobStatus, ClientError>>,
        impl Future<Output = Result<QueryJobStatus, ClientError>> + Send + 'static,
    ) {
        let (sender, receiver) = oneshot::channel();
        let wait_for_termination = async move {
            receiver
                .await
                .expect("the test should send the job's termination before dropping its sender")
        };
        (sender, wait_for_termination)
    }

    /// # Returns
    ///
    /// A tuple containing:
    ///
    /// * A future that requests a query job's cancellation by sending a request to the returned
    ///   receiver.
    /// * The receiver of the cancellation requests, which ends once the future has run or been
    ///   dropped.
    ///
    /// # Panics
    ///
    /// The returned future panics if the receiver has been dropped.
    fn request_cancellation() -> (
        impl Future<Output = ()> + Send + 'static,
        mpsc::UnboundedReceiver<()>,
    ) {
        let (sender, receiver) = mpsc::unbounded();
        let request_cancellation = async move {
            sender
                .unbounded_send(())
                .expect("the test should receive the cancellation request");
        };
        (request_cancellation, receiver)
    }

    /// # Returns
    ///
    /// The next item of `events`, or `None` if it has ended.
    ///
    /// # Panics
    ///
    /// Panics if neither happens within [`TIMEOUT`].
    async fn next_event(events: &mut (impl Stream<Item = TestEvent> + Unpin)) -> Option<TestEvent> {
        timeout(TIMEOUT, events.next())
            .await
            .expect("the query stream should yield an item or end")
    }

    /// Receives batches from `events` until they hold `num_results` results.
    ///
    /// # Returns
    ///
    /// The received results.
    ///
    /// # Panics
    ///
    /// Panics if the stream yields anything but batches, or the results don't arrive within
    /// [`TIMEOUT`].
    async fn receive_results(
        events: &mut (impl Stream<Item = TestEvent> + Unpin),
        num_results: usize,
    ) -> Vec<QueryResult> {
        let mut received = Vec::new();
        while received.len() < num_results {
            match next_event(events).await {
                Some(StreamingQueryEvent::Results(batch)) => received.extend(batch),
                event => panic!("the query stream should yield a batch: {event:?}"),
            }
        }
        received
    }

    /// Connects an attempt of task [`TASK_INDEX`] of archive [`ARCHIVE_ID`] to the session that
    /// `network_output` names, and sends its results at `0..2`, then those at `2..4` in a separate
    /// read, so that the first batch fills a session's channel of capacity 1 that nothing consumes,
    /// and the attempt waits to send the second.
    ///
    /// # Returns
    ///
    /// The connected attempt.
    ///
    /// # Panics
    ///
    /// Panics if connecting or sending fails.
    async fn connect_attempt_blocked_on_its_second_batch(
        network_output: &NetworkOutput,
    ) -> FakeClpS {
        let mut attempt = FakeClpS::connect(network_output).await;
        attempt.send_handshake(TASK_INDEX, ARCHIVE_ID).await;
        attempt.send_results(TASK_INDEX, 0..2).await;
        sleep(SETTLE_DELAY).await;
        attempt.send_results(TASK_INDEX, 2..4).await;
        sleep(SETTLE_DELAY).await;
        attempt
    }

    #[tokio::test]
    async fn results_are_forwarded_while_the_job_runs_and_the_end_carries_its_status_and_stats()
    -> Result<()> {
        let listener = bind_listener::<QueryResultEncoding>();
        let (session, results) = listener.open_session(SessionConfig::default())?;
        let network_output = session.network_output().clone();
        let (termination_sender, wait_for_termination) = wait_for_termination();
        let mut events = Box::pin(stream_events(
            session,
            results,
            wait_for_termination,
            async {},
        ));
        let mut attempt = FakeClpS::connect(&network_output).await;
        attempt.send_handshake(TASK_INDEX, ARCHIVE_ID).await;
        attempt.send_results(TASK_INDEX, 0..3).await;
        let received = receive_results(&mut events, 3).await;
        attempt.finish(3).await;

        termination_sender
            .send(Ok(QueryJobStatus::Succeeded))
            .expect("the stream should wait for the job's termination");
        let ending = Ending::receive(&mut events).await;

        assert_eq!(received, expected_results(TASK_INDEX, ARCHIVE_ID, 0..3));
        assert_eq!(ending.results, [] as [QueryResult; 0]);
        assert_eq!(
            ending.end(),
            StreamingQueryEnd {
                status: QueryJobStatus::Succeeded,
                stats: SessionStats {
                    num_results_emitted: 3,
                    num_duplicates_dropped: 0,
                },
            }
        );
        Ok(())
    }

    #[tokio::test]
    async fn end_comes_only_after_a_batch_claimed_before_the_job_terminated_is_forwarded()
    -> Result<()> {
        use std::num::NonZeroUsize;

        let listener = bind_listener::<QueryResultEncoding>();
        let (session, results) = listener.open_session(SessionConfig {
            channel_capacity: NonZeroUsize::MIN,
        })?;
        let network_output = session.network_output().clone();
        let (termination_sender, wait_for_termination) = wait_for_termination();
        let mut events = Box::pin(stream_events(
            session,
            results,
            wait_for_termination,
            async {},
        ));
        let _attempt = connect_attempt_blocked_on_its_second_batch(&network_output).await;
        termination_sender
            .send(Ok(QueryJobStatus::Succeeded))
            .expect("the stream should wait for the job's termination");

        let ending = Ending::receive(&mut events).await;

        assert_eq!(
            ending.results,
            expected_results(TASK_INDEX, ARCHIVE_ID, 0..4)
        );
        assert_eq!(
            ending.end(),
            StreamingQueryEnd {
                status: QueryJobStatus::Succeeded,
                stats: SessionStats {
                    num_results_emitted: 4,
                    num_duplicates_dropped: 0,
                },
            }
        );
        Ok(())
    }

    #[tokio::test]
    async fn termination_is_waited_for_while_the_stream_is_not_polled() -> Result<()> {
        let listener = bind_listener::<QueryResultEncoding>();
        let (session, results) = listener.open_session(SessionConfig::default())?;
        let (wait_finished_sender, wait_finished) = oneshot::channel();
        let wait_for_termination = async move {
            for _ in 0..3 {
                yield_now().await;
            }
            wait_finished_sender
                .send(())
                .expect("the test should wait for the job's termination to finish");
            Ok(QueryJobStatus::Succeeded)
        };
        let _events = stream_events(session, results, wait_for_termination, async {});

        timeout(TIMEOUT, wait_finished).await??;
        Ok(())
    }

    #[tokio::test]
    async fn session_failure_takes_priority_over_a_terminal_status_and_requests_cancellation_once()
    -> Result<()> {
        use clp_rust_utils::types::ArchiveId;
        use query_result_listener::test_utils::EncodingError;
        use query_result_listener::test_utils::FallibleQueryResultEncoding;
        use query_result_listener::test_utils::UNENCODABLE_MESSAGE;
        use query_result_listener::test_utils::encode_result;
        use query_result_listener::test_utils::timestamp;

        let listener = bind_listener::<FallibleQueryResultEncoding>();
        let (session, results) = listener.open_session(SessionConfig::default())?;
        let network_output = session.network_output().clone();
        let (termination_sender, wait_for_termination) = wait_for_termination();
        let (request_cancellation, cancellation_requests) = request_cancellation();
        let mut events = Box::pin(stream_events(
            session,
            results,
            wait_for_termination,
            request_cancellation,
        ));
        let mut failing_attempt = FakeClpS::connect(&network_output).await;
        failing_attempt.send_handshake(TASK_INDEX, ARCHIVE_ID).await;
        failing_attempt
            .send(&encode_result(0, timestamp(0), UNENCODABLE_MESSAGE))
            .await;
        // The listener closes the connection only once it has failed the session.
        failing_attempt.expect_closed_by_listener().await;
        termination_sender
            .send(Ok(QueryJobStatus::Succeeded))
            .expect("the stream should wait for the job's termination");
        // Lets the task waiting for the job's termination finish before the stream is polled.
        yield_now().await;

        let ending = Ending::receive(&mut events).await;

        let expected_error = EncodingError::UnencodableMessage {
            archive_id: ARCHIVE_ID.parse::<ArchiveId>()?,
            timestamp_millisecs: timestamp(0),
        };
        assert!(
            matches!(
                ending.error(),
                ClientError::QueryResultEncoding(message) if expected_error.to_string() == *message
            ),
            "unexpected error: {:?}",
            ending.error()
        );
        // Counted while the stream is alive, so that only the error can have requested the
        // cancellation.
        assert_eq!(timeout(TIMEOUT, cancellation_requests.count()).await?, 1);
        Ok(())
    }

    #[tokio::test]
    async fn failed_termination_ends_the_stream_with_its_error_and_requests_cancellation_once()
    -> Result<()> {
        use std::num::NonZeroUsize;

        const TERMINATION_ERROR: &str = "waiting for the job's termination failed";

        let listener = bind_listener::<QueryResultEncoding>();
        let (session, results) = listener.open_session(SessionConfig {
            channel_capacity: NonZeroUsize::MIN,
        })?;
        let network_output = session.network_output().clone();
        let (termination_sender, wait_for_termination) = wait_for_termination();
        let (request_cancellation, cancellation_requests) = request_cancellation();
        let mut events = Box::pin(stream_events(
            session,
            results,
            wait_for_termination,
            request_cancellation,
        ));
        let attempt = connect_attempt_blocked_on_its_second_batch(&network_output).await;
        termination_sender
            .send(Err(ClientError::Io(std::io::Error::other(
                TERMINATION_ERROR,
            ))))
            .expect("the stream should wait for the job's termination");
        // Lets the task waiting for the job's termination finish before the stream is polled.
        yield_now().await;

        let ending = Ending::receive(&mut events).await;
        attempt.expect_closed_by_listener().await;

        assert_eq!(ending.results, [] as [QueryResult; 0]);
        assert!(
            matches!(
                ending.error(),
                ClientError::Io(error) if TERMINATION_ERROR == error.to_string()
            ),
            "unexpected error: {:?}",
            ending.error()
        );
        // Counted while the stream is alive, so that only the error can have requested the
        // cancellation.
        assert_eq!(timeout(TIMEOUT, cancellation_requests.count()).await?, 1);
        Ok(())
    }

    #[tokio::test]
    async fn panicked_termination_ends_the_stream_with_a_join_error_and_requests_cancellation_once()
    -> Result<()> {
        let listener = bind_listener::<QueryResultEncoding>();
        let (session, results) = listener.open_session(SessionConfig::default())?;
        let (request_cancellation, cancellation_requests) = request_cancellation();
        let mut events = Box::pin(stream_events(
            session,
            results,
            async { panic!("the test's wait for the job's termination panics") },
            request_cancellation,
        ));

        let ending = Ending::receive(&mut events).await;

        assert!(
            matches!(ending.error(), ClientError::TaskJoin(error) if error.is_panic()),
            "unexpected error: {:?}",
            ending.error()
        );
        // Counted while the stream is alive, so that only the error can have requested the
        // cancellation.
        assert_eq!(timeout(TIMEOUT, cancellation_requests.count()).await?, 1);
        Ok(())
    }

    #[tokio::test]
    async fn dropping_the_stream_before_its_end_requests_cancellation_once_and_ends_the_session()
    -> Result<()> {
        let listener = bind_listener::<QueryResultEncoding>();
        let (session, results) = listener.open_session(SessionConfig::default())?;
        let network_output = session.network_output().clone();
        let (request_cancellation, cancellation_requests) = request_cancellation();
        let events = stream_events(
            session,
            results,
            std::future::pending(),
            request_cancellation,
        );
        let mut attempt = FakeClpS::connect(&network_output).await;
        attempt.send_handshake(TASK_INDEX, ARCHIVE_ID).await;
        // Lets the listener join the attempt to the session while the job runs.
        sleep(SETTLE_DELAY).await;

        drop(events);
        attempt.expect_closed_by_listener().await;

        assert_eq!(timeout(TIMEOUT, cancellation_requests.count()).await?, 1);
        Ok(())
    }

    #[tokio::test]
    async fn dropping_the_stream_while_the_job_runs_stops_waiting_for_termination() -> Result<()> {
        let listener = bind_listener::<QueryResultEncoding>();
        let (session, results) = listener.open_session(SessionConfig::default())?;
        let (mut termination_sender, wait_for_termination) = wait_for_termination();
        let events = stream_events(session, results, wait_for_termination, async {});

        drop(events);

        timeout(TIMEOUT, termination_sender.closed()).await?;
        Ok(())
    }

    #[tokio::test]
    async fn dropping_the_stream_while_it_drains_after_the_job_terminated_requests_nothing()
    -> Result<()> {
        use std::num::NonZeroUsize;

        let listener = bind_listener::<QueryResultEncoding>();
        let (session, results) = listener.open_session(SessionConfig {
            channel_capacity: NonZeroUsize::MIN,
        })?;
        let network_output = session.network_output().clone();
        let (termination_sender, wait_for_termination) = wait_for_termination();
        let (request_cancellation, cancellation_requests) = request_cancellation();
        let mut events = Box::pin(stream_events(
            session,
            results,
            wait_for_termination,
            request_cancellation,
        ));
        let _attempt = connect_attempt_blocked_on_its_second_batch(&network_output).await;
        termination_sender
            .send(Ok(QueryJobStatus::Succeeded))
            .expect("the stream should wait for the job's termination");
        // Lets the task waiting for the job's termination finish before the stream is polled.
        yield_now().await;
        // The stream finds the job terminated before it forwards the first batch, and then waits
        // for the second.
        receive_results(&mut events, 2).await;

        drop(events);

        assert_eq!(timeout(TIMEOUT, cancellation_requests.count()).await?, 0);
        Ok(())
    }

    #[test]
    fn dropping_the_stream_outside_a_runtime_does_not_panic() -> Result<()> {
        use futures::FutureExt;

        let runtime = tokio::runtime::Builder::new_current_thread()
            .enable_all()
            .build()?;
        let (request_cancellation, cancellation_requests) = request_cancellation();
        let events = {
            let _runtime_context = runtime.enter();
            let listener = bind_listener::<QueryResultEncoding>();
            let (session, results) = listener.open_session(SessionConfig::default())?;
            stream_events(
                session,
                results,
                std::future::pending(),
                request_cancellation,
            )
        };
        drop(runtime);

        drop(events);

        assert_eq!(cancellation_requests.count().now_or_never(), Some(0));
        Ok(())
    }
}

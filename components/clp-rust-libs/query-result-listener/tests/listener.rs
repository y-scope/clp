use std::future::Future;
use std::io::ErrorKind;
use std::num::NonZeroU64;
use std::time::Duration;

use anyhow::Result;
use clp_rust_utils::job_config::NetworkOutput;
use clp_rust_utils::job_config::QueryJobStatus;
use clp_rust_utils::types::ArchiveId;
use clp_rust_utils::types::SessionToken;
use futures::Stream;
use futures::StreamExt;
use futures::channel::mpsc;
use query_result_listener::SessionEventError;
use query_result_listener::listener::ListenerConfig;
use query_result_listener::listener::ResultListener;
use query_result_listener::protocol::Ack;
use query_result_listener::protocol::PROTOCOL_VERSION;
use query_result_listener::session::SessionConfig;
use query_result_listener::session::SessionEvent;
use query_result_listener::session::SessionStats;
use query_result_listener::test_utils::ARCHIVE_ID;
use query_result_listener::test_utils::EncodingError;
use query_result_listener::test_utils::FallibleQueryResultEncoding;
use query_result_listener::test_utils::OTHER_ARCHIVE_ID;
use query_result_listener::test_utils::QueryResult;
use query_result_listener::test_utils::QueryResultEncoding;
use query_result_listener::test_utils::TIMEOUT;
use query_result_listener::test_utils::UNENCODABLE_MESSAGE;
use query_result_listener::test_utils::encode_handshake;
use query_result_listener::test_utils::encode_result;
use query_result_listener::test_utils::encode_results;
use query_result_listener::test_utils::encode_trailer;
use query_result_listener::test_utils::expected_results;
use query_result_listener::test_utils::timestamp;
use spider_core::task::TaskIndex;
use tokio::io::AsyncReadExt;
use tokio::io::AsyncWriteExt;
use tokio::net::TcpStream;
use tokio::sync::oneshot;
use tokio::task::yield_now;
use tokio::time::sleep;
use tokio::time::timeout;

const HANDSHAKE_TIMEOUT_SEC: NonZeroU64 = NonZeroU64::new(1).expect("1 is nonzero");

/// How long a test waits for the listener to read what an attempt has sent.
const SETTLE_DELAY: Duration = Duration::from_millis(100);

/// The error of a failure to wait for a query job's termination in the tests.
type TerminationError = std::io::Error;

/// An item of a session's event stream in the tests.
type TestEvent = SessionEvent<Vec<QueryResult>, TerminationError>;

/// A simulated attempt of a `clp-s` query task, streaming its results over TCP.
struct FakeClpS {
    stream: TcpStream,
    session_token: SessionToken,
}

impl FakeClpS {
    /// Connects to the listener that `network_output` names, as an attempt for its session.
    ///
    /// # Returns
    ///
    /// The connected attempt.
    ///
    /// # Panics
    ///
    /// Panics if the connection fails.
    async fn connect(network_output: &NetworkOutput) -> Self {
        let stream = TcpStream::connect((network_output.host.as_str(), network_output.port.get()))
            .await
            .expect("connecting to the listener should succeed");
        Self {
            stream,
            session_token: network_output.session_token,
        }
    }

    /// Sends `bytes` as they are.
    ///
    /// # Panics
    ///
    /// Panics if the write fails.
    async fn send(&mut self, bytes: &[u8]) {
        self.stream
            .write_all(bytes)
            .await
            .expect("sending to the listener should succeed");
    }

    /// Sends a handshake for task `task_index` of archive `archive_id`, presenting the token of the
    /// session that the attempt connected to.
    ///
    /// # Panics
    ///
    /// Panics if `task_index` doesn't fit in `u64`, or the write fails.
    async fn send_handshake(&mut self, task_index: TaskIndex, archive_id: &str) {
        self.send(&encode_handshake(
            PROTOCOL_VERSION,
            &self.session_token.to_string(),
            u64::try_from(task_index).expect("test task indices fit in `u64`"),
            archive_id,
        ))
        .await;
    }

    /// Sends the results at `result_indices` of task `task_index` in a single write.
    ///
    /// # Panics
    ///
    /// Panics if the write fails.
    async fn send_results(
        &mut self,
        task_index: TaskIndex,
        result_indices: impl IntoIterator<Item = u64>,
    ) {
        self.send(&encode_results(task_index, result_indices)).await;
    }

    /// Sends the results at `result_indices` of task `task_index` one at a time, until every result
    /// is sent or the listener has closed the connection.
    async fn send_results_until_closed(
        mut self,
        task_index: TaskIndex,
        result_indices: impl IntoIterator<Item = u64>,
    ) {
        for result_index in result_indices {
            if self
                .stream
                .write_all(&encode_results(task_index, [result_index]))
                .await
                .is_err()
            {
                return;
            }
        }
    }

    /// Ends the stream the way `clp-s` does: sends a trailer counting `num_results` results, then
    /// EOF, and waits until the listener closes the connection.
    ///
    /// # Returns
    ///
    /// The bytes the listener sent back.
    ///
    /// # Panics
    ///
    /// Panics if sending fails, or the listener doesn't close the connection within [`TIMEOUT`].
    async fn finish(mut self, num_results: u64) -> Vec<u8> {
        self.send(&encode_trailer(num_results)).await;
        self.stream
            .shutdown()
            .await
            .expect("sending EOF to the listener should succeed");
        self.read_until_closed().await
    }

    /// Waits until the listener closes the connection without sending anything.
    ///
    /// # Panics
    ///
    /// Panics if the listener sends anything, or doesn't close the connection within [`TIMEOUT`].
    async fn expect_closed_by_listener(self) {
        assert_eq!(
            self.read_until_closed().await,
            [] as [u8; 0],
            "the listener shouldn't acknowledge the connection"
        );
    }

    /// Waits until the listener closes the connection, either cleanly or with a reset.
    ///
    /// # Returns
    ///
    /// The bytes the listener sent before closing.
    ///
    /// # Panics
    ///
    /// Panics if the listener doesn't close the connection within [`TIMEOUT`], or reading fails
    /// with an error other than a reset.
    async fn read_until_closed(mut self) -> Vec<u8> {
        let mut received = Vec::new();
        match timeout(TIMEOUT, self.stream.read_to_end(&mut received))
            .await
            .expect("the listener should close the connection")
        {
            Ok(_) => received,
            // The listener resets the connection if it closes it before reading every byte sent.
            Err(e) if ErrorKind::ConnectionReset == e.kind() => received,
            Err(e) => panic!("reading from the listener failed: {e}"),
        }
    }
}

/// The items that a session's event stream yields from some point until its last item.
struct Ending {
    results: Vec<QueryResult>,
    last_event: TestEvent,
}

impl Ending {
    /// Receives the items of `events` until its end or failure, without polling it further.
    ///
    /// # Returns
    ///
    /// The results of the received batches, and the stream's last item.
    ///
    /// # Panics
    ///
    /// Panics if the stream doesn't yield its end or failure within [`TIMEOUT`], or ends without
    /// one.
    async fn receive(events: &mut (impl Stream<Item = TestEvent> + Unpin)) -> Self {
        let mut results = Vec::new();
        let last_event = loop {
            match next_event(events)
                .await
                .expect("the event stream shouldn't end without an end or a failure")
            {
                SessionEvent::Results(batch) => results.extend(batch),
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
    /// The status and statistics that the stream's last item carries.
    ///
    /// # Panics
    ///
    /// Panics if the stream's last item isn't an end.
    fn ended(&self) -> Ended {
        let SessionEvent::Ended { status, stats } = &self.last_event else {
            panic!(
                "the event stream should end with an end: {:?}",
                self.last_event
            );
        };
        Ended {
            status: *status,
            stats: *stats,
        }
    }

    /// # Returns
    ///
    /// The error that the stream's last item carries.
    ///
    /// # Panics
    ///
    /// Panics if the stream's last item isn't a failure.
    fn error(&self) -> &SessionEventError<TerminationError> {
        let SessionEvent::Failed(error) = &self.last_event else {
            panic!(
                "the event stream should end with a failure: {:?}",
                self.last_event
            );
        };
        error
    }
}

/// The fields of a [`SessionEvent::Ended`], to compare as a whole.
#[derive(Debug, Eq, PartialEq)]
struct Ended {
    status: QueryJobStatus,
    stats: SessionStats,
}

/// # Returns
///
/// The configuration of a loopback listener whose handshake timeout outlasts [`TIMEOUT`], so that
/// only a rejection closes a connection within a test's wait.
///
/// # Panics
///
/// Panics if the advertised host fixture is empty.
fn base_listener_config() -> ListenerConfig {
    use std::net::Ipv4Addr;
    use std::net::SocketAddr;

    use non_empty_string::NonEmptyString;

    ListenerConfig {
        bind_addr: SocketAddr::from((Ipv4Addr::LOCALHOST, 0)),
        advertised_host: NonEmptyString::new("127.0.0.1".to_owned())
            .expect("the advertised host fixture should be non-empty"),
        handshake_timeout_sec: NonZeroU64::new(6 * TIMEOUT.as_secs())
            .expect("the base handshake timeout should be nonzero"),
    }
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
    oneshot::Sender<Result<QueryJobStatus, TerminationError>>,
    impl Future<Output = Result<QueryJobStatus, TerminationError>> + Send + 'static,
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
/// * The receiver of the cancellation requests, which ends once the future has run or been dropped.
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
        .expect("the event stream should yield an item or end")
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
            Some(SessionEvent::Results(batch)) => received.extend(batch),
            event => panic!("the event stream should yield a batch: {event:?}"),
        }
    }
    received
}

/// Connects an attempt of task 0 of archive [`ARCHIVE_ID`] to the session that `network_output`
/// names, and sends its results at `0..2`, then those at `2..4` in a separate read, so that the
/// first batch fills a session's channel of capacity 1 that nothing consumes, and the attempt waits
/// to send the second.
///
/// # Returns
///
/// The connected attempt.
///
/// # Panics
///
/// Panics if connecting or sending fails.
async fn connect_attempt_blocked_on_its_second_batch(network_output: &NetworkOutput) -> FakeClpS {
    let mut attempt = FakeClpS::connect(network_output).await;
    attempt.send_handshake(0, ARCHIVE_ID).await;
    attempt.send_results(0, 0..2).await;
    sleep(SETTLE_DELAY).await;
    attempt.send_results(0, 2..4).await;
    sleep(SETTLE_DELAY).await;
    attempt
}

#[tokio::test]
async fn results_sent_along_with_the_handshake_are_delivered_and_acknowledged() -> Result<()> {
    let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config())?;
    let session = listener.open_session(SessionConfig::default())?;
    let network_output = session.network_output().clone();
    let (termination_sender, wait_for_termination) = wait_for_termination();
    let mut events = Box::pin(session.into_events(wait_for_termination, async {}));
    let session_token = network_output.session_token.to_string();

    let mut attempt = FakeClpS::connect(&network_output).await;
    attempt
        .send(
            &[
                encode_handshake(PROTOCOL_VERSION, &session_token, 0, ARCHIVE_ID),
                encode_results(0, 0..3),
            ]
            .concat(),
        )
        .await;
    let acknowledgement = attempt.finish(3).await;
    termination_sender
        .send(Ok(QueryJobStatus::Succeeded))
        .expect("the stream should wait for the job's termination");
    let ending = Ending::receive(&mut events).await;

    assert_eq!(acknowledgement, Ack { num_results: 3 }.encode());
    assert_eq!(ending.results, expected_results(0, ARCHIVE_ID, 0..3));
    assert_eq!(
        ending.ended(),
        Ended {
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
async fn unknown_session_token_is_rejected() -> Result<()> {
    use query_result_listener::test_utils::UNKNOWN_SESSION_TOKEN;

    let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config())?;
    let session = listener.open_session(SessionConfig::default())?;

    let mut stranger = FakeClpS::connect(session.network_output()).await;
    stranger
        .send(
            &[
                encode_handshake(PROTOCOL_VERSION, UNKNOWN_SESSION_TOKEN, 0, ARCHIVE_ID),
                encode_results(0, 0..3),
            ]
            .concat(),
        )
        .await;
    stranger.expect_closed_by_listener().await;
    Ok(())
}

#[tokio::test]
async fn malformed_handshake_is_rejected() -> Result<()> {
    let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config())?;
    let session = listener.open_session(SessionConfig::default())?;
    let mut not_an_array = Vec::new();
    rmp::encode::write_str(
        &mut not_an_array,
        &session.network_output().session_token.to_string(),
    )?;

    let mut attempt = FakeClpS::connect(session.network_output()).await;
    attempt
        .send(&[not_an_array, encode_results(0, 0..3)].concat())
        .await;
    attempt.expect_closed_by_listener().await;
    Ok(())
}

#[tokio::test]
async fn connection_without_a_handshake_is_closed_after_the_handshake_timeout() -> Result<()> {
    use std::time::Instant;

    let listener = ResultListener::<QueryResultEncoding>::bind(ListenerConfig {
        handshake_timeout_sec: HANDSHAKE_TIMEOUT_SEC,
        ..base_listener_config()
    })?;
    let session = listener.open_session(SessionConfig::default())?;

    let connected_at = Instant::now();
    let silent = FakeClpS::connect(session.network_output()).await;
    silent.expect_closed_by_listener().await;

    assert!(
        connected_at.elapsed() >= Duration::from_secs(HANDSHAKE_TIMEOUT_SEC.get()),
        "the listener shouldn't close the connection before the handshake timeout"
    );
    Ok(())
}

#[tokio::test]
async fn connection_that_sent_its_handshake_outlasts_the_handshake_timeout() -> Result<()> {
    let listener = ResultListener::<QueryResultEncoding>::bind(ListenerConfig {
        handshake_timeout_sec: HANDSHAKE_TIMEOUT_SEC,
        ..base_listener_config()
    })?;
    let session = listener.open_session(SessionConfig::default())?;
    let network_output = session.network_output().clone();
    let (termination_sender, wait_for_termination) = wait_for_termination();
    let mut events = Box::pin(session.into_events(wait_for_termination, async {}));

    let mut attempt = FakeClpS::connect(&network_output).await;
    attempt.send_handshake(0, ARCHIVE_ID).await;
    tokio::time::sleep(2 * Duration::from_secs(HANDSHAKE_TIMEOUT_SEC.get())).await;
    attempt.send_results(0, 0..3).await;
    let acknowledgement = attempt.finish(3).await;
    termination_sender
        .send(Ok(QueryJobStatus::Succeeded))
        .expect("the stream should wait for the job's termination");
    let ending = Ending::receive(&mut events).await;

    assert_eq!(acknowledgement, Ack { num_results: 3 }.encode());
    assert_eq!(ending.results, expected_results(0, ARCHIVE_ID, 0..3));
    assert_eq!(
        ending.ended(),
        Ended {
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
async fn dropping_a_session_unregisters_it() -> Result<()> {
    let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config())?;
    let session = listener.open_session(SessionConfig::default())?;
    let network_output = session.network_output().clone();
    drop(session);

    let mut late_attempt = FakeClpS::connect(&network_output).await;
    late_attempt
        .send(
            &[
                encode_handshake(
                    PROTOCOL_VERSION,
                    &network_output.session_token.to_string(),
                    0,
                    ARCHIVE_ID,
                ),
                encode_results(0, 0..3),
            ]
            .concat(),
        )
        .await;
    late_attempt.expect_closed_by_listener().await;
    Ok(())
}

#[tokio::test]
async fn sessions_on_one_listener_receive_only_their_own_results() -> Result<()> {
    let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config())?;
    let first_session = listener.open_session(SessionConfig::default())?;
    let first_network_output = first_session.network_output().clone();
    let (first_termination_sender, first_wait_for_termination) = wait_for_termination();
    let mut first_events =
        Box::pin(first_session.into_events(first_wait_for_termination, async {}));
    let second_session = listener.open_session(SessionConfig::default())?;
    let second_network_output = second_session.network_output().clone();
    let (second_termination_sender, second_wait_for_termination) = wait_for_termination();
    let mut second_events =
        Box::pin(second_session.into_events(second_wait_for_termination, async {}));

    let mut first_attempt = FakeClpS::connect(&first_network_output).await;
    first_attempt.send_handshake(0, ARCHIVE_ID).await;
    first_attempt.send_results(0, 0..3).await;
    let first_acknowledgement = first_attempt.finish(3).await;
    let mut second_attempt = FakeClpS::connect(&second_network_output).await;
    second_attempt.send_handshake(0, OTHER_ARCHIVE_ID).await;
    second_attempt.send_results(0, 0..2).await;
    let second_acknowledgement = second_attempt.finish(2).await;
    for termination_sender in [first_termination_sender, second_termination_sender] {
        termination_sender
            .send(Ok(QueryJobStatus::Succeeded))
            .expect("the stream should wait for the job's termination");
    }
    let first_ending = Ending::receive(&mut first_events).await;
    let second_ending = Ending::receive(&mut second_events).await;

    assert_eq!(first_acknowledgement, Ack { num_results: 3 }.encode());
    assert_eq!(second_acknowledgement, Ack { num_results: 2 }.encode());
    assert_eq!(first_ending.results, expected_results(0, ARCHIVE_ID, 0..3));
    assert_eq!(
        second_ending.results,
        expected_results(0, OTHER_ARCHIVE_ID, 0..2)
    );
    assert_eq!(
        first_ending.ended(),
        Ended {
            status: QueryJobStatus::Succeeded,
            stats: SessionStats {
                num_results_emitted: 3,
                num_duplicates_dropped: 0,
            },
        }
    );
    assert_eq!(
        second_ending.ended(),
        Ended {
            status: QueryJobStatus::Succeeded,
            stats: SessionStats {
                num_results_emitted: 2,
                num_duplicates_dropped: 0,
            },
        }
    );
    Ok(())
}

#[tokio::test]
async fn tasks_connecting_one_after_another_each_deliver_their_results_and_are_acknowledged()
-> Result<()> {
    let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config())?;
    let session = listener.open_session(SessionConfig::default())?;
    let network_output = session.network_output().clone();
    let (termination_sender, wait_for_termination) = wait_for_termination();
    let mut events = Box::pin(session.into_events(wait_for_termination, async {}));

    let mut first_task_attempt = FakeClpS::connect(&network_output).await;
    first_task_attempt.send_handshake(0, ARCHIVE_ID).await;
    first_task_attempt.send_results(0, 0..3).await;
    let first_task_acknowledgement = first_task_attempt.finish(3).await;
    let mut second_task_attempt = FakeClpS::connect(&network_output).await;
    second_task_attempt
        .send_handshake(1, OTHER_ARCHIVE_ID)
        .await;
    second_task_attempt.send_results(1, 0..2).await;
    let second_task_acknowledgement = second_task_attempt.finish(2).await;
    termination_sender
        .send(Ok(QueryJobStatus::Succeeded))
        .expect("the stream should wait for the job's termination");
    let ending = Ending::receive(&mut events).await;

    assert_eq!(first_task_acknowledgement, Ack { num_results: 3 }.encode());
    assert_eq!(second_task_acknowledgement, Ack { num_results: 2 }.encode());
    assert_eq!(
        ending.results,
        [
            expected_results(0, ARCHIVE_ID, 0..3),
            expected_results(1, OTHER_ARCHIVE_ID, 0..2),
        ]
        .concat()
    );
    assert_eq!(
        ending.ended(),
        Ended {
            status: QueryJobStatus::Succeeded,
            stats: SessionStats {
                num_results_emitted: 5,
                num_duplicates_dropped: 0,
            },
        }
    );
    Ok(())
}

#[tokio::test]
async fn concurrent_tasks_each_deliver_their_results_and_are_acknowledged() -> Result<()> {
    let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config())?;
    let session = listener.open_session(SessionConfig::default())?;
    let network_output = session.network_output().clone();
    let (termination_sender, wait_for_termination) = wait_for_termination();
    let mut events = Box::pin(session.into_events(wait_for_termination, async {}));

    let mut first_task_attempt = FakeClpS::connect(&network_output).await;
    first_task_attempt.send_handshake(0, ARCHIVE_ID).await;
    let mut second_task_attempt = FakeClpS::connect(&network_output).await;
    second_task_attempt
        .send_handshake(1, OTHER_ARCHIVE_ID)
        .await;
    first_task_attempt.send_results(0, 0..1).await;
    second_task_attempt.send_results(1, 0..2).await;
    first_task_attempt.send_results(0, 1..3).await;
    let second_task_acknowledgement = second_task_attempt.finish(2).await;
    let first_task_acknowledgement = first_task_attempt.finish(3).await;
    termination_sender
        .send(Ok(QueryJobStatus::Succeeded))
        .expect("the stream should wait for the job's termination");
    let ending = Ending::receive(&mut events).await;
    let first_task_archive_id = ARCHIVE_ID.parse::<ArchiveId>()?;
    let (first_task_results, second_task_results): (Vec<_>, Vec<_>) = ending
        .results
        .iter()
        .cloned()
        .partition(|result| first_task_archive_id == result.archive_id);

    assert_eq!(first_task_acknowledgement, Ack { num_results: 3 }.encode());
    assert_eq!(second_task_acknowledgement, Ack { num_results: 2 }.encode());
    assert_eq!(first_task_results, expected_results(0, ARCHIVE_ID, 0..3));
    assert_eq!(
        second_task_results,
        expected_results(1, OTHER_ARCHIVE_ID, 0..2)
    );
    assert_eq!(
        ending.ended(),
        Ended {
            status: QueryJobStatus::Succeeded,
            stats: SessionStats {
                num_results_emitted: 5,
                num_duplicates_dropped: 0,
            },
        }
    );
    Ok(())
}

#[tokio::test]
async fn retry_after_a_partial_attempt_delivers_only_the_missing_results_and_is_acknowledged()
-> Result<()> {
    let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config())?;
    let session = listener.open_session(SessionConfig::default())?;
    let network_output = session.network_output().clone();
    let (termination_sender, wait_for_termination) = wait_for_termination();
    let mut events = Box::pin(session.into_events(wait_for_termination, async {}));

    let mut first_attempt = FakeClpS::connect(&network_output).await;
    first_attempt.send_handshake(0, ARCHIVE_ID).await;
    first_attempt.send_results(0, 0..2).await;
    let mut received = receive_results(&mut events, 2).await;
    drop(first_attempt);
    let mut retry = FakeClpS::connect(&network_output).await;
    retry.send_handshake(0, ARCHIVE_ID).await;
    retry.send_results(0, 0..4).await;
    let acknowledgement = retry.finish(4).await;
    termination_sender
        .send(Ok(QueryJobStatus::Succeeded))
        .expect("the stream should wait for the job's termination");
    let ending = Ending::receive(&mut events).await;
    received.extend_from_slice(&ending.results);

    assert_eq!(acknowledgement, Ack { num_results: 4 }.encode());
    assert_eq!(received, expected_results(0, ARCHIVE_ID, 0..4));
    assert_eq!(
        ending.ended(),
        Ended {
            status: QueryJobStatus::Succeeded,
            stats: SessionStats {
                num_results_emitted: 4,
                num_duplicates_dropped: 2,
            },
        }
    );
    Ok(())
}

#[tokio::test]
async fn concurrent_attempts_of_one_task_deliver_each_result_once_and_are_both_acknowledged()
-> Result<()> {
    let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config())?;
    let session = listener.open_session(SessionConfig::default())?;
    let network_output = session.network_output().clone();
    let (termination_sender, wait_for_termination) = wait_for_termination();
    let mut events = Box::pin(session.into_events(wait_for_termination, async {}));

    let mut first_attempt = FakeClpS::connect(&network_output).await;
    first_attempt.send_handshake(0, ARCHIVE_ID).await;
    let mut second_attempt = FakeClpS::connect(&network_output).await;
    second_attempt.send_handshake(0, ARCHIVE_ID).await;
    first_attempt.send_results(0, 0..2).await;
    let mut received = receive_results(&mut events, 2).await;
    second_attempt.send_results(0, 0..3).await;
    received.extend(receive_results(&mut events, 1).await);
    first_attempt.send_results(0, 2..3).await;
    let first_acknowledgement = first_attempt.finish(3).await;
    let second_acknowledgement = second_attempt.finish(3).await;
    termination_sender
        .send(Ok(QueryJobStatus::Succeeded))
        .expect("the stream should wait for the job's termination");
    let ending = Ending::receive(&mut events).await;
    received.extend_from_slice(&ending.results);

    assert_eq!(first_acknowledgement, Ack { num_results: 3 }.encode());
    assert_eq!(second_acknowledgement, Ack { num_results: 3 }.encode());
    assert_eq!(received, expected_results(0, ARCHIVE_ID, 0..3));
    assert_eq!(
        ending.ended(),
        Ended {
            status: QueryJobStatus::Succeeded,
            stats: SessionStats {
                num_results_emitted: 3,
                num_duplicates_dropped: 3,
            },
        }
    );
    Ok(())
}

#[tokio::test]
async fn events_end_once_the_job_terminates_and_its_open_connection_is_cut_off() -> Result<()> {
    let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config())?;
    let session = listener.open_session(SessionConfig::default())?;
    let network_output = session.network_output().clone();
    let (termination_sender, wait_for_termination) = wait_for_termination();
    let mut events = Box::pin(session.into_events(wait_for_termination, async {}));
    let mut attempt = FakeClpS::connect(&network_output).await;
    attempt.send_handshake(0, ARCHIVE_ID).await;
    attempt.send_results(0, 0..2).await;
    let received = receive_results(&mut events, 2).await;

    termination_sender
        .send(Ok(QueryJobStatus::Succeeded))
        .expect("the stream should wait for the job's termination");
    let ending = Ending::receive(&mut events).await;
    attempt.expect_closed_by_listener().await;

    assert_eq!(received, expected_results(0, ARCHIVE_ID, 0..2));
    assert_eq!(ending.results, [] as [QueryResult; 0]);
    assert_eq!(
        ending.ended(),
        Ended {
            status: QueryJobStatus::Succeeded,
            stats: SessionStats {
                num_results_emitted: 2,
                num_duplicates_dropped: 0,
            },
        }
    );
    Ok(())
}

#[tokio::test]
async fn dropping_the_listener_stops_accepting_and_keeps_serving_open_connections() -> Result<()> {
    const RETRY_INTERVAL: Duration = Duration::from_millis(10);

    let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config())?;
    let session = listener.open_session(SessionConfig::default())?;
    let network_output = session.network_output().clone();
    let (termination_sender, wait_for_termination) = wait_for_termination();
    let mut events = Box::pin(session.into_events(wait_for_termination, async {}));
    let mut attempt = FakeClpS::connect(&network_output).await;
    attempt.send_handshake(0, ARCHIVE_ID).await;
    attempt.send_results(0, 0..2).await;
    let mut received = receive_results(&mut events, 2).await;

    drop(listener);
    let address = (network_output.host.as_str(), network_output.port.get());
    timeout(TIMEOUT, async {
        while TcpStream::connect(address).await.is_ok() {
            tokio::time::sleep(RETRY_INTERVAL).await;
        }
    })
    .await
    .expect("the listener should stop accepting connections");
    attempt.send_results(0, 2..4).await;
    let acknowledgement = attempt.finish(4).await;
    termination_sender
        .send(Ok(QueryJobStatus::Succeeded))
        .expect("the stream should wait for the job's termination");
    let ending = Ending::receive(&mut events).await;
    received.extend_from_slice(&ending.results);

    assert_eq!(acknowledgement, Ack { num_results: 4 }.encode());
    assert_eq!(received, expected_results(0, ARCHIVE_ID, 0..4));
    assert_eq!(
        ending.ended(),
        Ended {
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
async fn connection_streaming_past_its_jobs_termination_delivers_only_the_results_it_had_claimed()
-> Result<()> {
    use std::num::NonZeroUsize;

    const NUM_RESULTS: u64 = 1000;

    let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config())?;
    let session = listener.open_session(SessionConfig {
        channel_capacity: NonZeroUsize::MIN,
    })?;
    let network_output = session.network_output().clone();
    let (termination_sender, wait_for_termination) = wait_for_termination();
    let mut events = Box::pin(session.into_events(wait_for_termination, async {}));
    let attempt = connect_attempt_blocked_on_its_second_batch(&network_output).await;

    termination_sender
        .send(Ok(QueryJobStatus::Succeeded))
        .expect("the stream should wait for the job's termination");
    tokio::spawn(attempt.send_results_until_closed(0, 4..NUM_RESULTS));
    // Lets the results streamed past the job's termination reach the connection's socket before the
    // stream ends the session and makes room for the batch the connection waits to send.
    sleep(SETTLE_DELAY).await;
    let ending = Ending::receive(&mut events).await;

    assert_eq!(ending.results, expected_results(0, ARCHIVE_ID, 0..4));
    assert_eq!(
        ending.ended(),
        Ended {
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
async fn claimed_batch_of_a_blocked_attempt_is_delivered_after_its_retry_is_acknowledged()
-> Result<()> {
    use std::num::NonZeroUsize;

    let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config())?;
    let session = listener.open_session(SessionConfig {
        channel_capacity: NonZeroUsize::MIN,
    })?;
    let network_output = session.network_output().clone();
    let (termination_sender, wait_for_termination) = wait_for_termination();
    let mut events = Box::pin(session.into_events(wait_for_termination, async {}));
    let blocked_attempt = connect_attempt_blocked_on_its_second_batch(&network_output).await;
    let mut retry = FakeClpS::connect(&network_output).await;
    retry.send_handshake(0, ARCHIVE_ID).await;
    retry.send_results(0, 0..4).await;
    let retry_acknowledgement = retry.finish(4).await;

    termination_sender
        .send(Ok(QueryJobStatus::Succeeded))
        .expect("the stream should wait for the job's termination");
    let ending = Ending::receive(&mut events).await;
    blocked_attempt.expect_closed_by_listener().await;

    assert_eq!(retry_acknowledgement, Ack { num_results: 4 }.encode());
    assert_eq!(ending.results, expected_results(0, ARCHIVE_ID, 0..4));
    assert_eq!(
        ending.ended(),
        Ended {
            status: QueryJobStatus::Succeeded,
            stats: SessionStats {
                num_results_emitted: 4,
                num_duplicates_dropped: 4,
            },
        }
    );
    Ok(())
}

#[tokio::test]
async fn connection_waiting_for_eof_after_its_trailer_is_cut_off_once_its_session_ends()
-> Result<()> {
    let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config())?;
    let session = listener.open_session(SessionConfig::default())?;
    let network_output = session.network_output().clone();
    let (termination_sender, wait_for_termination) = wait_for_termination();
    let mut events = Box::pin(session.into_events(wait_for_termination, async {}));
    let mut attempt = FakeClpS::connect(&network_output).await;
    attempt.send_handshake(0, ARCHIVE_ID).await;
    // One write, so that the batch shows that the connection has read the trailer.
    attempt
        .send(&[encode_results(0, 0..2), encode_trailer(2)].concat())
        .await;
    receive_results(&mut events, 2).await;

    termination_sender
        .send(Ok(QueryJobStatus::Succeeded))
        .expect("the stream should wait for the job's termination");
    let ending = Ending::receive(&mut events).await;
    attempt.expect_closed_by_listener().await;

    assert_eq!(
        ending.ended(),
        Ended {
            status: QueryJobStatus::Succeeded,
            stats: SessionStats {
                num_results_emitted: 2,
                num_duplicates_dropped: 0,
            },
        }
    );
    Ok(())
}

#[tokio::test]
async fn dropping_a_session_does_not_cut_off_another_sessions_connection() -> Result<()> {
    let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config())?;
    let first_session = listener.open_session(SessionConfig::default())?;
    let second_session = listener.open_session(SessionConfig::default())?;
    let second_network_output = second_session.network_output().clone();
    let mut second_events = Box::pin(second_session.into_events(
        std::future::pending::<Result<QueryJobStatus, TerminationError>>(),
        async {},
    ));
    let mut attempt = FakeClpS::connect(&second_network_output).await;
    attempt.send_handshake(0, ARCHIVE_ID).await;
    attempt.send_results(0, 0..2).await;
    receive_results(&mut second_events, 2).await;

    drop(first_session);
    attempt.send_results(0, 2..4).await;
    let acknowledgement = attempt.finish(4).await;

    assert_eq!(acknowledgement, Ack { num_results: 4 }.encode());
    Ok(())
}

#[tokio::test]
async fn end_carries_the_statistics_of_all_the_sessions_connections() -> Result<()> {
    let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config())?;
    let session = listener.open_session(SessionConfig::default())?;
    let network_output = session.network_output().clone();
    let (termination_sender, wait_for_termination) = wait_for_termination();
    let mut events = Box::pin(session.into_events(wait_for_termination, async {}));
    let mut first_attempt = FakeClpS::connect(&network_output).await;
    first_attempt.send_handshake(0, ARCHIVE_ID).await;
    first_attempt.send_results(0, 0..3).await;
    first_attempt.finish(3).await;
    let mut retry = FakeClpS::connect(&network_output).await;
    retry.send_handshake(0, ARCHIVE_ID).await;
    retry.send_results(0, 0..5).await;
    retry.finish(5).await;

    termination_sender
        .send(Ok(QueryJobStatus::Succeeded))
        .expect("the stream should wait for the job's termination");
    let ending = Ending::receive(&mut events).await;

    assert_eq!(
        ending.ended(),
        Ended {
            status: QueryJobStatus::Succeeded,
            stats: SessionStats {
                num_results_emitted: 5,
                num_duplicates_dropped: 3,
            },
        }
    );
    Ok(())
}

#[tokio::test]
async fn encoding_failure_in_one_session_does_not_affect_another_session() -> Result<()> {
    let listener = ResultListener::<FallibleQueryResultEncoding>::bind(base_listener_config())?;
    let failing_session = listener.open_session(SessionConfig::default())?;
    let failing_network_output = failing_session.network_output().clone();
    let mut failing_events = Box::pin(failing_session.into_events(
        std::future::pending::<Result<QueryJobStatus, TerminationError>>(),
        async {},
    ));
    let other_session = listener.open_session(SessionConfig::default())?;
    let other_network_output = other_session.network_output().clone();
    let (termination_sender, wait_for_termination) = wait_for_termination();
    let mut other_events = Box::pin(other_session.into_events(wait_for_termination, async {}));
    let mut other_session_attempt = FakeClpS::connect(&other_network_output).await;
    other_session_attempt.send_handshake(0, ARCHIVE_ID).await;
    other_session_attempt.send_results(0, 0..2).await;
    let mut received = receive_results(&mut other_events, 2).await;

    let mut failing_attempt = FakeClpS::connect(&failing_network_output).await;
    failing_attempt.send_handshake(0, ARCHIVE_ID).await;
    failing_attempt
        .send(&encode_result(0, timestamp(0), UNENCODABLE_MESSAGE))
        .await;
    failing_attempt.expect_closed_by_listener().await;
    let failing_ending = Ending::receive(&mut failing_events).await;
    other_session_attempt.send_results(0, 2..3).await;
    let acknowledgement = other_session_attempt.finish(3).await;
    let mut late_attempt = FakeClpS::connect(&other_network_output).await;
    late_attempt.send_handshake(1, ARCHIVE_ID).await;
    late_attempt.send_results(1, 0..1).await;
    let late_acknowledgement = late_attempt.finish(1).await;
    termination_sender
        .send(Ok(QueryJobStatus::Succeeded))
        .expect("the stream should wait for the job's termination");
    // The stream checks for a session failure first, so it would end with one if the other session
    // had failed.
    let other_ending = Ending::receive(&mut other_events).await;
    received.extend_from_slice(&other_ending.results);

    assert!(
        matches!(failing_ending.error(), SessionEventError::Encoding(_)),
        "unexpected error: {:?}",
        failing_ending.error()
    );
    assert_eq!(acknowledgement, Ack { num_results: 3 }.encode());
    assert_eq!(late_acknowledgement, Ack { num_results: 1 }.encode());
    assert_eq!(
        received,
        [
            expected_results(0, ARCHIVE_ID, 0..3),
            expected_results(1, ARCHIVE_ID, 0..1),
        ]
        .concat()
    );
    assert_eq!(
        other_ending.ended(),
        Ended {
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
async fn healthy_session_does_not_fail() -> Result<()> {
    let listener = ResultListener::<FallibleQueryResultEncoding>::bind(base_listener_config())?;
    let session = listener.open_session(SessionConfig::default())?;
    let network_output = session.network_output().clone();
    let (termination_sender, wait_for_termination) = wait_for_termination();
    let mut events = Box::pin(session.into_events(wait_for_termination, async {}));
    let mut acknowledged_attempt = FakeClpS::connect(&network_output).await;
    acknowledged_attempt.send_handshake(0, ARCHIVE_ID).await;
    acknowledged_attempt.send_results(0, 0..3).await;
    let acknowledgement = acknowledged_attempt.finish(3).await;
    let mut rejected_attempt = FakeClpS::connect(&network_output).await;
    rejected_attempt.send_handshake(1, ARCHIVE_ID).await;
    rejected_attempt.send_results(1, [1]).await;
    rejected_attempt.expect_closed_by_listener().await;

    termination_sender
        .send(Ok(QueryJobStatus::Succeeded))
        .expect("the stream should wait for the job's termination");
    // The stream checks for a session failure first, so it would end with one if the session had
    // failed.
    let ending = Ending::receive(&mut events).await;

    assert_eq!(acknowledgement, Ack { num_results: 3 }.encode());
    assert_eq!(
        ending.ended(),
        Ended {
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
async fn results_are_forwarded_while_the_job_runs_and_the_end_carries_its_status_and_stats()
-> Result<()> {
    let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config())?;
    let session = listener.open_session(SessionConfig::default())?;
    let network_output = session.network_output().clone();
    let (termination_sender, wait_for_termination) = wait_for_termination();
    let mut events = Box::pin(session.into_events(wait_for_termination, async {}));
    let mut attempt = FakeClpS::connect(&network_output).await;
    attempt.send_handshake(0, ARCHIVE_ID).await;
    attempt.send_results(0, 0..3).await;
    let received = receive_results(&mut events, 3).await;
    attempt.finish(3).await;

    termination_sender
        .send(Ok(QueryJobStatus::Succeeded))
        .expect("the stream should wait for the job's termination");
    let ending = Ending::receive(&mut events).await;

    assert_eq!(received, expected_results(0, ARCHIVE_ID, 0..3));
    assert_eq!(ending.results, [] as [QueryResult; 0]);
    assert_eq!(
        ending.ended(),
        Ended {
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
async fn end_comes_only_after_a_batch_claimed_before_the_job_terminated_is_forwarded() -> Result<()>
{
    use std::num::NonZeroUsize;

    let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config())?;
    let session = listener.open_session(SessionConfig {
        channel_capacity: NonZeroUsize::MIN,
    })?;
    let network_output = session.network_output().clone();
    let (termination_sender, wait_for_termination) = wait_for_termination();
    let mut events = Box::pin(session.into_events(wait_for_termination, async {}));
    let _attempt = connect_attempt_blocked_on_its_second_batch(&network_output).await;
    termination_sender
        .send(Ok(QueryJobStatus::Succeeded))
        .expect("the stream should wait for the job's termination");

    let ending = Ending::receive(&mut events).await;

    assert_eq!(ending.results, expected_results(0, ARCHIVE_ID, 0..4));
    assert_eq!(
        ending.ended(),
        Ended {
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
    let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config())?;
    let session = listener.open_session(SessionConfig::default())?;
    let (wait_finished_sender, wait_finished) = oneshot::channel();
    let wait_for_termination = async move {
        for _ in 0..3 {
            yield_now().await;
        }
        wait_finished_sender
            .send(())
            .expect("the test should wait for the job's termination to finish");
        Ok::<_, TerminationError>(QueryJobStatus::Succeeded)
    };
    let _events = session.into_events(wait_for_termination, async {});

    timeout(TIMEOUT, wait_finished).await??;
    Ok(())
}

#[tokio::test]
async fn session_failure_takes_priority_over_a_terminal_status_and_requests_cancellation_once()
-> Result<()> {
    let listener = ResultListener::<FallibleQueryResultEncoding>::bind(base_listener_config())?;
    let session = listener.open_session(SessionConfig::default())?;
    let network_output = session.network_output().clone();
    let (termination_sender, wait_for_termination) = wait_for_termination();
    let (request_cancellation, cancellation_requests) = request_cancellation();
    let mut events = Box::pin(session.into_events(wait_for_termination, request_cancellation));
    let mut failing_attempt = FakeClpS::connect(&network_output).await;
    failing_attempt.send_handshake(0, ARCHIVE_ID).await;
    failing_attempt
        .send(
            &[
                encode_results(0, 0..2),
                encode_result(2, timestamp(2), UNENCODABLE_MESSAGE),
                encode_trailer(3),
            ]
            .concat(),
        )
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
        timestamp_millisecs: timestamp(2),
    };
    assert!(
        matches!(
            ending.error(),
            SessionEventError::Encoding(message) if expected_error.to_string() == *message
        ),
        "unexpected error: {:?}",
        ending.error()
    );
    // Counted while the stream is alive, so that only the failure can have requested the
    // cancellation.
    assert_eq!(timeout(TIMEOUT, cancellation_requests.count()).await?, 1);
    Ok(())
}

#[tokio::test]
async fn failed_termination_ends_the_stream_with_its_error_and_requests_cancellation_once()
-> Result<()> {
    use std::num::NonZeroUsize;

    const TERMINATION_ERROR: &str = "waiting for the job's termination failed";

    let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config())?;
    let session = listener.open_session(SessionConfig {
        channel_capacity: NonZeroUsize::MIN,
    })?;
    let network_output = session.network_output().clone();
    let (termination_sender, wait_for_termination) = wait_for_termination();
    let (request_cancellation, cancellation_requests) = request_cancellation();
    let mut events = Box::pin(session.into_events(wait_for_termination, request_cancellation));
    let attempt = connect_attempt_blocked_on_its_second_batch(&network_output).await;
    termination_sender
        .send(Err(TerminationError::other(TERMINATION_ERROR)))
        .expect("the stream should wait for the job's termination");
    // Lets the task waiting for the job's termination finish before the stream is polled.
    yield_now().await;

    let ending = Ending::receive(&mut events).await;
    attempt.expect_closed_by_listener().await;

    assert_eq!(ending.results, [] as [QueryResult; 0]);
    assert!(
        matches!(
            ending.error(),
            SessionEventError::Termination(error) if TERMINATION_ERROR == error.to_string()
        ),
        "unexpected error: {:?}",
        ending.error()
    );
    // Counted while the stream is alive, so that only the failure can have requested the
    // cancellation.
    assert_eq!(timeout(TIMEOUT, cancellation_requests.count()).await?, 1);
    Ok(())
}

#[tokio::test]
async fn panicked_termination_ends_the_stream_with_a_join_error_and_requests_cancellation_once()
-> Result<()> {
    let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config())?;
    let session = listener.open_session(SessionConfig::default())?;
    let (request_cancellation, cancellation_requests) = request_cancellation();
    let mut events = Box::pin(session.into_events(
        async { panic!("the test's wait for the job's termination panics") },
        request_cancellation,
    ));

    let ending = Ending::receive(&mut events).await;

    assert!(
        matches!(ending.error(), SessionEventError::TerminationPanicked(error) if error.is_panic()),
        "unexpected error: {:?}",
        ending.error()
    );
    // Counted while the stream is alive, so that only the failure can have requested the
    // cancellation.
    assert_eq!(timeout(TIMEOUT, cancellation_requests.count()).await?, 1);
    Ok(())
}

#[tokio::test]
async fn dropping_the_stream_before_its_end_requests_cancellation_once_and_ends_the_session()
-> Result<()> {
    let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config())?;
    let session = listener.open_session(SessionConfig::default())?;
    let network_output = session.network_output().clone();
    let (request_cancellation, cancellation_requests) = request_cancellation();
    let events = session.into_events(
        std::future::pending::<Result<QueryJobStatus, TerminationError>>(),
        request_cancellation,
    );
    let mut attempt = FakeClpS::connect(&network_output).await;
    attempt.send_handshake(0, ARCHIVE_ID).await;
    // Lets the listener join the attempt to the session while the job runs.
    sleep(SETTLE_DELAY).await;

    drop(events);
    attempt.expect_closed_by_listener().await;

    assert_eq!(timeout(TIMEOUT, cancellation_requests.count()).await?, 1);
    Ok(())
}

#[tokio::test]
async fn dropping_the_stream_while_the_job_runs_stops_waiting_for_termination() -> Result<()> {
    let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config())?;
    let session = listener.open_session(SessionConfig::default())?;
    let (mut termination_sender, wait_for_termination) = wait_for_termination();
    let events = session.into_events(wait_for_termination, async {});

    drop(events);

    timeout(TIMEOUT, termination_sender.closed()).await?;
    Ok(())
}

#[tokio::test]
async fn dropping_the_stream_while_it_drains_after_the_job_terminated_requests_nothing()
-> Result<()> {
    use std::num::NonZeroUsize;

    let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config())?;
    let session = listener.open_session(SessionConfig {
        channel_capacity: NonZeroUsize::MIN,
    })?;
    let network_output = session.network_output().clone();
    let (termination_sender, wait_for_termination) = wait_for_termination();
    let (request_cancellation, cancellation_requests) = request_cancellation();
    let mut events = Box::pin(session.into_events(wait_for_termination, request_cancellation));
    let _attempt = connect_attempt_blocked_on_its_second_batch(&network_output).await;
    termination_sender
        .send(Ok(QueryJobStatus::Succeeded))
        .expect("the stream should wait for the job's termination");
    // Lets the task waiting for the job's termination finish before the stream is polled.
    yield_now().await;
    // The stream finds the job terminated before it forwards the first batch, and then waits for
    // the second.
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
        let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config())?;
        let session = listener.open_session(SessionConfig::default())?;
        session.into_events(
            std::future::pending::<Result<QueryJobStatus, TerminationError>>(),
            request_cancellation,
        )
    };
    drop(runtime);

    drop(events);

    assert_eq!(cancellation_requests.count().now_or_never(), Some(0));
    Ok(())
}

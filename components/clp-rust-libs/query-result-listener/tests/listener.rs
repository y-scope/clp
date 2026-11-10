use std::io::ErrorKind;
use std::num::NonZeroU64;
use std::time::Duration;

use anyhow::Result;
use clp_rust_utils::job_config::NetworkOutput;
use clp_rust_utils::types::SessionToken;
use futures::StreamExt;
use query_result_listener::listener::ListenerConfig;
use query_result_listener::listener::ResultListener;
use query_result_listener::protocol::Ack;
use query_result_listener::protocol::PROTOCOL_VERSION;
use query_result_listener::session::ResultStream;
use query_result_listener::session::SessionConfig;
use query_result_listener::test_utils::ARCHIVE_ID;
use query_result_listener::test_utils::OTHER_ARCHIVE_ID;
use query_result_listener::test_utils::QueryResult;
use query_result_listener::test_utils::QueryResultEncoding;
use query_result_listener::test_utils::TIMEOUT;
use query_result_listener::test_utils::encode_handshake;
use query_result_listener::test_utils::encode_results;
use query_result_listener::test_utils::encode_trailer;
use query_result_listener::test_utils::expected_results;
use spider_core::task::TaskIndex;
use tokio::io::AsyncReadExt;
use tokio::io::AsyncWriteExt;
use tokio::net::TcpStream;
use tokio::time::timeout;

const HANDSHAKE_TIMEOUT_SEC: NonZeroU64 = NonZeroU64::new(1).expect("1 is nonzero");

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

/// Receives batches from `results` until they hold `num_results` results.
///
/// # Returns
///
/// The received results.
///
/// # Panics
///
/// Panics if the stream ends, or the results don't arrive within [`TIMEOUT`].
async fn receive_results(
    results: &mut ResultStream<Vec<QueryResult>>,
    num_results: usize,
) -> Vec<QueryResult> {
    let mut received = Vec::new();
    while received.len() < num_results {
        received.extend(
            timeout(TIMEOUT, results.next())
                .await
                .expect("a batch should arrive")
                .expect("the result stream shouldn't end yet"),
        );
    }
    received
}

/// # Returns
///
/// The results of the batches that `results` yields until it ends.
///
/// # Panics
///
/// Panics if the stream doesn't end within [`TIMEOUT`].
async fn collect_results(results: &mut ResultStream<Vec<QueryResult>>) -> Vec<QueryResult> {
    timeout(TIMEOUT, results.concat())
        .await
        .expect("the result stream should end")
}

/// # Returns
///
/// The results of the batches that `results` holds already.
fn take_buffered_results(results: &mut ResultStream<Vec<QueryResult>>) -> Vec<QueryResult> {
    use futures::FutureExt;

    let mut taken = Vec::new();
    while let Some(Some(batch)) = results.next().now_or_never() {
        taken.extend(batch);
    }
    taken
}

#[tokio::test]
async fn results_sent_along_with_the_handshake_are_delivered_and_acknowledged() -> Result<()> {
    let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config())?;
    let (session, mut results) = listener.open_session(SessionConfig::default())?;
    let session_token = session.network_output().session_token.to_string();

    let mut attempt = FakeClpS::connect(session.network_output()).await;
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

    assert_eq!(acknowledgement, Ack { num_results: 3 }.encode());
    assert_eq!(
        take_buffered_results(&mut results),
        expected_results(0, ARCHIVE_ID, 0..3)
    );
    Ok(())
}

#[tokio::test]
async fn unknown_session_token_is_rejected() -> Result<()> {
    use query_result_listener::test_utils::UNKNOWN_SESSION_TOKEN;

    let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config())?;
    let (session, _results) = listener.open_session(SessionConfig::default())?;

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
    let (session, _results) = listener.open_session(SessionConfig::default())?;
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
    let (session, _results) = listener.open_session(SessionConfig::default())?;

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
    let (session, mut results) = listener.open_session(SessionConfig::default())?;

    let mut attempt = FakeClpS::connect(session.network_output()).await;
    attempt.send_handshake(0, ARCHIVE_ID).await;
    tokio::time::sleep(2 * Duration::from_secs(HANDSHAKE_TIMEOUT_SEC.get())).await;
    attempt.send_results(0, 0..3).await;
    let acknowledgement = attempt.finish(3).await;

    assert_eq!(acknowledgement, Ack { num_results: 3 }.encode());
    assert_eq!(
        take_buffered_results(&mut results),
        expected_results(0, ARCHIVE_ID, 0..3)
    );
    Ok(())
}

#[tokio::test]
async fn dropping_a_session_unregisters_it() -> Result<()> {
    let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config())?;
    let (session, _results) = listener.open_session(SessionConfig::default())?;
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
    let (first_session, mut first_results) = listener.open_session(SessionConfig::default())?;
    let (second_session, mut second_results) = listener.open_session(SessionConfig::default())?;

    let mut first_attempt = FakeClpS::connect(first_session.network_output()).await;
    first_attempt.send_handshake(0, ARCHIVE_ID).await;
    first_attempt.send_results(0, 0..3).await;
    let first_acknowledgement = first_attempt.finish(3).await;
    let mut second_attempt = FakeClpS::connect(second_session.network_output()).await;
    second_attempt.send_handshake(0, OTHER_ARCHIVE_ID).await;
    second_attempt.send_results(0, 0..2).await;
    let second_acknowledgement = second_attempt.finish(2).await;

    assert_eq!(first_acknowledgement, Ack { num_results: 3 }.encode());
    assert_eq!(second_acknowledgement, Ack { num_results: 2 }.encode());
    assert_eq!(
        take_buffered_results(&mut first_results),
        expected_results(0, ARCHIVE_ID, 0..3)
    );
    assert_eq!(
        take_buffered_results(&mut second_results),
        expected_results(0, OTHER_ARCHIVE_ID, 0..2)
    );
    Ok(())
}

#[tokio::test]
async fn tasks_connecting_one_after_another_each_deliver_their_results_and_are_acknowledged()
-> Result<()> {
    let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config())?;
    let (session, mut results) = listener.open_session(SessionConfig::default())?;

    let mut first_task_attempt = FakeClpS::connect(session.network_output()).await;
    first_task_attempt.send_handshake(0, ARCHIVE_ID).await;
    first_task_attempt.send_results(0, 0..3).await;
    let first_task_acknowledgement = first_task_attempt.finish(3).await;
    let mut second_task_attempt = FakeClpS::connect(session.network_output()).await;
    second_task_attempt
        .send_handshake(1, OTHER_ARCHIVE_ID)
        .await;
    second_task_attempt.send_results(1, 0..2).await;
    let second_task_acknowledgement = second_task_attempt.finish(2).await;

    assert_eq!(first_task_acknowledgement, Ack { num_results: 3 }.encode());
    assert_eq!(second_task_acknowledgement, Ack { num_results: 2 }.encode());
    assert_eq!(
        take_buffered_results(&mut results),
        [
            expected_results(0, ARCHIVE_ID, 0..3),
            expected_results(1, OTHER_ARCHIVE_ID, 0..2),
        ]
        .concat()
    );
    Ok(())
}

#[tokio::test]
async fn concurrent_tasks_each_deliver_their_results_and_are_acknowledged() -> Result<()> {
    use clp_rust_utils::types::ArchiveId;

    let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config())?;
    let (session, mut results) = listener.open_session(SessionConfig::default())?;

    let mut first_task_attempt = FakeClpS::connect(session.network_output()).await;
    first_task_attempt.send_handshake(0, ARCHIVE_ID).await;
    let mut second_task_attempt = FakeClpS::connect(session.network_output()).await;
    second_task_attempt
        .send_handshake(1, OTHER_ARCHIVE_ID)
        .await;
    first_task_attempt.send_results(0, 0..1).await;
    second_task_attempt.send_results(1, 0..2).await;
    first_task_attempt.send_results(0, 1..3).await;
    let second_task_acknowledgement = second_task_attempt.finish(2).await;
    let first_task_acknowledgement = first_task_attempt.finish(3).await;
    let first_task_archive_id = ARCHIVE_ID.parse::<ArchiveId>()?;
    let (first_task_results, second_task_results): (Vec<_>, Vec<_>) =
        take_buffered_results(&mut results)
            .into_iter()
            .partition(|result| first_task_archive_id == result.archive_id);

    assert_eq!(first_task_acknowledgement, Ack { num_results: 3 }.encode());
    assert_eq!(second_task_acknowledgement, Ack { num_results: 2 }.encode());
    assert_eq!(first_task_results, expected_results(0, ARCHIVE_ID, 0..3));
    assert_eq!(
        second_task_results,
        expected_results(1, OTHER_ARCHIVE_ID, 0..2)
    );
    Ok(())
}

#[tokio::test]
async fn retry_after_a_partial_attempt_delivers_only_the_missing_results_and_is_acknowledged()
-> Result<()> {
    let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config())?;
    let (session, mut results) = listener.open_session(SessionConfig::default())?;

    let mut first_attempt = FakeClpS::connect(session.network_output()).await;
    first_attempt.send_handshake(0, ARCHIVE_ID).await;
    first_attempt.send_results(0, 0..2).await;
    let mut received = receive_results(&mut results, 2).await;
    drop(first_attempt);
    let mut retry = FakeClpS::connect(session.network_output()).await;
    retry.send_handshake(0, ARCHIVE_ID).await;
    retry.send_results(0, 0..4).await;
    let acknowledgement = retry.finish(4).await;
    received.extend(take_buffered_results(&mut results));

    assert_eq!(acknowledgement, Ack { num_results: 4 }.encode());
    assert_eq!(received, expected_results(0, ARCHIVE_ID, 0..4));
    Ok(())
}

#[tokio::test]
async fn concurrent_attempts_of_one_task_deliver_each_result_once_and_are_both_acknowledged()
-> Result<()> {
    let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config())?;
    let (session, mut results) = listener.open_session(SessionConfig::default())?;

    let mut first_attempt = FakeClpS::connect(session.network_output()).await;
    first_attempt.send_handshake(0, ARCHIVE_ID).await;
    let mut second_attempt = FakeClpS::connect(session.network_output()).await;
    second_attempt.send_handshake(0, ARCHIVE_ID).await;
    first_attempt.send_results(0, 0..2).await;
    let mut received = receive_results(&mut results, 2).await;
    second_attempt.send_results(0, 0..3).await;
    received.extend(receive_results(&mut results, 1).await);
    first_attempt.send_results(0, 2..3).await;
    let first_acknowledgement = first_attempt.finish(3).await;
    let second_acknowledgement = second_attempt.finish(3).await;
    received.extend(take_buffered_results(&mut results));

    assert_eq!(first_acknowledgement, Ack { num_results: 3 }.encode());
    assert_eq!(second_acknowledgement, Ack { num_results: 3 }.encode());
    assert_eq!(received, expected_results(0, ARCHIVE_ID, 0..3));
    Ok(())
}

#[tokio::test]
async fn result_stream_ends_once_its_session_is_dropped_and_its_open_connection_is_cut_off()
-> Result<()> {
    let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config())?;
    let (session, mut results) = listener.open_session(SessionConfig::default())?;
    let mut attempt = FakeClpS::connect(session.network_output()).await;
    attempt.send_handshake(0, ARCHIVE_ID).await;
    attempt.send_results(0, 0..2).await;
    let mut received = receive_results(&mut results, 2).await;

    drop(session);
    received.extend(collect_results(&mut results).await);
    attempt.expect_closed_by_listener().await;

    assert_eq!(received, expected_results(0, ARCHIVE_ID, 0..2));
    Ok(())
}

#[tokio::test]
async fn dropping_the_listener_stops_accepting_and_keeps_serving_open_connections() -> Result<()> {
    use tokio::net::TcpStream;

    const RETRY_INTERVAL: Duration = Duration::from_millis(10);

    let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config())?;
    let (session, mut results) = listener.open_session(SessionConfig::default())?;
    let network_output = session.network_output();
    let mut attempt = FakeClpS::connect(network_output).await;
    attempt.send_handshake(0, ARCHIVE_ID).await;
    attempt.send_results(0, 0..2).await;
    let mut received = receive_results(&mut results, 2).await;

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
    received.extend(take_buffered_results(&mut results));

    assert_eq!(acknowledgement, Ack { num_results: 4 }.encode());
    assert_eq!(received, expected_results(0, ARCHIVE_ID, 0..4));
    Ok(())
}

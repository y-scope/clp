//! The TCP listener that accepts connections from query tasks and routes each one to its session.

use std::net::SocketAddr;
use std::num::NonZeroU16;
use std::num::NonZeroU64;
use std::sync::Arc;
use std::time::Duration;

use non_empty_string::NonEmptyString;
use tokio::net::TcpListener;
use tokio::task::JoinHandle;

use crate::connection;
use crate::encoding::ResultEncoding;
use crate::session::Registry;
use crate::session::ResultStream;
use crate::session::Session;
use crate::session::SessionConfig;

/// Configuration of a listener.
#[derive(Clone, Debug, Eq, PartialEq)]
pub struct ListenerConfig {
    /// The address to listen on.
    pub bind_addr: SocketAddr,

    /// The host that query tasks connect to.
    pub advertised_host: NonEmptyString,

    /// How long a new connection may take to send its handshake, in seconds.
    pub handshake_timeout_sec: NonZeroU64,
}

/// A listener that receives the results query tasks stream, and routes each connection to the
/// session that its handshake names.
///
/// Dropping the listener stops accepting connections. The connections already being served keep
/// running.
///
/// # Type Parameters
///
/// * `EncodingType` - The encoding of the results the listener's sessions deliver.
pub struct ResultListener<EncodingType: ResultEncoding> {
    sessions: Arc<Registry<EncodingType>>,
    advertised_host: NonEmptyString,
    port: NonZeroU16,
    accept_loop: JoinHandle<()>,
}

impl<EncodingType: ResultEncoding> ResultListener<EncodingType> {
    /// Factory function.
    ///
    /// Binds a listener to `config.bind_addr` and starts accepting connections in a background
    /// task.
    ///
    /// # Returns
    ///
    /// The newly bound listener on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`TcpListener::bind`]'s return values on failure.
    /// * Forwards [`TcpListener::local_addr`]'s return values on failure.
    ///
    /// # Panics
    ///
    /// Panics if the bound port is 0, which the OS never assigns.
    pub async fn bind(config: ListenerConfig) -> Result<Self, std::io::Error> {
        let tcp_listener = TcpListener::bind(config.bind_addr).await?;
        let port =
            NonZeroU16::new(tcp_listener.local_addr()?.port()).expect("a bound port is nonzero");
        let sessions = Arc::new(Registry::new());
        let accept_loop = tokio::spawn(accept_connections(
            tcp_listener,
            Arc::clone(&sessions),
            Duration::from_secs(config.handshake_timeout_sec.get()),
        ));
        Ok(Self {
            sessions,
            advertised_host: config.advertised_host,
            port,
            accept_loop,
        })
    }

    /// Opens a session for one query job, registered before this method returns.
    ///
    /// # Returns
    ///
    /// A tuple containing:
    ///
    /// * The newly opened session.
    /// * The stream of the session's results.
    ///
    /// # Panics
    ///
    /// Panics if the session's fresh token is already registered, which a random UUID rules out.
    #[must_use]
    pub fn open_session(
        &self,
        config: SessionConfig,
    ) -> (Session<EncodingType>, ResultStream<EncodingType::Batch>) {
        Session::open(
            Arc::clone(&self.sessions),
            self.advertised_host.clone(),
            self.port,
            config,
        )
    }
}

impl<EncodingType: ResultEncoding> Drop for ResultListener<EncodingType> {
    fn drop(&mut self) {
        self.accept_loop.abort();
    }
}

/// The delay before accepting again after a failed accept, so that a persistent failure such as
/// running out of file descriptors doesn't spin.
const ACCEPT_RETRY_DELAY: Duration = Duration::from_millis(100);

/// Accepts connections on `tcp_listener` and serves each one in its own task.
///
/// # Type Parameters
///
/// * `EncodingType` - The encoding of the results the sessions deliver.
async fn accept_connections<EncodingType: ResultEncoding>(
    tcp_listener: TcpListener,
    sessions: Arc<Registry<EncodingType>>,
    handshake_timeout: Duration,
) {
    loop {
        match tcp_listener.accept().await {
            Ok((stream, peer_addr)) => {
                tokio::spawn(connection::serve(
                    stream,
                    peer_addr,
                    Arc::clone(&sessions),
                    handshake_timeout,
                ));
            }
            Err(e) => {
                tracing::warn!(error = % e, "Failed to accept a connection.");
                tokio::time::sleep(ACCEPT_RETRY_DELAY).await;
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use std::num::NonZeroU64;
    use std::time::Duration;

    use futures::StreamExt;
    use tokio::time::timeout;

    use super::ListenerConfig;
    use super::ResultListener;
    use crate::protocol::Ack;
    use crate::protocol::PROTOCOL_VERSION;
    use crate::session::ResultStream;
    use crate::session::SessionConfig;
    use crate::test_utils::FakeClpS;
    use crate::test_utils::QueryResult;
    use crate::test_utils::QueryResultEncoding;
    use crate::test_utils::TIMEOUT;
    use crate::test_utils::encode_handshake;
    use crate::test_utils::encode_results;
    use crate::test_utils::expected_results;

    const ARCHIVE_ID: &str = "018e90e5-8b2a-4a61-a2fc-cac799936caf";
    const HANDSHAKE_TIMEOUT_SEC: NonZeroU64 = NonZeroU64::new(1).expect("1 is nonzero");

    /// # Returns
    ///
    /// The configuration of a loopback listener whose handshake timeout outlasts [`TIMEOUT`], so
    /// that only a rejection closes a connection within a test's wait.
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
    async fn results_sent_along_with_the_handshake_are_delivered_and_acknowledged()
    -> anyhow::Result<()> {
        let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config()).await?;
        let (session, mut results) = listener.open_session(SessionConfig::default());
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
    async fn unknown_session_token_is_rejected() -> anyhow::Result<()> {
        const UNKNOWN_SESSION_TOKEN: &str = "6f1d3b52-8a4e-4c1b-9f6e-2d7a5c0b9e13";

        let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config()).await?;
        let (session, _results) = listener.open_session(SessionConfig::default());

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
    async fn malformed_handshake_is_rejected() -> anyhow::Result<()> {
        let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config()).await?;
        let (session, _results) = listener.open_session(SessionConfig::default());
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
    async fn connection_without_a_handshake_is_closed_after_the_handshake_timeout()
    -> anyhow::Result<()> {
        use std::time::Instant;

        let listener = ResultListener::<QueryResultEncoding>::bind(ListenerConfig {
            handshake_timeout_sec: HANDSHAKE_TIMEOUT_SEC,
            ..base_listener_config()
        })
        .await?;
        let (session, _results) = listener.open_session(SessionConfig::default());

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
    async fn connection_that_sent_its_handshake_outlasts_the_handshake_timeout()
    -> anyhow::Result<()> {
        let listener = ResultListener::<QueryResultEncoding>::bind(ListenerConfig {
            handshake_timeout_sec: HANDSHAKE_TIMEOUT_SEC,
            ..base_listener_config()
        })
        .await?;
        let (session, mut results) = listener.open_session(SessionConfig::default());

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
    async fn dropping_a_session_unregisters_it() -> anyhow::Result<()> {
        let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config()).await?;
        let (session, _results) = listener.open_session(SessionConfig::default());
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
    async fn sessions_on_one_listener_receive_only_their_own_results() -> anyhow::Result<()> {
        const OTHER_ARCHIVE_ID: &str = "4b0c2f8e-6d13-4f4e-9a57-1c2d3e4f5a6b";

        let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config()).await?;
        let (first_session, mut first_results) = listener.open_session(SessionConfig::default());
        let (second_session, mut second_results) = listener.open_session(SessionConfig::default());

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
    async fn result_stream_ends_once_its_session_is_dropped_and_its_connections_have_finished()
    -> anyhow::Result<()> {
        const STREAM_OPEN_CHECK_DURATION: Duration = Duration::from_millis(100);

        let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config()).await?;
        let (session, mut results) = listener.open_session(SessionConfig::default());
        let mut attempt = FakeClpS::connect(session.network_output()).await;
        attempt.send_handshake(0, ARCHIVE_ID).await;
        attempt.send_results(0, 0..2).await;
        let mut received = receive_results(&mut results, 2).await;

        drop(session);
        let stream_open_check = timeout(STREAM_OPEN_CHECK_DURATION, results.next()).await;
        attempt.send_results(0, 2..4).await;
        let acknowledgement = attempt.finish(4).await;
        received.extend(
            timeout(TIMEOUT, results.concat())
                .await
                .expect("the result stream should end"),
        );

        assert!(
            stream_open_check.is_err(),
            "the result stream shouldn't end or yield while its connection is open: \
             {stream_open_check:?}"
        );
        assert_eq!(acknowledgement, Ack { num_results: 4 }.encode());
        assert_eq!(received, expected_results(0, ARCHIVE_ID, 0..4));
        Ok(())
    }

    #[tokio::test]
    async fn dropping_the_listener_stops_accepting_and_keeps_serving_open_connections()
    -> anyhow::Result<()> {
        use tokio::net::TcpStream;

        const RETRY_INTERVAL: Duration = Duration::from_millis(10);

        let listener = ResultListener::<QueryResultEncoding>::bind(base_listener_config()).await?;
        let (session, mut results) = listener.open_session(SessionConfig::default());
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
}

//! The registry of the sessions open on a listener, which joins each connection to its session.

use std::num::NonZeroU16;
use std::sync::Arc;

use clp_rust_utils::job_config::NetworkOutput;
use clp_rust_utils::types::SessionToken;
use dashmap::DashMap;
use dashmap::Entry;
use non_empty_string::NonEmptyString;
use tokio::sync::mpsc;

use crate::encoding::ResultEncoding;
use crate::error::ConnectionError;
use crate::error::SessionError;
use crate::protocol::Handshake;
use crate::session::OpenSession;
use crate::session::SessionConfig;
use crate::session::connection::Connection;
use crate::session::open_session::ResultStream;
use crate::session::state::State;
use crate::session::stats::AtomicSessionStats;

/// The sessions registered on a listener, keyed by their tokens.
///
/// # Type Parameters
///
/// * `EncodingType` - The encoding of the results the sessions deliver.
pub struct SessionRegistry<EncodingType: ResultEncoding> {
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
    /// The newly opened session on success.
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
    ) -> Result<OpenSession<EncodingType>, SessionError> {
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
        Ok(OpenSession {
            session,
            results: ResultStream {
                batches_receiver,
                stats,
            },
        })
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
    pub(super) fn join(
        &self,
        handshake: &Handshake,
    ) -> Result<Connection<EncodingType>, ConnectionError> {
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
pub(super) struct Session<EncodingType: ResultEncoding> {
    _registration: Registration<EncodingType>,
    network_output: NetworkOutput,
    state: Arc<State<EncodingType>>,
}

impl<EncodingType: ResultEncoding> Session<EncodingType> {
    /// # Returns
    ///
    /// The network output that directs a query job's tasks to stream their results to this session.
    #[must_use]
    pub(super) const fn network_output(&self) -> &NetworkOutput {
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
    pub(super) async fn failed(&self) -> &EncodingType::Error {
        self.state.end_signal.cancelled().await;
        self.state
            .encoding_error()
            .expect("a session's end signal is cancelled only once it fails or is dropped")
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
    use super::SessionRegistry;
    use crate::encoding::ResultEncoding;
    use crate::error::ConnectionError;
    use crate::error::ProtocolError;
    use crate::protocol::Handshake;
    use crate::session::OpenSession;
    use crate::session::SessionConfig;
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

        let OpenSession { session, .. } = session_registry
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

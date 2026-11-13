//! The TCP listener that accepts connections from query tasks and routes each one to its session.

use std::net::SocketAddr;
use std::num::NonZeroU16;
use std::num::NonZeroU64;
use std::sync::Arc;
use std::time::Duration;

use non_empty_string::NonEmptyString;
use tokio::net::TcpListener;
use tokio::net::TcpSocket;
use tokio::task::JoinHandle;

use crate::connection;
use crate::encoding::ResultEncoding;
use crate::error::SessionError;
use crate::session::OpenSession;
use crate::session::SessionConfig;
use crate::session::SessionRegistry;

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
    session_registry: Arc<SessionRegistry<EncodingType>>,
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
    /// * Forwards [`TcpSocket::new_v4`]'s return values on failure.
    /// * Forwards [`TcpSocket::new_v6`]'s return values on failure.
    /// * Forwards [`TcpSocket::set_reuseaddr`]'s return values on failure.
    /// * Forwards [`TcpSocket::bind`]'s return values on failure.
    /// * Forwards [`TcpSocket::listen`]'s return values on failure.
    /// * Forwards [`TcpListener::local_addr`]'s return values on failure.
    ///
    /// # Panics
    ///
    /// Panics if the bound port is 0, which the OS never assigns.
    pub fn bind(config: ListenerConfig) -> Result<Self, std::io::Error> {
        let tcp_socket = match config.bind_addr {
            SocketAddr::V4(_) => TcpSocket::new_v4()?,
            SocketAddr::V6(_) => TcpSocket::new_v6()?,
        };
        tcp_socket.set_reuseaddr(true)?;
        tcp_socket.bind(config.bind_addr)?;
        let tcp_listener = tcp_socket.listen(LISTEN_BACKLOG)?;
        let port =
            NonZeroU16::new(tcp_listener.local_addr()?.port()).expect("a bound port is nonzero");
        let session_registry = Arc::new(SessionRegistry::new());
        let accept_loop = tokio::spawn(accept_connections(
            tcp_listener,
            Arc::clone(&session_registry),
            Duration::from_secs(config.handshake_timeout_sec.get()),
        ));
        Ok(Self {
            session_registry,
            advertised_host: config.advertised_host,
            port,
            accept_loop,
        })
    }

    /// Opens a session for one query job, registered before this method returns.
    ///
    /// # Returns
    ///
    /// The newly opened session on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards `SessionRegistry::open`'s return values on failure.
    pub fn open_session(
        &self,
        config: SessionConfig,
    ) -> Result<OpenSession<EncodingType>, SessionError> {
        self.session_registry
            .open(self.advertised_host.clone(), self.port, config)
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

/// The maximum number of connections waiting to be accepted on the listening socket. Linux caps it
/// at `net.core.somaxconn`.
const LISTEN_BACKLOG: u32 = 1024;

/// Accepts connections on `tcp_listener` and serves each one in its own task.
///
/// # Type Parameters
///
/// * `EncodingType` - The encoding of the results the sessions deliver.
async fn accept_connections<EncodingType: ResultEncoding>(
    tcp_listener: TcpListener,
    session_registry: Arc<SessionRegistry<EncodingType>>,
    handshake_timeout: Duration,
) {
    loop {
        match tcp_listener.accept().await {
            Ok((stream, peer_addr)) => {
                tokio::spawn(connection::serve(
                    stream,
                    peer_addr,
                    Arc::clone(&session_registry),
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

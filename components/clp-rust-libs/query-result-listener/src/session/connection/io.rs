//! Reads from the socket of a connection from a query task.

use bytes::BufMut;
use bytes::BytesMut;
use tokio::io::AsyncRead;
use tokio::io::AsyncReadExt;
use tokio::net::TcpStream;

use crate::error::ConnectionError;
use crate::error::ProtocolError;
use crate::protocol::Handshake;

pub(super) const MAX_READ_SIZE: usize = 64 * 1024;

/// Reads the handshake a query task sends first on its connection, leaving the bytes read after it
/// in `buffer`.
///
/// # Returns
///
/// On success:
///
/// * The handshake.
/// * `None` if the connection closes before sending any byte.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`ConnectionError::TruncatedFrame`] if the connection closes in the middle of the handshake.
/// * Forwards [`Handshake::decode`]'s return values on failure.
/// * Forwards [`read_more`]'s return values on failure.
pub(super) async fn read_handshake(
    input_stream: &mut TcpStream,
    buffer: &mut BytesMut,
) -> Result<Option<Handshake>, ConnectionError> {
    loop {
        if let Some(handshake) = Handshake::decode(buffer)? {
            return Ok(Some(handshake));
        }
        if 0 == read_more(input_stream, buffer).await? {
            if buffer.is_empty() {
                return Ok(None);
            }
            return Err(ConnectionError::TruncatedFrame);
        }
    }
}

/// Reads at most [`MAX_READ_SIZE`] more bytes from `input_stream` into `buffer`.
///
/// # Type Parameters
///
/// * `StreamType` - The type of the connection's stream.
///
/// # Returns
///
/// The number of bytes read on success, which is 0 once `input_stream` has reached EOF.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`AsyncReadExt::read_buf`]'s return values on failure.
pub(super) async fn read_more<StreamType: AsyncRead + Unpin>(
    input_stream: &mut StreamType,
    buffer: &mut BytesMut,
) -> std::io::Result<usize> {
    buffer.reserve(MAX_READ_SIZE);
    let mut read_window = (&mut *buffer).limit(MAX_READ_SIZE);
    input_stream.read_buf(&mut read_window).await
}

/// Checks that no bytes follow the connection's trailer, either left in `buffer` or read next from
/// `input_stream`.
///
/// # Type Parameters
///
/// * `StreamType` - The type of the connection's stream.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`ConnectionError::Protocol`] wrapping [`ProtocolError::DataAfterTrailer`] if any byte follows
///   the trailer.
/// * Forwards [`read_more`]'s return values on failure.
pub(super) async fn check_eof<StreamType: AsyncRead + Unpin>(
    input_stream: &mut StreamType,
    buffer: &mut BytesMut,
) -> Result<(), ConnectionError> {
    if !buffer.is_empty() || 0 != read_more(input_stream, buffer).await? {
        return Err(ProtocolError::DataAfterTrailer.into());
    }
    Ok(())
}

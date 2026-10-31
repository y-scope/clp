//! Wire protocol v1, which `clp-s` uses to stream a query task's results.
//!
//! A connection carries msgpack arrays without length prefixes: one handshake, followed by one
//! frame per result and a trailer that counts them. `clp-s` then shuts down its side of the
//! connection and waits for the listener's acknowledgement before it reports success.

use bytes::Buf;
use bytes::BytesMut;
use clp_rust_utils::types::ArchiveId;
use clp_rust_utils::types::SessionToken;
use rmp::decode::Bytes;
use rmp::decode::NumValueReadError;
use rmp::decode::ValueReadError;
use rmp::decode::bytes::BytesReadError;

use crate::error::ProtocolError;

/// The wire protocol version that a handshake must carry.
pub const PROTOCOL_VERSION: u64 = 1;

/// The frame a query task sends first on every connection.
#[derive(Debug, Eq, PartialEq)]
pub struct Handshake {
    /// The token of the session that receives the task's results.
    pub session_token: SessionToken,

    /// The task's index within its query job.
    pub task_index: u64,

    /// The ID of the archive the task searches.
    pub archive_id: ArchiveId,
}

impl Handshake {
    /// Decodes a handshake from the front of `buffer`, consuming its bytes.
    ///
    /// # Returns
    ///
    /// On success:
    ///
    /// * The decoded handshake.
    /// * `None` if `buffer` doesn't hold a complete handshake yet, leaving `buffer` untouched.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ProtocolError::UnsupportedVersion`] if the handshake's version isn't
    ///   [`PROTOCOL_VERSION`].
    /// * [`ProtocolError::MalformedFrame`] if the bytes don't form a handshake.
    /// * [`ProtocolError::InvalidSessionToken`] if the session token isn't a valid UUID.
    /// * [`ProtocolError::InvalidArchiveId`] if the archive ID isn't a valid UUID.
    pub fn decode(buffer: &mut BytesMut) -> Result<Option<Self>, ProtocolError> {
        decode_frame(buffer, |reader| {
            let num_fields = rmp::decode::read_array_len(reader)?;
            if 0 == num_fields {
                return Err(ProtocolError::MalformedFrame(format!(
                    "expected {NUM_HANDSHAKE_FIELDS} fields, got 0"
                ))
                .into());
            }
            let version: u64 = rmp::decode::read_int(reader)?;
            if PROTOCOL_VERSION != version {
                return Err(ProtocolError::UnsupportedVersion(version).into());
            }
            expect_num_fields(num_fields, NUM_HANDSHAKE_FIELDS)?;
            let session_token = read_str(reader, MAX_HANDSHAKE_STRING_LEN)?
                .parse::<SessionToken>()
                .map_err(ProtocolError::from)?;
            let task_index = rmp::decode::read_int(reader)?;
            let archive_id = read_str(reader, MAX_HANDSHAKE_STRING_LEN)?
                .parse::<ArchiveId>()
                .map_err(ProtocolError::from)?;
            Ok(Self {
                session_token,
                task_index,
                archive_id,
            })
        })
    }
}

/// A frame that follows the handshake.
#[derive(Debug, Eq, PartialEq)]
pub enum Frame {
    /// One result of the task.
    Result(ResultFrame),

    /// The last frame, sent once the task has sent every result.
    Trailer(Trailer),
}

impl Frame {
    /// Decodes a frame from the front of `buffer`, consuming its bytes. The number of fields of
    /// the frame's array tells a result from a trailer.
    ///
    /// # Returns
    ///
    /// On success:
    ///
    /// * The decoded frame.
    /// * `None` if `buffer` doesn't hold a complete frame yet, leaving `buffer` untouched.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ProtocolError::MalformedFrame`] if the bytes don't form a result frame or trailer.
    pub fn decode(buffer: &mut BytesMut) -> Result<Option<Self>, ProtocolError> {
        decode_frame(buffer, |reader| {
            match rmp::decode::read_array_len(reader)? {
                NUM_RESULT_FIELDS => {
                    let result_index = rmp::decode::read_int(reader)?;
                    let timestamp = rmp::decode::read_int(reader)?;
                    let message = read_str(reader, usize::MAX)?.to_owned();
                    Ok(Self::Result(ResultFrame {
                        result_index,
                        timestamp,
                        message,
                    }))
                }
                NUM_TRAILER_FIELDS => Ok(Self::Trailer(Trailer {
                    num_results: rmp::decode::read_int(reader)?,
                })),
                num_fields => Err(ProtocolError::MalformedFrame(format!(
                    "expected {NUM_RESULT_FIELDS} fields for a result or {NUM_TRAILER_FIELDS} for \
                     a trailer, got {num_fields}"
                ))
                .into()),
            }
        })
    }
}

/// The frame a query task sends for each result.
#[derive(Debug, Eq, PartialEq)]
pub struct ResultFrame {
    /// The result's index among the results of the connection, counting from 0.
    pub result_index: u64,

    /// The result's timestamp, in milliseconds since the Unix epoch.
    pub timestamp: i64,

    /// The result's log event message.
    pub message: String,
}

/// The frame a query task sends after its last result.
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct Trailer {
    /// The number of results the task sent on the connection.
    pub num_results: u64,
}

/// The acknowledgement the listener sends once it has taken responsibility for every result of a
/// connection.
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct Ack {
    /// The number of results the connection's trailer counted.
    pub num_results: u64,
}

impl Ack {
    /// # Returns
    ///
    /// The msgpack array `[num_results]`.
    ///
    /// # Panics
    ///
    /// Panics if writing to the `Vec` fails. This should never happen since writing to a `Vec`
    /// can't fail.
    #[must_use]
    pub fn encode(self) -> Vec<u8> {
        let mut encoded = Vec::new();
        rmp::encode::write_array_len(&mut encoded, NUM_ACK_FIELDS)
            .expect("writing to a `Vec` shouldn't fail");
        rmp::encode::write_uint(&mut encoded, self.num_results)
            .expect("writing to a `Vec` shouldn't fail");
        encoded
    }
}

const NUM_HANDSHAKE_FIELDS: u32 = 4;
const NUM_RESULT_FIELDS: u32 = 3;
const NUM_TRAILER_FIELDS: u32 = 1;
const NUM_ACK_FIELDS: u32 = 1;

/// The longest string a handshake may carry. Every textual UUID form fits, and the bound keeps an
/// unauthenticated peer from making the listener buffer an arbitrarily long frame.
const MAX_HANDSHAKE_STRING_LEN: usize = 64;

/// Why a frame couldn't be decoded from a buffer.
enum DecodeError {
    /// The buffer ends before the frame does.
    Incomplete,

    /// The buffer's bytes don't form a valid frame.
    Invalid(ProtocolError),
}

impl From<ProtocolError> for DecodeError {
    fn from(error: ProtocolError) -> Self {
        Self::Invalid(error)
    }
}

impl From<ValueReadError<BytesReadError>> for DecodeError {
    fn from(error: ValueReadError<BytesReadError>) -> Self {
        match error {
            ValueReadError::InvalidMarkerRead(_) | ValueReadError::InvalidDataRead(_) => {
                Self::Incomplete
            }
            ValueReadError::TypeMismatch(marker) => Self::Invalid(ProtocolError::MalformedFrame(
                format!("unexpected msgpack marker {marker:?}"),
            )),
        }
    }
}

impl From<NumValueReadError<BytesReadError>> for DecodeError {
    fn from(error: NumValueReadError<BytesReadError>) -> Self {
        match error {
            NumValueReadError::InvalidMarkerRead(_) | NumValueReadError::InvalidDataRead(_) => {
                Self::Incomplete
            }
            NumValueReadError::TypeMismatch(marker) => Self::Invalid(
                ProtocolError::MalformedFrame(format!("expected an integer, got {marker:?}")),
            ),
            NumValueReadError::OutOfRange => Self::Invalid(ProtocolError::MalformedFrame(
                "integer out of range".to_owned(),
            )),
        }
    }
}

/// Decodes one frame from the front of `buffer` with `decode`, consuming the frame's bytes only if
/// it decodes completely.
///
/// # Type Parameters
///
/// * `FrameType` - The type of the frame that `decode` produces.
///
/// # Returns
///
/// On success:
///
/// * The decoded frame.
/// * `None` if `buffer` ends before the frame does.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards the [`ProtocolError`] of an invalid frame from `decode`.
fn decode_frame<FrameType>(
    buffer: &mut BytesMut,
    decode: impl FnOnce(&mut Bytes<'_>) -> Result<FrameType, DecodeError>,
) -> Result<Option<FrameType>, ProtocolError> {
    let mut reader = Bytes::new(buffer);
    match decode(&mut reader) {
        Ok(frame) => {
            let frame_len = buffer.len() - reader.remaining_slice().len();
            buffer.advance(frame_len);
            Ok(Some(frame))
        }
        Err(DecodeError::Incomplete) => Ok(None),
        Err(DecodeError::Invalid(error)) => Err(error),
    }
}

/// # Errors
///
/// Returns an error if:
///
/// * [`DecodeError::Invalid`] if `num_fields` isn't `expected`.
fn expect_num_fields(num_fields: u32, expected: u32) -> Result<(), DecodeError> {
    if expected == num_fields {
        return Ok(());
    }
    Err(
        ProtocolError::MalformedFrame(format!("expected {expected} fields, got {num_fields}"))
            .into(),
    )
}

/// Reads a msgpack string of at most `max_len` bytes.
///
/// # Returns
///
/// The string, borrowed from the reader's buffer, on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`DecodeError::Incomplete`] if the reader ends before the string does.
/// * [`DecodeError::Invalid`] if:
///   * The next value isn't a string.
///   * The string is longer than `max_len` bytes.
///   * The string isn't valid UTF-8.
fn read_str<'buffer>(
    reader: &mut Bytes<'buffer>,
    max_len: usize,
) -> Result<&'buffer str, DecodeError> {
    let len = usize::try_from(rmp::decode::read_str_len(reader)?).map_err(|_| {
        ProtocolError::MalformedFrame("string length exceeds the address space".to_owned())
    })?;
    if len > max_len {
        return Err(ProtocolError::MalformedFrame(format!(
            "string of {len} bytes exceeds the limit of {max_len} bytes"
        ))
        .into());
    }
    let Some((bytes, remaining)) = reader.remaining_slice().split_at_checked(len) else {
        return Err(DecodeError::Incomplete);
    };
    *reader = Bytes::new(remaining);
    std::str::from_utf8(bytes)
        .map_err(|e| ProtocolError::MalformedFrame(format!("invalid UTF-8 string: {e}")).into())
}

//! Wire protocol v1, which `clp-s` uses to stream a query task's results.
//!
//! A connection carries msgpack arrays without length prefixes: one handshake, followed by one
//! frame per result and a trailer that counts them. `clp-s` then shuts down its side of the
//! connection and waits for the listener's acknowledgement before it reports success.

use std::borrow::Cow;

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
            check_num_fields(num_fields, NUM_HANDSHAKE_FIELDS)?;
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
                    let timestamp_millisecs = rmp::decode::read_int(reader)?;
                    let message = read_str(reader, usize::MAX)?.into_owned();
                    Ok(Self::Result(ResultFrame {
                        result_index,
                        timestamp_millisecs,
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
    pub timestamp_millisecs: i64,

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
    #[must_use]
    pub fn encode(self) -> Vec<u8> {
        let mut encoded = rmp::encode::ByteBuf::new();
        rmp::encode::write_array_len(&mut encoded, NUM_ACK_FIELDS);
        rmp::encode::write_uint(&mut encoded, self.num_results);
        encoded.into_vec()
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
            // A `Bytes` reader only fails to read when its buffer runs out.
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
            // A `Bytes` reader only fails to read when its buffer runs out.
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
fn check_num_fields(num_fields: u32, expected: u32) -> Result<(), DecodeError> {
    if expected == num_fields {
        return Ok(());
    }
    Err(
        ProtocolError::MalformedFrame(format!("expected {expected} fields, got {num_fields}"))
            .into(),
    )
}

/// Reads a msgpack string of at most `max_len` bytes, replacing each invalid UTF-8 sequence with
/// U+FFFD.
///
/// # Returns
///
/// The string on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`DecodeError::Incomplete`] if the reader ends before the string does.
/// * [`DecodeError::Invalid`] if:
///   * The next value isn't a string.
///   * The string's length doesn't fit in `usize`.
///   * The string is longer than `max_len` bytes.
fn read_str<'buffer>(
    reader: &mut Bytes<'buffer>,
    max_len: usize,
) -> Result<Cow<'buffer, str>, DecodeError> {
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
    Ok(String::from_utf8_lossy(bytes))
}

#[cfg(test)]
mod tests {
    use bytes::BytesMut;
    use clp_rust_utils::types::ArchiveId;
    use clp_rust_utils::types::SessionToken;

    use super::Ack;
    use super::Frame;
    use super::Handshake;
    use super::MAX_HANDSHAKE_STRING_LEN;
    use super::PROTOCOL_VERSION;
    use super::ResultFrame;
    use super::Trailer;
    use crate::error::ProtocolError;
    use crate::test_utils::encode_handshake;
    use crate::test_utils::encode_result;
    use crate::test_utils::encode_trailer;

    const ARCHIVE_ID: &str = "018e90e5-8b2a-4a61-a2fc-cac799936caf";
    const SESSION_TOKEN: &str = "6f1d3b52-8a4e-4c1b-9f6e-2d7a5c0b9e13";
    const TASK_INDEX: u64 = 7;

    /// # Returns
    ///
    /// The frames decoded from `bytes`, which must hold only complete frames.
    ///
    /// # Panics
    ///
    /// Panics if `bytes` doesn't hold only complete, valid frames.
    fn decode_frames(bytes: &[u8]) -> Vec<Frame> {
        let mut buffer = BytesMut::from(bytes);
        let mut frames = Vec::new();
        while let Some(frame) = Frame::decode(&mut buffer).expect("frames should be valid") {
            frames.push(frame);
        }
        assert_eq!(&buffer[..], &[] as &[u8]);
        frames
    }

    #[test]
    fn handshake_decodes_only_once_complete() -> anyhow::Result<()> {
        let encoded = encode_handshake(PROTOCOL_VERSION, SESSION_TOKEN, TASK_INDEX, ARCHIVE_ID);
        let mut buffer = BytesMut::new();
        for &byte in &encoded[..encoded.len() - 1] {
            buffer.extend_from_slice(&[byte]);
            assert_eq!(Handshake::decode(&mut buffer)?, None);
        }
        assert_eq!(buffer.len(), encoded.len() - 1);

        buffer.extend_from_slice(&encoded[encoded.len() - 1..]);
        buffer.extend_from_slice(&[0x93]);
        assert_eq!(
            Handshake::decode(&mut buffer)?,
            Some(Handshake {
                session_token: SESSION_TOKEN.parse::<SessionToken>()?,
                task_index: TASK_INDEX,
                archive_id: ARCHIVE_ID.parse::<ArchiveId>()?,
            })
        );
        assert_eq!(&buffer[..], &[0x93]);
        Ok(())
    }

    #[test]
    fn handshake_without_fields_is_rejected() {
        let mut buffer = BytesMut::from(&[0x90][..]);
        let error = Handshake::decode(&mut buffer).expect_err("an empty array should be rejected");
        assert!(
            matches!(error, ProtocolError::MalformedFrame(_)),
            "unexpected error: {error:?}"
        );
    }

    #[test]
    fn handshake_of_another_version_is_rejected_before_its_other_fields() {
        for version in [0, 2] {
            let mut buffer = BytesMut::from(
                &encode_handshake(version, SESSION_TOKEN, TASK_INDEX, ARCHIVE_ID)[..2],
            );
            let error = Handshake::decode(&mut buffer).expect_err("the version should be rejected");
            assert!(
                matches!(error, ProtocolError::UnsupportedVersion(rejected) if version == rejected),
                "unexpected error: {error:?}"
            );
        }
    }

    #[test]
    fn handshake_with_a_wrong_number_of_fields_is_rejected() -> anyhow::Result<()> {
        for num_fields in [3, 5] {
            let mut bytes = Vec::new();
            rmp::encode::write_array_len(&mut bytes, num_fields)?;
            rmp::encode::write_uint(&mut bytes, PROTOCOL_VERSION)?;
            rmp::encode::write_str(&mut bytes, SESSION_TOKEN)?;
            rmp::encode::write_uint(&mut bytes, TASK_INDEX)?;
            rmp::encode::write_str(&mut bytes, ARCHIVE_ID)?;
            let mut buffer = BytesMut::from(&bytes[..]);

            let error =
                Handshake::decode(&mut buffer).expect_err("the handshake should be rejected");
            assert!(
                matches!(error, ProtocolError::MalformedFrame(_)),
                "{num_fields} fields: unexpected error: {error:?}"
            );
        }
        Ok(())
    }

    #[test]
    fn handshake_with_an_overlong_string_is_rejected_before_it_arrives() -> anyhow::Result<()> {
        let mut bytes = Vec::new();
        rmp::encode::write_array_len(&mut bytes, 4)?;
        rmp::encode::write_uint(&mut bytes, PROTOCOL_VERSION)?;
        rmp::encode::write_str_len(&mut bytes, u32::try_from(MAX_HANDSHAKE_STRING_LEN + 1)?)?;
        let mut buffer = BytesMut::from(&bytes[..]);

        let error = Handshake::decode(&mut buffer).expect_err("the token should be rejected");
        assert!(
            matches!(error, ProtocolError::MalformedFrame(_)),
            "unexpected error: {error:?}"
        );
        Ok(())
    }

    #[test]
    fn result_accepts_a_negative_or_unsigned_encoded_timestamp() -> anyhow::Result<()> {
        let mut bytes = encode_result(0, -5, "message\n");
        // msgpack-c packs non-negative `int64_t` values with the smallest unsigned encoding.
        rmp::encode::write_array_len(&mut bytes, 3)?;
        rmp::encode::write_uint(&mut bytes, 1)?;
        rmp::encode::write_u64(&mut bytes, 1_700_000_000_000)?;
        rmp::encode::write_str(&mut bytes, "message\n")?;

        let result = |result_index: u64, timestamp_millisecs: i64| {
            Frame::Result(ResultFrame {
                result_index,
                timestamp_millisecs,
                message: "message\n".to_owned(),
            })
        };
        assert_eq!(
            decode_frames(&bytes),
            [result(0, -5), result(1, 1_700_000_000_000)]
        );
        Ok(())
    }

    #[test]
    fn result_with_invalid_utf8_decodes_lossily() -> anyhow::Result<()> {
        let mut bytes = Vec::new();
        rmp::encode::write_array_len(&mut bytes, 3)?;
        rmp::encode::write_uint(&mut bytes, 0)?;
        rmp::encode::write_sint(&mut bytes, 0)?;
        rmp::encode::write_str_len(&mut bytes, 1)?;
        bytes.push(0xff);

        assert_eq!(
            decode_frames(&bytes),
            [Frame::Result(ResultFrame {
                result_index: 0,
                timestamp_millisecs: 0,
                message: char::REPLACEMENT_CHARACTER.to_string(),
            })]
        );
        Ok(())
    }

    #[test]
    fn trailer_with_a_multi_byte_count_decodes_only_once_complete() -> anyhow::Result<()> {
        let encoded = encode_trailer(u64::MAX);
        let mut buffer = BytesMut::new();
        for &byte in &encoded[..encoded.len() - 1] {
            buffer.extend_from_slice(&[byte]);
            assert_eq!(Frame::decode(&mut buffer)?, None);
        }
        assert_eq!(buffer.len(), encoded.len() - 1);

        buffer.extend_from_slice(&encoded[encoded.len() - 1..]);
        buffer.extend_from_slice(&[0x93]);
        assert_eq!(
            Frame::decode(&mut buffer)?,
            Some(Frame::Trailer(Trailer {
                num_results: u64::MAX
            }))
        );
        assert_eq!(&buffer[..], &[0x93]);
        Ok(())
    }

    #[test]
    fn malformed_frames_are_rejected() -> anyhow::Result<()> {
        let mut malformed_frames = Vec::new();
        for num_fields in [0, 2, 4] {
            let mut bytes = Vec::new();
            rmp::encode::write_array_len(&mut bytes, num_fields)?;
            rmp::encode::write_uint(&mut bytes, 0)?;
            malformed_frames.push(bytes);
        }
        let mut negative_count = Vec::new();
        rmp::encode::write_array_len(&mut negative_count, 1)?;
        rmp::encode::write_sint(&mut negative_count, -1)?;
        malformed_frames.push(negative_count);
        let mut string_count = Vec::new();
        rmp::encode::write_array_len(&mut string_count, 1)?;
        rmp::encode::write_str(&mut string_count, "1")?;
        malformed_frames.push(string_count);
        let mut not_an_array = Vec::new();
        rmp::encode::write_uint(&mut not_an_array, 1)?;
        malformed_frames.push(not_an_array);

        for bytes in malformed_frames {
            let mut buffer = BytesMut::from(&bytes[..]);
            let error = Frame::decode(&mut buffer).expect_err("the frame should be rejected");
            assert!(
                matches!(error, ProtocolError::MalformedFrame(_)),
                "{bytes:02x?}: unexpected error: {error:?}"
            );
        }
        Ok(())
    }

    #[test]
    fn ack_is_a_one_element_array_of_its_count() -> anyhow::Result<()> {
        const NUM_RESULTS: u64 = 20_000;
        let encoded = Ack {
            num_results: NUM_RESULTS,
        }
        .encode();
        let mut reader = &encoded[..];
        assert_eq!(rmp::decode::read_array_len(&mut reader)?, 1);
        assert_eq!(rmp::decode::read_int::<u64, _>(&mut reader)?, NUM_RESULTS);
        assert_eq!(reader, &[] as &[u8]);
        Ok(())
    }

    #[test]
    fn result_decodes_only_once_complete() -> anyhow::Result<()> {
        const NEXT_BYTE: u8 = 0x93;

        // These lengths select msgpack's fixstr, str8, str16 and str32 headers, respectively.
        for message_len in [8, 32, 256, 65_536] {
            let message = "m".repeat(message_len);
            let encoded = encode_result(1, 2, &message);
            let mut buffer = BytesMut::new();
            for &byte in &encoded {
                assert_eq!(Frame::decode(&mut buffer)?, None);
                buffer.extend_from_slice(&[byte]);
            }
            assert_eq!(buffer.len(), encoded.len());

            buffer.extend_from_slice(&[NEXT_BYTE]);
            assert_eq!(
                Frame::decode(&mut buffer)?,
                Some(Frame::Result(ResultFrame {
                    result_index: 1,
                    timestamp_millisecs: 2,
                    message,
                }))
            );
            assert_eq!(&buffer[..], &[NEXT_BYTE]);
        }
        Ok(())
    }
}

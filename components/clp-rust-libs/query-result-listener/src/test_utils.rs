//! Encoders for the frames of the wire protocol and encodings of query results, for tests.

use std::convert::Infallible;
use std::ops::Range;
use std::time::Duration;

use clp_rust_utils::types::ArchiveId;
use spider_core::task::TaskIndex;

use crate::encoding::ResultEncoder;
use crate::encoding::ResultEncoding;

/// The ID of the archive that the tests' query tasks search.
pub const ARCHIVE_ID: &str = "018e90e5-8b2a-4a61-a2fc-cac799936caf";

/// The ID of an archive other than [`ARCHIVE_ID`].
pub const OTHER_ARCHIVE_ID: &str = "4b0c2f8e-6d13-4f4e-9a57-1c2d3e4f5a6b";

/// The ID of an archive for which a [`FallibleQueryResultEncoder`] can't be created.
pub const UNENCODABLE_ARCHIVE_ID: &str = "9c3e5a71-2f4b-4d8e-b6a0-7e1f2d3c4b5a";

/// A message that a [`FallibleQueryResultEncoder`] can't encode.
pub const UNENCODABLE_MESSAGE: &str = "unencodable\n";

/// The session token that the tests' handshakes carry.
pub const SESSION_TOKEN: &str = "6f1d3b52-8a4e-4c1b-9f6e-2d7a5c0b9e13";

/// A session token that no session is registered under.
pub const UNKNOWN_SESSION_TOKEN: &str = "d2b7e4a9-3c61-4f8e-a5d0-8b9c1e2f3a47";

/// The longest a test waits for the listener or a connection handler before failing.
pub const TIMEOUT: Duration = Duration::from_secs(10);

/// A query result in the batches of a [`QueryResultEncoding`].
#[derive(Clone, Debug, Eq, PartialEq)]
pub struct QueryResult {
    /// The ID of the archive that produced the result.
    pub archive_id: ArchiveId,

    /// The result's timestamp in milliseconds since the Unix epoch.
    pub timestamp_millisecs: i64,

    /// The result's log event message.
    pub message: String,
}

/// A [`ResultEncoding`] that batches [`QueryResult`] values.
pub enum QueryResultEncoding {}

impl ResultEncoding for QueryResultEncoding {
    type Batch = Vec<QueryResult>;
    type Encoder = QueryResultEncoder;
    type Error = Infallible;
}

/// The [`ResultEncoder`] for [`QueryResultEncoding`].
pub struct QueryResultEncoder {
    archive_id: ArchiveId,
    batch: Vec<QueryResult>,
}

impl ResultEncoder for QueryResultEncoder {
    type Batch = Vec<QueryResult>;
    type Error = Infallible;

    fn new(archive_id: ArchiveId) -> Result<Self, Infallible> {
        Ok(Self {
            archive_id,
            batch: Vec::new(),
        })
    }

    fn encode(&mut self, timestamp_millisecs: i64, message: &str) -> Result<(), Infallible> {
        self.batch.push(QueryResult {
            archive_id: self.archive_id,
            timestamp_millisecs,
            message: message.to_owned(),
        });
        Ok(())
    }

    fn take_batch(&mut self) -> Vec<QueryResult> {
        std::mem::take(&mut self.batch)
    }
}

/// A [`ResultEncoding`] like [`QueryResultEncoding`] that fails for [`UNENCODABLE_ARCHIVE_ID`] and
/// [`UNENCODABLE_MESSAGE`].
pub enum FallibleQueryResultEncoding {}

impl ResultEncoding for FallibleQueryResultEncoding {
    type Batch = Vec<QueryResult>;
    type Encoder = FallibleQueryResultEncoder;
    type Error = EncodingError;
}

/// The [`ResultEncoder`] for [`FallibleQueryResultEncoding`].
pub struct FallibleQueryResultEncoder {
    encoder: QueryResultEncoder,
}

impl ResultEncoder for FallibleQueryResultEncoder {
    type Batch = Vec<QueryResult>;
    type Error = EncodingError;

    fn new(archive_id: ArchiveId) -> Result<Self, EncodingError> {
        if UNENCODABLE_ARCHIVE_ID == archive_id.to_string() {
            return Err(EncodingError::UnencodableArchive(archive_id));
        }
        let Ok(encoder) = QueryResultEncoder::new(archive_id);
        Ok(Self { encoder })
    }

    fn encode(&mut self, timestamp_millisecs: i64, message: &str) -> Result<(), EncodingError> {
        if UNENCODABLE_MESSAGE == message {
            return Err(EncodingError::UnencodableMessage {
                archive_id: self.encoder.archive_id,
                timestamp_millisecs,
            });
        }
        let Ok(()) = self.encoder.encode(timestamp_millisecs, message);
        Ok(())
    }

    fn take_batch(&mut self) -> Vec<QueryResult> {
        self.encoder.take_batch()
    }
}

/// Failures of a [`FallibleQueryResultEncoder`].
#[derive(Debug, Eq, PartialEq, thiserror::Error)]
pub enum EncodingError {
    #[error("archive {0} is unencodable")]
    UnencodableArchive(ArchiveId),

    #[error(
        "the message of the result at {timestamp_millisecs} of archive {archive_id} is unencodable"
    )]
    UnencodableMessage {
        archive_id: ArchiveId,
        timestamp_millisecs: i64,
    },
}

/// # Returns
///
/// The bytes of a handshake with the given fields.
#[must_use]
pub fn encode_handshake(
    version: u64,
    session_token: &str,
    task_index: u64,
    archive_id: &str,
) -> Vec<u8> {
    let mut bytes = rmp::encode::ByteBuf::new();
    rmp::encode::write_array_len(&mut bytes, 4);
    rmp::encode::write_uint(&mut bytes, version);
    rmp::encode::write_str(&mut bytes, session_token);
    rmp::encode::write_uint(&mut bytes, task_index);
    rmp::encode::write_str(&mut bytes, archive_id);
    bytes.into_vec()
}

/// # Returns
///
/// The bytes of a result frame with the given fields.
#[must_use]
pub fn encode_result(result_index: u64, timestamp_millisecs: i64, message: &str) -> Vec<u8> {
    let mut bytes = rmp::encode::ByteBuf::new();
    rmp::encode::write_array_len(&mut bytes, 3);
    rmp::encode::write_uint(&mut bytes, result_index);
    rmp::encode::write_sint(&mut bytes, timestamp_millisecs);
    rmp::encode::write_str(&mut bytes, message);
    bytes.into_vec()
}

/// # Returns
///
/// The bytes of a trailer counting `num_results` results.
#[must_use]
pub fn encode_trailer(num_results: u64) -> Vec<u8> {
    let mut bytes = rmp::encode::ByteBuf::new();
    rmp::encode::write_array_len(&mut bytes, 1);
    rmp::encode::write_uint(&mut bytes, num_results);
    bytes.into_vec()
}

/// # Returns
///
/// The bytes of the result frames at `result_indices` of task `task_index`.
#[must_use]
pub fn encode_results(
    task_index: TaskIndex,
    result_indices: impl IntoIterator<Item = u64>,
) -> Vec<u8> {
    let mut bytes = Vec::new();
    for result_index in result_indices {
        bytes.extend(encode_result(
            result_index,
            timestamp(result_index),
            &message(task_index, result_index),
        ));
    }
    bytes
}

/// # Returns
///
/// The timestamp of the result at `result_index`.
///
/// # Panics
///
/// Panics if `result_index` doesn't fit in `i64`.
#[must_use]
pub fn timestamp(result_index: u64) -> i64 {
    1_700_000_000_000 + i64::try_from(result_index).expect("test result indices fit in `i64`")
}

/// # Returns
///
/// The message of the result at `result_index` of task `task_index`.
#[must_use]
pub fn message(task_index: TaskIndex, result_index: u64) -> String {
    format!("{{\"task\":{task_index},\"result\":{result_index}}}\n")
}

/// # Returns
///
/// The results at `result_indices` of task `task_index` of archive `archive_id`, in index order.
///
/// # Panics
///
/// Panics if `archive_id` isn't a valid UUID.
#[must_use]
pub fn expected_results(
    task_index: TaskIndex,
    archive_id: &str,
    result_indices: Range<u64>,
) -> Vec<QueryResult> {
    let archive_id = archive_id
        .parse::<ArchiveId>()
        .expect("archive ID fixtures should be valid UUIDs");
    result_indices
        .map(|result_index| QueryResult {
            archive_id,
            timestamp_millisecs: timestamp(result_index),
            message: message(task_index, result_index),
        })
        .collect()
}

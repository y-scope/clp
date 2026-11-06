//! Encoders for the frames of the wire protocol, and an encoder of query results, for tests.

use clp_rust_utils::types::ArchiveId;

use crate::encoding::ResultEncoder;

/// A query result in the batches of a [`QueryResultEncoder`].
#[derive(Clone, Debug, Eq, PartialEq)]
pub struct QueryResult {
    /// The ID of the archive that produced the result.
    pub archive_id: ArchiveId,

    /// The result's timestamp in milliseconds since the Unix epoch.
    pub timestamp_millisecs: i64,

    /// The result's log event message.
    pub message: String,
}

/// A [`ResultEncoder`] that batches [`QueryResult`] values.
pub struct QueryResultEncoder {
    archive_id: ArchiveId,
    batch: Vec<QueryResult>,
}

impl ResultEncoder for QueryResultEncoder {
    type Batch = Vec<QueryResult>;

    fn new(archive_id: ArchiveId) -> Self {
        Self {
            archive_id,
            batch: Vec::new(),
        }
    }

    fn encode(&mut self, timestamp_millisecs: i64, message: &str) {
        self.batch.push(QueryResult {
            archive_id: self.archive_id,
            timestamp_millisecs,
            message: message.to_owned(),
        });
    }

    fn take_batch(&mut self) -> Vec<QueryResult> {
        std::mem::take(&mut self.batch)
    }
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

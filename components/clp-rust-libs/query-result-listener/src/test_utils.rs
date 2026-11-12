//! Encoders for the frames of the wire protocol, encodings of query results, and a simulated
//! `clp-s` query task, for tests.

use std::convert::Infallible;
use std::ops::Range;
use std::time::Duration;

use clp_rust_utils::job_config::NetworkOutput;
use clp_rust_utils::types::ArchiveId;
use clp_rust_utils::types::SessionToken;
use spider_core::task::TaskIndex;
use tokio::io::AsyncWriteExt;
use tokio::net::TcpStream;

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

/// A simulated attempt of a `clp-s` query task, streaming its results over TCP.
pub struct FakeClpS {
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
    pub async fn connect(network_output: &NetworkOutput) -> Self {
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
    pub async fn send(&mut self, bytes: &[u8]) {
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
    pub async fn send_handshake(&mut self, task_index: TaskIndex, archive_id: &str) {
        use crate::protocol::PROTOCOL_VERSION;

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
    pub async fn send_results(
        &mut self,
        task_index: TaskIndex,
        result_indices: impl IntoIterator<Item = u64>,
    ) {
        self.send(&encode_results(task_index, result_indices)).await;
    }

    /// Sends the results at `result_indices` of task `task_index` one at a time, until every result
    /// is sent or the listener has closed the connection.
    pub async fn send_results_until_closed(
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
    pub async fn finish(mut self, num_results: u64) -> Vec<u8> {
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
    pub async fn expect_closed_by_listener(self) {
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
        use std::io::ErrorKind;

        use tokio::io::AsyncReadExt;
        use tokio::time::timeout;

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

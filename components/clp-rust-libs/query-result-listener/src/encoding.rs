//! Encodes query results into transport-specific batches.

use clp_rust_utils::types::ArchiveId;

/// A transport-specific encoding of query results.
///
/// An encoding is only a type: it binds the batches the session's consumer receives to the encoder
/// that produces them, and is never instantiated.
pub trait ResultEncoding: Send + Sync + 'static {
    /// A batch of encoded query results.
    type Batch: Send + 'static;

    /// An encoder for one connection's query results.
    type Encoder: ResultEncoder<Batch = Self::Batch>;
}

/// Encodes the query results of one connection into transport-specific batches.
pub trait ResultEncoder: Send + 'static {
    /// A batch of encoded query results.
    type Batch: Send + 'static;

    /// Creates an encoder for one connection's query results from one archive.
    ///
    /// # Parameters
    ///
    /// * `archive_id` - The ID of the archive searched by the connection.
    ///
    /// # Returns
    ///
    /// A new encoder for the connection's query results.
    fn new(archive_id: ArchiveId) -> Self;

    /// Appends a query result to the pending batch.
    ///
    /// # Parameters
    ///
    /// * `timestamp_millisecs` - The result's timestamp in milliseconds since the Unix epoch.
    /// * `message` - The result's log event message.
    fn encode(&mut self, timestamp_millisecs: i64, message: &str);

    /// Takes the pending batch and starts a new empty batch.
    ///
    /// # Returns
    ///
    /// The query results appended since the encoder was created or [`Self::take_batch`] was last
    /// called.
    fn take_batch(&mut self) -> Self::Batch;
}

#[cfg(test)]
mod tests {
    use anyhow::Result;
    use clp_rust_utils::types::ArchiveId;

    use super::ResultEncoder;
    use super::ResultEncoding;
    use crate::test_utils::QueryResult;
    use crate::test_utils::QueryResultEncoding;

    #[test]
    fn query_result_encoder_stamps_the_archive_id_and_starts_next_batch_empty() -> Result<()> {
        use crate::test_utils::ARCHIVE_ID;

        let archive_id = ARCHIVE_ID.parse::<ArchiveId>()?;
        let query_result = |timestamp_millisecs: i64, message: &str| QueryResult {
            archive_id,
            timestamp_millisecs,
            message: message.to_owned(),
        };
        let mut encoder = <QueryResultEncoding as ResultEncoding>::Encoder::new(archive_id);

        encoder.encode(2, "b\n");
        encoder.encode(1, "a\n");
        let first_batch = encoder.take_batch();
        encoder.encode(3, "c");
        let second_batch = encoder.take_batch();

        assert_eq!(
            first_batch,
            [query_result(2, "b\n"), query_result(1, "a\n")]
        );
        assert_eq!(second_batch, [query_result(3, "c")]);
        Ok(())
    }
}

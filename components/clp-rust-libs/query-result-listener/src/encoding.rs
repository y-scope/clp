//! Encodes query results into transport-specific batches.

use clp_rust_utils::types::ArchiveId;

/// A transport-specific encoding of query results.
pub trait ResultEncoding: Send + Sync + 'static {
    /// A batch of encoded query results.
    type Batch: Send + 'static;

    /// An encoder for one connection's query results.
    type Encoder: ResultEncoder<Batch = Self::Batch>;

    /// Creates an encoder for one connection's query results from one archive.
    ///
    /// # Parameters
    ///
    /// * `archive_id` - The ID of the archive searched by the connection.
    ///
    /// # Returns
    ///
    /// A new encoder for the connection's query results.
    fn encoder(&self, archive_id: ArchiveId) -> Self::Encoder;
}

/// Encodes query results from one connection into batches.
pub trait ResultEncoder: Send + 'static {
    /// A batch of encoded query results.
    type Batch: Send + 'static;

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

/// A query result in the batches of a [`QueryResultEncoding`].
#[cfg(test)]
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
#[cfg(test)]
#[derive(Clone, Copy, Debug, Default)]
pub struct QueryResultEncoding;

#[cfg(test)]
impl ResultEncoding for QueryResultEncoding {
    type Batch = Vec<QueryResult>;
    type Encoder = QueryResultEncoder;

    fn encoder(&self, archive_id: ArchiveId) -> QueryResultEncoder {
        QueryResultEncoder {
            archive_id,
            batch: Vec::new(),
        }
    }
}

/// The [`ResultEncoder`] for [`QueryResultEncoding`].
#[cfg(test)]
pub struct QueryResultEncoder {
    archive_id: ArchiveId,
    batch: Vec<QueryResult>,
}

#[cfg(test)]
impl ResultEncoder for QueryResultEncoder {
    type Batch = Vec<QueryResult>;

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

#[cfg(test)]
mod tests {
    use anyhow::Result;
    use clp_rust_utils::types::ArchiveId;

    use super::QueryResult;
    use super::QueryResultEncoding;
    use super::ResultEncoder;
    use super::ResultEncoding;

    #[test]
    fn query_result_encoder_stamps_the_archive_id_and_starts_next_batch_empty() -> Result<()> {
        const ARCHIVE_ID: &str = "018e90e5-8b2a-4a61-a2fc-cac799936caf";

        let archive_id = ARCHIVE_ID.parse::<ArchiveId>()?;
        let query_result = |timestamp_millisecs: i64, message: &str| QueryResult {
            archive_id,
            timestamp_millisecs,
            message: message.to_owned(),
        };
        let mut encoder = QueryResultEncoding.encoder(archive_id);

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

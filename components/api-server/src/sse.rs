//! Server-Sent Events encoding of streamed query results.

use axum::body::Bytes;
use clp_rust_utils::types::ArchiveId;
use query_result_listener::encoding::ResultEncoder;
use query_result_listener::encoding::ResultEncoding;

/// A [`ResultEncoding`] that encodes each result as an unnamed event whose data is the result
/// serialized as JSON, without its message's trailing newline.
pub enum SseEncoding {}

impl ResultEncoding for SseEncoding {
    type Batch = Bytes;
    type Encoder = SseEncoder;
    type Error = serde_json::Error;
}

/// The [`ResultEncoder`] for [`SseEncoding`].
pub struct SseEncoder {
    event_prefix: Box<[u8]>,
    batch: Vec<u8>,
}

impl ResultEncoder for SseEncoder {
    type Batch = Bytes;
    type Error = serde_json::Error;

    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`serde_json::to_writer`]'s return values on failure.
    fn new(archive_id: ArchiveId) -> Result<Self, serde_json::Error> {
        let mut event_prefix = b"data: {\"archive_id\":".to_vec();
        serde_json::to_writer(&mut event_prefix, &archive_id)?;
        event_prefix.extend_from_slice(b",\"timestamp_millisecs\":");
        Ok(Self {
            event_prefix: event_prefix.into_boxed_slice(),
            batch: Vec::new(),
        })
    }

    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`serde_json::to_writer`]'s return values on failure.
    fn encode(&mut self, timestamp_millisecs: i64, message: &str) -> Result<(), serde_json::Error> {
        self.batch.extend_from_slice(&self.event_prefix);
        serde_json::to_writer(&mut self.batch, &timestamp_millisecs)?;
        self.batch.extend_from_slice(b",\"message\":");
        serde_json::to_writer(
            &mut self.batch,
            message.strip_suffix('\n').unwrap_or(message),
        )?;
        self.batch.extend_from_slice(b"}\n\n");
        Ok(())
    }

    fn take_batch(&mut self) -> Bytes {
        Bytes::from(std::mem::take(&mut self.batch))
    }
}

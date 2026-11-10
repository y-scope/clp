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
}

/// The [`ResultEncoder`] for [`SseEncoding`].
pub struct SseEncoder {
    event_prefix: Box<[u8]>,
    batch: Vec<u8>,
}

impl SseEncoder {
    /// Appends the event of one result to the pending batch.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`serde_json::to_writer`]'s return values on failure.
    fn write_event(
        &mut self,
        timestamp_millisecs: i64,
        message: &str,
    ) -> Result<(), serde_json::Error> {
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
}

impl ResultEncoder for SseEncoder {
    type Batch = Bytes;

    /// # Panics
    ///
    /// Panics if serializing the archive ID into a `Vec` fails, which it never does.
    fn new(archive_id: ArchiveId) -> Self {
        let mut event_prefix = b"data: {\"archive_id\":".to_vec();
        serde_json::to_writer(&mut event_prefix, &archive_id)
            .expect("serializing into a `Vec` shouldn't fail");
        event_prefix.extend_from_slice(b",\"timestamp_millisecs\":");
        Self {
            event_prefix: event_prefix.into_boxed_slice(),
            batch: Vec::new(),
        }
    }

    /// # Panics
    ///
    /// Panics if serializing the result into a `Vec` fails, which it never does.
    fn encode(&mut self, timestamp_millisecs: i64, message: &str) {
        self.write_event(timestamp_millisecs, message)
            .expect("serializing into a `Vec` shouldn't fail");
    }

    fn take_batch(&mut self) -> Bytes {
        Bytes::from(std::mem::take(&mut self.batch))
    }
}

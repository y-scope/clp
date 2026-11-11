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

#[cfg(test)]
mod tests {
    use anyhow::Result;
    use axum::response::sse::Event;
    use query_result_listener::test_utils::ARCHIVE_ID;
    use serde::Serialize;

    use super::*;

    /// The data of a query result's event.
    #[derive(Serialize)]
    struct StreamingQueryResult {
        archive_id: ArchiveId,
        timestamp_millisecs: i64,
        message: String,
    }

    /// # Returns
    ///
    /// The event that [`Event::json_data`] encodes for the result made of `archive_id`,
    /// `timestamp_millisecs` and `message`.
    ///
    /// # Panics
    ///
    /// Panics if serializing the result fails.
    fn axum_result_event(
        archive_id: ArchiveId,
        timestamp_millisecs: i64,
        message: String,
    ) -> Event {
        Event::default()
            .json_data(StreamingQueryResult {
                archive_id,
                timestamp_millisecs,
                message,
            })
            .expect("serializing a result shouldn't fail")
    }

    /// # Returns
    ///
    /// The body of an [`axum::response::Sse`] response streaming `events`.
    ///
    /// # Panics
    ///
    /// Panics if the body can't be read.
    async fn axum_sse_body(events: Vec<Event>) -> Bytes {
        use std::convert::Infallible;

        use axum::response::IntoResponse;
        use axum::response::Sse;
        use futures::stream;
        use http_body_util::BodyExt;

        Sse::new(stream::iter(events.into_iter().map(Ok::<_, Infallible>)))
            .into_response()
            .into_body()
            .collect()
            .await
            .expect("the body should be readable")
            .to_bytes()
    }

    #[tokio::test]
    async fn sse_encoder_matches_axum_json_events() -> Result<()> {
        const TIMESTAMP_MILLISECS: i64 = -1;

        let archive_id = ARCHIVE_ID.parse::<ArchiveId>()?;
        let mut encoder = <SseEncoding as ResultEncoding>::Encoder::new(archive_id)?;
        let mut axum_events = Vec::new();
        for (message, event_message) in [
            ("{\"quoted\":\"value\"}\n", "{\"quoted\":\"value\"}"),
            ("x\n\n", "x\n"),
            ("x\r\n", "x\r"),
            ("inner\nnewline", "inner\nnewline"),
            ("", ""),
            ("\n", ""),
        ] {
            encoder.encode(TIMESTAMP_MILLISECS, message)?;
            axum_events.push(axum_result_event(
                archive_id,
                TIMESTAMP_MILLISECS,
                event_message.to_owned(),
            ));
        }

        assert_eq!(encoder.take_batch(), axum_sse_body(axum_events).await);
        Ok(())
    }

    #[tokio::test]
    async fn sse_encoder_starts_the_next_batch_empty() -> Result<()> {
        let archive_id = ARCHIVE_ID.parse::<ArchiveId>()?;
        let mut encoder = <SseEncoding as ResultEncoding>::Encoder::new(archive_id)?;
        encoder.encode(1, "first\n")?;
        encoder.take_batch();
        encoder.encode(2, "second\n")?;

        assert_eq!(
            encoder.take_batch(),
            axum_sse_body(vec![axum_result_event(archive_id, 2, "second".to_owned())]).await
        );
        Ok(())
    }
}

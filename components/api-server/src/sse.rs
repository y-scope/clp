//! Server-Sent Events encoding of streamed query results, and the response that streams the
//! encoded events.

use std::pin::Pin;
use std::task::Context;
use std::task::Poll;
use std::time::Duration;

use axum::BoxError;
use axum::body::Bytes;
use axum::response::IntoResponse;
use axum::response::Response;
use clp_rust_utils::types::ArchiveId;
use futures::Stream;
use pin_project_lite::pin_project;
use query_result_listener::encoding::ResultEncoder;
use query_result_listener::encoding::ResultEncoding;
use serde::Serialize;
use tokio::time::Sleep;

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
        event_prefix.extend_from_slice(b",\"timestamp\":");
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

/// A Server-Sent Events response whose events are already encoded, laid out as
/// [`axum::response::Sse`] with [`axum::response::sse::KeepAlive::default`] lays them out.
///
/// # Type Parameters
///
/// * `EventStreamType` - The stream of the encoded events.
pub struct SseResponse<EventStreamType> {
    events: KeepAliveStream<EventStreamType>,
}

impl<EventStreamType> SseResponse<EventStreamType> {
    /// Factory function.
    ///
    /// Starts the keep-alive interval, as [`axum::response::Sse::keep_alive`] does.
    ///
    /// # Returns
    ///
    /// The newly created response.
    pub fn new(events: EventStreamType) -> Self {
        const KEEP_ALIVE_INTERVAL: Duration = Duration::from_secs(15);

        Self {
            events: KeepAliveStream::new(events, KEEP_ALIVE_INTERVAL),
        }
    }
}

impl<
    EventStreamType: Stream<Item = Result<Bytes, ErrorType>> + Send + 'static,
    ErrorType: Into<BoxError>,
> IntoResponse for SseResponse<EventStreamType>
{
    fn into_response(self) -> Response {
        use axum::body::Body;
        use axum::http::header::CACHE_CONTROL;
        use axum::http::header::CONTENT_TYPE;

        (
            [
                (CONTENT_TYPE, "text/event-stream"),
                (CACHE_CONTROL, "no-cache"),
            ],
            Body::from_stream(self.events),
        )
            .into_response()
    }
}

/// Encodes a named event, laid out as [`axum::response::sse::Event::json_data`] lays out an event
/// named with [`axum::response::sse::Event::event`].
///
/// # Type Parameters
///
/// * `DataType` - The type of the event's data.
///
/// # Returns
///
/// The event's bytes on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`serde_json::to_writer`]'s return values on failure.
pub fn encode_event<DataType: Serialize>(
    name: &str,
    data: &DataType,
) -> Result<Bytes, serde_json::Error> {
    let mut event = Vec::new();
    event.extend_from_slice(b"event: ");
    event.extend_from_slice(name.as_bytes());
    event.extend_from_slice(b"\ndata: ");
    serde_json::to_writer(&mut event, data)?;
    event.extend_from_slice(b"\n\n");
    Ok(Bytes::from(event))
}

pin_project! {
    /// A stream of encoded events that also yields a keep-alive comment whenever no event has been
    /// yielded for an interval.
    ///
    /// # Type Parameters
    ///
    /// * `EventStreamType` - The stream of the encoded events.
    struct KeepAliveStream<EventStreamType> {
        #[pin]
        events: EventStreamType,
        #[pin]
        timer: Sleep,
        interval: Duration,
    }
}

impl<EventStreamType> KeepAliveStream<EventStreamType> {
    /// Factory function.
    ///
    /// Starts the keep-alive interval.
    ///
    /// # Returns
    ///
    /// The newly created stream.
    fn new(events: EventStreamType, interval: Duration) -> Self {
        Self {
            events,
            timer: tokio::time::sleep(interval),
            interval,
        }
    }
}

impl<EventStreamType: Stream<Item = Result<Bytes, ErrorType>>, ErrorType> Stream
    for KeepAliveStream<EventStreamType>
{
    type Item = Result<Bytes, ErrorType>;

    fn poll_next(self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Option<Self::Item>> {
        use std::task::ready;

        use tokio::time::Instant;

        const KEEP_ALIVE_COMMENT: &[u8] = b":\n\n";

        let mut this = self.project();
        match this.events.poll_next(cx) {
            Poll::Ready(Some(Ok(event))) => {
                this.timer.reset(Instant::now() + *this.interval);
                Poll::Ready(Some(Ok(event)))
            }
            Poll::Ready(end_or_error) => Poll::Ready(end_or_error),
            Poll::Pending => {
                ready!(this.timer.as_mut().poll(cx));
                this.timer.reset(Instant::now() + *this.interval);
                Poll::Ready(Some(Ok(Bytes::from_static(KEEP_ALIVE_COMMENT))))
            }
        }
    }
}

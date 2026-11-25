//! Server-Sent Events encodings of streamed query results and of named events, and the response
//! that streams encoded events.

use std::pin::pin;

use async_stream::stream;
use axum::body::Bytes;
use axum::response::IntoResponse;
use axum::response::Response;
use clp_rust_utils::api_server::SSE_KEEP_ALIVE_INTERVAL;
use clp_rust_utils::types::ArchiveUuid;
use futures::Stream;
use futures::StreamExt;
use query_result_listener::encoding::ResultEncoder;
use query_result_listener::encoding::ResultEncoding;
use serde::Serialize;

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

    fn new(archive_uuid: ArchiveUuid) -> Result<Self, serde_json::Error> {
        let mut event_prefix = b"data: {\"archive_id\":".to_vec();
        serde_json::to_writer(&mut event_prefix, &archive_uuid)?;
        event_prefix.extend_from_slice(b",\"timestamp_millisecs\":");
        Ok(Self {
            event_prefix: event_prefix.into_boxed_slice(),
            batch: Vec::new(),
        })
    }

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

/// A Server-Sent Events response that streams already encoded events as they are, with the headers
/// of an [`axum::response::Sse`] response, and sends a keep-alive comment whenever it has sent
/// nothing for [`SSE_KEEP_ALIVE_INTERVAL`].
///
/// # Type Parameters
///
/// * `EventStreamType` - The stream of the encoded events, whose error ends the response.
pub struct SseResponse<EventStreamType: Stream<Item = Result<Bytes, serde_json::Error>>> {
    event_stream: EventStreamType,
}

impl<EventStreamType: Stream<Item = Result<Bytes, serde_json::Error>>>
    SseResponse<EventStreamType>
{
    /// Factory function.
    ///
    /// # Returns
    ///
    /// The newly created response.
    pub const fn new(event_stream: EventStreamType) -> Self {
        Self { event_stream }
    }
}

impl<EventStreamType: Stream<Item = Result<Bytes, serde_json::Error>> + Send + 'static> IntoResponse
    for SseResponse<EventStreamType>
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
            Body::from_stream(with_keep_alive_comments(self.event_stream)),
        )
            .into_response()
    }
}

/// Encodes an event named `name` whose data is `data` serialized as JSON, laid out as
/// [`Event::json_data`](axum::response::sse::Event::json_data) lays out an event named with
/// [`Event::event`](axum::response::sse::Event::event).
///
/// # Type Parameters
///
/// * `DataType` - The type of the event's data.
///
/// # Returns
///
/// The encoded event on success.
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
    let mut event = b"event: ".to_vec();
    event.extend_from_slice(name.as_bytes());
    event.extend_from_slice(b"\ndata: ");
    serde_json::to_writer(&mut event, data)?;
    event.extend_from_slice(b"\n\n");
    Ok(Bytes::from(event))
}

/// Adds keep-alive comments to `event_stream`.
///
/// # Type Parameters
///
/// * `EventStreamType` - The stream of the encoded events.
///
/// # Returns
///
/// A stream that yields the items of `event_stream` as they are, and a keep-alive comment whenever
/// it has yielded nothing for [`SSE_KEEP_ALIVE_INTERVAL`].
fn with_keep_alive_comments<EventStreamType: Stream<Item = Result<Bytes, serde_json::Error>>>(
    event_stream: EventStreamType,
) -> impl Stream<Item = Result<Bytes, serde_json::Error>> {
    const KEEP_ALIVE_COMMENT: &[u8] = b":\n\n";

    stream! {
        let mut event_stream = pin!(event_stream);
        loop {
            match tokio::time::timeout(SSE_KEEP_ALIVE_INTERVAL, event_stream.next()).await {
                Ok(Some(event)) => yield event,
                Ok(None) => break,
                Err(_) => yield Ok(Bytes::from_static(KEEP_ALIVE_COMMENT)),
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use std::time::Duration;

    use anyhow::Result;
    use axum::response::sse::Event;
    use query_result_listener::test_utils::ARCHIVE_UUID;

    use super::*;

    /// # Returns
    ///
    /// The event that [`Event::json_data`] encodes for the result made of `archive_uuid`,
    /// `timestamp_millisecs` and `message`.
    ///
    /// # Panics
    ///
    /// Panics if serializing the result fails.
    fn axum_result_event(
        archive_uuid: ArchiveUuid,
        timestamp_millisecs: i64,
        message: String,
    ) -> Event {
        use clp_rust_utils::api_server::streaming_query::StreamingQueryResultEvent;

        Event::default()
            .json_data(StreamingQueryResultEvent {
                archive_uuid,
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

        let archive_uuid = ARCHIVE_UUID.parse::<ArchiveUuid>()?;
        let mut encoder = <SseEncoding as ResultEncoding>::Encoder::new(archive_uuid)?;
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
                archive_uuid,
                TIMESTAMP_MILLISECS,
                event_message.to_owned(),
            ));
        }

        assert_eq!(encoder.take_batch(), axum_sse_body(axum_events).await);
        Ok(())
    }

    #[tokio::test]
    async fn sse_encoder_starts_the_next_batch_empty() -> Result<()> {
        let archive_uuid = ARCHIVE_UUID.parse::<ArchiveUuid>()?;
        let mut encoder = <SseEncoding as ResultEncoding>::Encoder::new(archive_uuid)?;
        encoder.encode(1, "first\n")?;
        encoder.take_batch();
        encoder.encode(2, "second\n")?;

        assert_eq!(
            encoder.take_batch(),
            axum_sse_body(vec![axum_result_event(
                archive_uuid,
                2,
                "second".to_owned()
            )])
            .await
        );
        Ok(())
    }

    #[tokio::test]
    async fn encode_event_matches_axum_named_json_events() -> Result<()> {
        let data = serde_json::json!({"message": "a \"quoted\"\nmessage"});

        assert_eq!(
            encode_event("error", &data)?,
            axum_sse_body(vec![Event::default().event("error").json_data(&data)?]).await
        );
        Ok(())
    }

    #[tokio::test]
    async fn sse_response_has_the_headers_of_an_axum_sse_response() {
        use std::convert::Infallible;

        use axum::response::Sse;
        use futures::stream;

        let response =
            SseResponse::new(stream::pending::<Result<Bytes, serde_json::Error>>()).into_response();
        let axum_response =
            Sse::new(stream::pending::<Result<Event, Infallible>>()).into_response();

        assert_eq!(response.headers(), axum_response.headers());
    }

    #[tokio::test(start_paused = true)]
    async fn an_idle_sse_response_sends_a_keep_alive_comment_every_interval() -> Result<()> {
        use futures::stream;
        use tokio::time::Instant;

        let keep_alive_comment = axum_sse_body(vec![Event::DEFAULT_KEEP_ALIVE]).await;
        let start = Instant::now();
        let mut body = SseResponse::new(stream::pending::<Result<Bytes, serde_json::Error>>())
            .into_response()
            .into_body();

        for num_intervals in 1..=2 {
            assert_eq!(next_frame_data(&mut body).await?, keep_alive_comment);
            assert_eq!(start.elapsed(), SSE_KEEP_ALIVE_INTERVAL * num_intervals);
        }
        Ok(())
    }

    #[tokio::test(start_paused = true)]
    async fn an_event_postpones_the_keep_alive_comment_by_an_interval() -> Result<()> {
        use futures::stream;
        use tokio::time::Instant;

        const EVENT: &[u8] = b"data: 1\n\n";
        const EVENT_DELAY: Duration = Duration::from_secs(10);

        let keep_alive_comment = axum_sse_body(vec![Event::DEFAULT_KEEP_ALIVE]).await;
        let event_stream = stream::once(async {
            tokio::time::sleep(EVENT_DELAY).await;
            Ok::<_, serde_json::Error>(Bytes::from_static(EVENT))
        })
        .chain(stream::pending());
        let start = Instant::now();
        let mut body = SseResponse::new(event_stream).into_response().into_body();

        assert_eq!(next_frame_data(&mut body).await?, EVENT);
        assert_eq!(next_frame_data(&mut body).await?, keep_alive_comment);
        assert_eq!(start.elapsed(), EVENT_DELAY + SSE_KEEP_ALIVE_INTERVAL);
        Ok(())
    }

    #[tokio::test(start_paused = true)]
    async fn an_sse_response_ends_when_its_event_stream_ends() -> Result<()> {
        use std::task::Poll;

        use futures::stream;
        use http_body_util::BodyExt;

        const EVENT: &[u8] = b"data: 1\n\n";

        let mut has_ended = false;
        let event_stream = stream::iter([Ok::<_, serde_json::Error>(Bytes::from_static(EVENT))])
            .chain(stream::poll_fn(move |_| {
                assert!(
                    !has_ended,
                    "the event stream shouldn't be polled after it has ended"
                );
                has_ended = true;
                Poll::Ready(None)
            }));
        let mut body = SseResponse::new(event_stream).into_response().into_body();

        assert_eq!(next_frame_data(&mut body).await?, EVENT);
        assert!(
            tokio::time::timeout(SSE_KEEP_ALIVE_INTERVAL.saturating_mul(2), body.frame())
                .await?
                .is_none()
        );
        Ok(())
    }

    /// Waits for the next frame of `body`, for longer than [`SSE_KEEP_ALIVE_INTERVAL`].
    ///
    /// # Returns
    ///
    /// The data of the frame on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`anyhow::Error`] if `body` ends, or its frame isn't data.
    /// * Forwards [`tokio::time::timeout`]'s return values on failure.
    /// * Forwards [`http_body_util::BodyExt::frame`]'s return values on failure.
    async fn next_frame_data(body: &mut axum::body::Body) -> Result<Bytes> {
        use anyhow::Context as _;
        use http_body_util::BodyExt;

        let frame = tokio::time::timeout(SSE_KEEP_ALIVE_INTERVAL.saturating_mul(2), body.frame())
            .await?
            .context("the body ended")??;
        frame
            .into_data()
            .map_err(|_| anyhow::anyhow!("the body's frame isn't data"))
    }
}

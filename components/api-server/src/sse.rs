//! Server-Sent Events encodings of streamed query results and of named events, and the response
//! that streams encoded events.

use std::pin::Pin;
use std::task::Context;
use std::task::Poll;
use std::time::Duration;

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

    fn new(archive_id: ArchiveId) -> Result<Self, serde_json::Error> {
        let mut event_prefix = b"data: {\"archive_id\":".to_vec();
        serde_json::to_writer(&mut event_prefix, &archive_id)?;
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
/// nothing for [`KEEP_ALIVE_INTERVAL`].
///
/// # Type Parameters
///
/// * `EventStreamType` - The stream of the encoded events, whose error ends the response.
pub struct SseResponse<EventStreamType> {
    events: KeepAliveStream<EventStreamType>,
}

impl<EventStreamType> SseResponse<EventStreamType> {
    /// Factory function.
    ///
    /// Starts the keep-alive interval.
    ///
    /// # Returns
    ///
    /// The newly created response.
    pub fn new(events: EventStreamType) -> Self {
        Self {
            events: KeepAliveStream {
                events,
                timer: tokio::time::sleep(KEEP_ALIVE_INTERVAL),
            },
        }
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
            Body::from_stream(self.events),
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
///
/// # Panics
///
/// Panics if `name` contains a line break, which would end the event's name field early.
pub fn encode_event<DataType: Serialize>(
    name: &str,
    data: &DataType,
) -> Result<Bytes, serde_json::Error> {
    assert!(
        !name.contains(['\n', '\r']),
        "an event's name mustn't contain a line break"
    );
    let mut event = b"event: ".to_vec();
    event.extend_from_slice(name.as_bytes());
    event.extend_from_slice(b"\ndata: ");
    serde_json::to_writer(&mut event, data)?;
    event.extend_from_slice(b"\n\n");
    Ok(Bytes::from(event))
}

/// The longest time a response sends nothing before it sends a keep-alive comment, so that proxies
/// don't close its idle connection. The same as the interval of
/// [`axum::response::sse::KeepAlive::default`].
const KEEP_ALIVE_INTERVAL: Duration = Duration::from_secs(15);

pin_project! {
    /// A stream of encoded events that also yields a keep-alive comment whenever it has yielded
    /// nothing for [`KEEP_ALIVE_INTERVAL`].
    ///
    /// # Type Parameters
    ///
    /// * `EventStreamType` - The stream of the encoded events.
    struct KeepAliveStream<EventStreamType> {
        #[pin]
        events: EventStreamType,
        #[pin]
        timer: Sleep,
    }
}

impl<EventStreamType: Stream<Item = Result<Bytes, serde_json::Error>>> Stream
    for KeepAliveStream<EventStreamType>
{
    type Item = Result<Bytes, serde_json::Error>;

    fn poll_next(self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Option<Self::Item>> {
        use std::task::ready;

        use tokio::time::Instant;

        const KEEP_ALIVE_COMMENT: &[u8] = b":\n\n";

        let mut this = self.project();
        match this.events.poll_next(cx) {
            Poll::Ready(Some(Ok(event))) => {
                this.timer.reset(Instant::now() + KEEP_ALIVE_INTERVAL);
                Poll::Ready(Some(Ok(event)))
            }
            Poll::Ready(end_or_error) => Poll::Ready(end_or_error),
            Poll::Pending => {
                // Polling the timer registers the task to be woken when it elapses, so that the
                // stream returns `Pending` only once both the events and the timer will wake it.
                ready!(this.timer.as_mut().poll(cx));
                this.timer.reset(Instant::now() + KEEP_ALIVE_INTERVAL);
                Poll::Ready(Some(Ok(Bytes::from_static(KEEP_ALIVE_COMMENT))))
            }
        }
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

    #[tokio::test]
    async fn encode_event_matches_axum_named_json_events() -> Result<()> {
        let data = serde_json::json!({"message": "a \"quoted\"\nmessage"});

        assert_eq!(
            encode_event("error", &data)?,
            axum_sse_body(vec![Event::default().event("error").json_data(&data)?]).await
        );
        Ok(())
    }

    #[test]
    fn encode_event_panics_if_the_name_contains_a_line_break() {
        for name in ["a\nb", "a\rb"] {
            let result = std::panic::catch_unwind(|| encode_event(name, &1));
            assert!(result.is_err(), "name: {name:?}");
        }
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
            assert_eq!(start.elapsed(), KEEP_ALIVE_INTERVAL * num_intervals);
        }
        Ok(())
    }

    #[tokio::test(start_paused = true)]
    async fn an_event_postpones_the_keep_alive_comment_by_an_interval() -> Result<()> {
        use futures::StreamExt;
        use futures::stream;
        use tokio::time::Instant;

        const EVENT: &[u8] = b"data: 1\n\n";
        const EVENT_DELAY: Duration = Duration::from_secs(10);

        let keep_alive_comment = axum_sse_body(vec![Event::DEFAULT_KEEP_ALIVE]).await;
        let events = stream::once(async {
            tokio::time::sleep(EVENT_DELAY).await;
            Ok::<_, serde_json::Error>(Bytes::from_static(EVENT))
        })
        .chain(stream::pending());
        let start = Instant::now();
        let mut body = SseResponse::new(events).into_response().into_body();

        assert_eq!(next_frame_data(&mut body).await?, EVENT);
        assert_eq!(next_frame_data(&mut body).await?, keep_alive_comment);
        assert_eq!(start.elapsed(), EVENT_DELAY + KEEP_ALIVE_INTERVAL);
        Ok(())
    }

    /// Waits for the next frame of `body`, for longer than [`KEEP_ALIVE_INTERVAL`].
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

        let frame = tokio::time::timeout(KEEP_ALIVE_INTERVAL.saturating_mul(2), body.frame())
            .await?
            .context("the body ended")??;
        frame
            .into_data()
            .map_err(|_| anyhow::anyhow!("the body's frame isn't data"))
    }
}

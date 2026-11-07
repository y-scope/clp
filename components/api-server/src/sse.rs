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

#[cfg(test)]
mod tests {
    use std::convert::Infallible;
    use std::pin::pin;
    use std::time::Duration;

    use axum::body::Body;
    use axum::body::Bytes;
    use axum::response::Sse;
    use axum::response::sse::Event;
    use clp_rust_utils::types::ArchiveId;
    use futures::FutureExt;
    use futures::StreamExt;
    use futures::stream;
    use serde::Serialize;
    use tokio::time::advance;

    use super::KeepAliveStream;

    const ARCHIVE_ID: &str = "018e90e5-8b2a-4a61-a2fc-cac799936caf";

    const INTERVAL: Duration = Duration::from_millis(50);

    /// The smallest step of tokio's timer.
    const TICK: Duration = Duration::from_millis(1);

    /// The data of a query result's event.
    #[derive(Serialize)]
    struct StreamingQueryResult {
        archive_id: ArchiveId,
        timestamp: i64,
        message: String,
    }

    /// # Returns
    ///
    /// Messages whose JSON escaping or trailing newlines an event encoding could get wrong, each
    /// paired with a timestamp at the edge of its range in turn.
    fn tricky_results() -> Vec<(i64, &'static str)> {
        const TRICKY_MESSAGES: &[&str] = &[
            "{\"quoted\":\"value\"}\n",
            "back\\slash\n",
            "nul \u{0} unit separator \u{1f} delete \u{7f}\n",
            "tab\t backspace\u{8} form feed\u{c}\n",
            "line separator \u{2028} paragraph separator \u{2029}\n",
            "non-ASCII é 日本 🦀\n",
            "x\n",
            "x\n\n",
            "x\r\n",
            "inner\nnewline",
            "",
            "\n",
        ];
        const TRICKY_TIMESTAMPS: &[i64] = &[i64::MIN, -1, 0, i64::MAX];

        TRICKY_TIMESTAMPS
            .iter()
            .copied()
            .cycle()
            .zip(TRICKY_MESSAGES.iter().copied())
            .collect()
    }

    /// # Returns
    ///
    /// The event that [`Event::json_data`] encodes for a result.
    ///
    /// # Panics
    ///
    /// Panics if serializing the result fails.
    fn axum_result_event(archive_id: ArchiveId, timestamp_millisecs: i64, message: &str) -> Event {
        let mut message = message.to_owned();
        if message.ends_with('\n') {
            message.pop();
        }
        Event::default()
            .json_data(StreamingQueryResult {
                archive_id,
                timestamp: timestamp_millisecs,
                message,
            })
            .expect("serializing a result shouldn't fail")
    }

    /// # Returns
    ///
    /// The body of an axum [`Sse`] response streaming `events`.
    ///
    /// # Panics
    ///
    /// Panics if the body can't be read or isn't UTF-8.
    async fn axum_sse_body(events: Vec<Event>) -> String {
        use axum::response::IntoResponse;
        use http_body_util::BodyExt;

        let bytes = Sse::new(stream::iter(events.into_iter().map(Ok::<_, Infallible>)))
            .into_response()
            .into_body()
            .collect()
            .await
            .expect("the body should be readable")
            .to_bytes();
        String::from_utf8(bytes.to_vec()).expect("the body should be UTF-8")
    }

    /// # Returns
    ///
    /// `bytes` as a string.
    ///
    /// # Panics
    ///
    /// Panics if `bytes` isn't UTF-8.
    fn utf8(bytes: &Bytes) -> String {
        String::from_utf8(bytes.to_vec()).expect("the bytes should be UTF-8")
    }

    /// # Returns
    ///
    /// The data of the frame that `body` yields without waiting, or `None` if no frame is ready.
    ///
    /// # Panics
    ///
    /// Panics if the body ends, can't be read, or yields a frame that doesn't hold data.
    fn ready_data(body: &mut Body) -> Option<Bytes> {
        use http_body_util::BodyExt;

        body.frame().now_or_never().map(|frame| {
            frame
                .expect("the body shouldn't end")
                .expect("the body should be readable")
                .into_data()
                .expect("the frame should hold data")
        })
    }

    #[tokio::test]
    async fn sse_encoder_matches_axum_json_events() -> anyhow::Result<()> {
        use query_result_listener::encoding::ResultEncoder;
        use query_result_listener::encoding::ResultEncoding;

        use super::SseEncoding;

        let archive_id = ARCHIVE_ID.parse::<ArchiveId>()?;
        let mut encoder = <SseEncoding as ResultEncoding>::Encoder::new(archive_id);
        let mut axum_events = Vec::new();
        for (timestamp_millisecs, message) in tricky_results() {
            encoder.encode(timestamp_millisecs, message);
            axum_events.push(axum_result_event(archive_id, timestamp_millisecs, message));
        }

        assert_eq!(
            utf8(&encoder.take_batch()),
            axum_sse_body(axum_events).await
        );
        Ok(())
    }

    #[tokio::test]
    async fn sse_encoder_starts_the_next_batch_empty() -> anyhow::Result<()> {
        use query_result_listener::encoding::ResultEncoder;
        use query_result_listener::encoding::ResultEncoding;

        use super::SseEncoding;

        let archive_id = ARCHIVE_ID.parse::<ArchiveId>()?;
        let mut encoder = <SseEncoding as ResultEncoding>::Encoder::new(archive_id);
        encoder.encode(1, "first\n");
        encoder.take_batch();
        encoder.encode(2, "second\n");

        assert_eq!(
            utf8(&encoder.take_batch()),
            axum_sse_body(vec![axum_result_event(archive_id, 2, "second\n")]).await
        );
        Ok(())
    }

    #[tokio::test]
    async fn named_events_match_axum() -> anyhow::Result<()> {
        use serde_json::json;

        use super::encode_event;

        let error = json!({"message": "failed to wait for the \"query\" job\nto terminate"});

        assert_eq!(
            utf8(&encode_event("error", &error)?),
            axum_sse_body(vec![Event::default().event("error").json_data(&error)?]).await
        );
        Ok(())
    }

    #[tokio::test(start_paused = true)]
    async fn idle_response_sends_keep_alive_comments() -> anyhow::Result<()> {
        use axum::response::IntoResponse;
        use axum::response::sse::KeepAlive;
        use http_body_util::BodyExt;
        use tokio::time::timeout;

        const TIMEOUT: Duration = Duration::from_secs(10);

        let mut axum_body = Sse::new(stream::pending::<Result<Event, Infallible>>())
            .keep_alive(KeepAlive::new().interval(INTERVAL))
            .into_response()
            .into_body();
        let axum_comment = timeout(TIMEOUT, axum_body.frame())
            .await?
            .expect("the axum body shouldn't end")?
            .into_data()
            .expect("the axum frame should hold data");
        let mut keep_alive = pin!(KeepAliveStream::new(
            stream::pending::<Result<Bytes, Infallible>>(),
            INTERVAL
        ));

        for _ in 0..2 {
            assert_eq!(keep_alive.next().now_or_never(), None);
            advance(INTERVAL.saturating_sub(TICK)).await;
            assert_eq!(keep_alive.next().now_or_never(), None);
            advance(TICK).await;
            assert_eq!(
                keep_alive.next().now_or_never(),
                Some(Some(Ok(axum_comment.clone())))
            );
        }
        Ok(())
    }

    #[tokio::test(start_paused = true)]
    async fn event_postpones_the_next_keep_alive_comment_by_the_interval() -> anyhow::Result<()> {
        use tokio::time::sleep;

        const EVENT: &[u8] = b"data: 1\n\n";

        let events = stream::once(async {
            sleep(INTERVAL * 3 / 5).await;
            Ok::<_, Infallible>(Bytes::from_static(EVENT))
        })
        .chain(stream::pending());
        let mut keep_alive = pin!(KeepAliveStream::new(events, INTERVAL));

        assert_eq!(keep_alive.next().now_or_never(), None);
        advance(INTERVAL * 3 / 5).await;
        assert_eq!(
            keep_alive.next().now_or_never(),
            Some(Some(Ok(Bytes::from_static(EVENT))))
        );
        advance(INTERVAL.saturating_sub(TICK)).await;
        assert_eq!(keep_alive.next().now_or_never(), None);
        advance(TICK).await;
        assert_eq!(
            keep_alive.next().now_or_never(),
            Some(Some(Ok(Bytes::from_static(b":\n\n"))))
        );
        Ok(())
    }

    #[tokio::test(start_paused = true)]
    async fn sse_response_is_laid_out_as_axum_sse_with_default_keep_alive() -> anyhow::Result<()> {
        use axum::http::header::CACHE_CONTROL;
        use axum::http::header::CONTENT_TYPE;
        use axum::response::IntoResponse;
        use axum::response::sse::KeepAlive;

        use super::SseResponse;

        /// The interval of [`KeepAlive::default`].
        const DEFAULT_INTERVAL: Duration = Duration::from_secs(15);

        let response =
            SseResponse::new(stream::pending::<Result<Bytes, Infallible>>()).into_response();
        let axum_response = Sse::new(stream::pending::<Result<Event, Infallible>>())
            .keep_alive(KeepAlive::default())
            .into_response();
        for header in [CONTENT_TYPE, CACHE_CONTROL] {
            assert_eq!(
                response.headers().get(&header),
                axum_response.headers().get(&header)
            );
        }

        let mut body = response.into_body();
        let mut axum_body = axum_response.into_body();
        assert_eq!(ready_data(&mut body), None);
        assert_eq!(ready_data(&mut axum_body), None);
        advance(DEFAULT_INTERVAL.saturating_sub(TICK)).await;
        assert_eq!(ready_data(&mut body), None);
        assert_eq!(ready_data(&mut axum_body), None);
        advance(TICK).await;
        let axum_comment =
            ready_data(&mut axum_body).expect("axum should send a keep-alive comment by now");
        assert_eq!(ready_data(&mut body), Some(axum_comment));
        Ok(())
    }

    #[tokio::test]
    async fn keep_alive_stream_forwards_errors_and_ends_with_its_events() -> anyhow::Result<()> {
        const ERROR: &str = "failed to stream the events";

        let mut keep_alive = pin!(KeepAliveStream::new(
            stream::iter([Err::<Bytes, _>(ERROR)]),
            INTERVAL
        ));

        assert_eq!(keep_alive.next().now_or_never(), Some(Some(Err(ERROR))));
        assert_eq!(keep_alive.next().now_or_never(), Some(None));
        Ok(())
    }
}

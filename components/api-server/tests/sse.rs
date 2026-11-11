use anyhow::Result;
use api_server::sse::SseEncoding;
use axum::body::Bytes;
use axum::response::sse::Event;
use clp_rust_utils::types::ArchiveId;
use query_result_listener::encoding::ResultEncoder;
use query_result_listener::encoding::ResultEncoding;
use query_result_listener::test_utils::ARCHIVE_ID;
use serde::Serialize;

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
fn axum_result_event(archive_id: ArchiveId, timestamp_millisecs: i64, message: String) -> Event {
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

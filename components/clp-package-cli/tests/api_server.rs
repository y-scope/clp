//! End-to-end tests of [`Client`] against an HTTP server, bound to a loopback port, that serves
//! canned responses of the API server's streaming query route.

use std::net::Ipv4Addr;
use std::time::Duration;

use anyhow::Result;
use axum::Json;
use axum::Router;
use axum::body::Body;
use axum::http::HeaderMap;
use axum::http::StatusCode;
use axum::http::header::ACCEPT;
use axum::response::IntoResponse;
use axum::response::Response;
use axum::routing::post;
use clp_package_cli::api_server::Client;
use clp_package_cli::api_server::Error;
use clp_package_cli::api_server::Event;
use clp_rust_utils::api_server::streaming_query::StreamingQueryConfig;
use clp_rust_utils::api_server::streaming_query::StreamingQueryEndEvent;
use clp_rust_utils::api_server::streaming_query::StreamingQueryJobEvent;
use clp_rust_utils::job_config::QueryJobStatus;
use clp_rust_utils::job_config::SearchQuery;
use futures::Stream;
use futures::StreamExt;
use url::Url;

/// The longest time a test waits for the client's stream of events to end.
const TEST_TIMEOUT: Duration = Duration::from_secs(10);

const JOB_EVENT: &str = "event: job\ndata: {\"query_job_id\":1}\n\n";

const RESULT: &str = concat!(
    r#"{"archive_id":"018e90e5-8b2a-4a61-a2fc-cac799936caf","#,
    r#""timestamp_millisecs":0,"message":"log"}"#
);

#[tokio::test]
async fn client_streams_the_events_of_a_successful_query() -> Result<()> {
    const END_EVENT: &str =
        "event: end\ndata: {\"status\":\"Succeeded\",\"num_results_emitted\":2}\n\n";
    const KEEP_ALIVE_COMMENT: &str = ":\n\n";

    let result_event = format!("data: {RESULT}\n\n");
    let (first_half, second_half) = result_event.split_at(result_event.len() / 2);
    let chunks = vec![
        JOB_EVENT.to_owned(),
        KEEP_ALIVE_COMMENT.to_owned(),
        result_event.clone(),
        KEEP_ALIVE_COMMENT.to_owned(),
        first_half.to_owned(),
        second_half.to_owned(),
        END_EVENT.to_owned(),
    ];
    let router = Router::new().route(
        "/query/stream",
        post(
            move |headers: HeaderMap, Json(config): Json<StreamingQueryConfig>| {
                let chunks = chunks.clone();
                async move {
                    if headers
                        .get(ACCEPT)
                        .is_none_or(|accept| accept != "text/event-stream")
                        || config.query != search_query()
                    {
                        return (StatusCode::BAD_REQUEST, "unexpected request").into_response();
                    }
                    event_stream_response(futures::stream::iter(chunks.into_iter().map(Ok)))
                }
            },
        ),
    );

    let mut events = collect_events(router).await?.into_iter();

    let Some(Ok(Event::Job(StreamingQueryJobEvent { query_job_id }))) = events.next() else {
        anyhow::bail!(
            "the first event should be the `job` event, unless the request is unexpected"
        );
    };
    assert_eq!(query_job_id, 1);
    for _ in 0..2 {
        let Some(Ok(Event::Result(data))) = events.next() else {
            anyhow::bail!("the `job` event should be followed by two results");
        };
        assert_eq!(data, RESULT);
    }
    let Some(Ok(Event::End(StreamingQueryEndEvent {
        status,
        num_results_emitted,
    }))) = events.next()
    else {
        anyhow::bail!("the results should be followed by the `end` event");
    };
    assert_eq!(status, QueryJobStatus::Succeeded);
    assert_eq!(num_results_emitted, 2);
    assert!(
        events.next().is_none(),
        "the stream should end after the `end` event"
    );
    Ok(())
}

#[tokio::test]
async fn client_yields_the_status_and_body_of_a_rejected_query() -> Result<()> {
    const BODY: &str = "Failed to deserialize the JSON body into the target type: query.datasets: \
                        invalid dataset name \"a-b\"";

    let router = Router::new().route(
        "/query/stream",
        post(|| async { (StatusCode::UNPROCESSABLE_ENTITY, BODY) }),
    );

    let events = collect_events(router).await?;

    let [Err(error)] = events.as_slice() else {
        anyhow::bail!("the stream should yield only an error");
    };
    assert!(
        matches!(
            error,
            Error::Status { status, body }
                if *status == StatusCode::UNPROCESSABLE_ENTITY && body == BODY
        ),
        "unexpected error: {error:?}"
    );
    Ok(())
}

#[tokio::test]
async fn client_yields_an_error_if_the_response_is_cut_before_its_end() -> Result<()> {
    let router = Router::new().route(
        "/query/stream",
        post(|Json(_): Json<StreamingQueryConfig>| async {
            let events = futures::stream::iter([
                Ok(JOB_EVENT.to_owned()),
                Ok(format!("data: {RESULT}\n\n")),
            ]);
            // Yielding before the error lets the server send the events before it cuts the
            // response.
            let cut = futures::stream::once(async {
                tokio::task::yield_now().await;
                Err(std::io::Error::other("the response is cut"))
            });
            event_stream_response(events.chain(cut))
        }),
    );

    let mut events = collect_events(router).await?.into_iter();

    assert!(matches!(events.next(), Some(Ok(Event::Job(_)))));
    assert!(matches!(events.next(), Some(Ok(Event::Result(_)))));
    let Some(Err(error)) = events.next() else {
        anyhow::bail!("the result should be followed by an error");
    };
    assert!(
        matches!(error, Error::Body(_)),
        "unexpected error: {error:?}"
    );
    assert!(
        events.next().is_none(),
        "the stream should end after its error"
    );
    Ok(())
}

/// # Returns
///
/// The query that the tests submit.
///
/// # Panics
///
/// Panics if the query doesn't deserialize.
fn search_query() -> SearchQuery {
    serde_json::from_value(serde_json::json!({
        "datasets": ["default", "other"],
        "query_string": "level: ERROR",
        "time_range": {"begin_timestamp_millisecs": 0, "end_timestamp_millisecs": 1},
        "ignore_case": true,
    }))
    .expect("the query should deserialize")
}

/// # Returns
///
/// A Server-Sent Events response whose body is made of the chunks of `body`. An error cuts the
/// response, as the API server's response is cut when it fails to encode an event.
fn event_stream_response(
    body: impl Stream<Item = Result<String, std::io::Error>> + Send + 'static,
) -> Response {
    (
        [("content-type", "text/event-stream")],
        Body::from_stream(body),
    )
        .into_response()
}

/// Serves `router` on a loopback port while a [`Client`] streams the events of [`search_query`]'s
/// query from it.
///
/// # Returns
///
/// Every item that the client's stream yields before it ends on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`anyhow::Error`] if the server stops.
/// * Forwards [`tokio::net::TcpListener::bind`]'s return values on failure.
/// * Forwards [`tokio::net::TcpListener::local_addr`]'s return values on failure.
/// * Forwards [`Url::parse`]'s return values on failure.
/// * Forwards [`Client::new`]'s return values on failure.
/// * Forwards [`tokio::time::timeout`]'s return values on failure.
async fn collect_events(router: Router) -> Result<Vec<Result<Event, Error>>> {
    let listener = tokio::net::TcpListener::bind((Ipv4Addr::LOCALHOST, 0)).await?;
    let client = Client::new(Url::parse(&format!("http://{}", listener.local_addr()?))?)?;
    let config = StreamingQueryConfig {
        query: search_query(),
    };
    let events = tokio::time::timeout(
        TEST_TIMEOUT,
        client.stream_query(&config).collect::<Vec<_>>(),
    );

    tokio::select! {
        result = axum::serve(listener, router).into_future() => {
            anyhow::bail!("the server stopped: {result:?}")
        }
        events = events => Ok(events?),
    }
}

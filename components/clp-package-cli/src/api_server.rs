//! A client of the API server's streaming query route, `POST /query/stream`, which streams the
//! events of the query's job.

use std::time::Duration;

use bytes::Bytes;
use clp_rust_utils::api_server::streaming_query::StreamingQueryConfig;
use clp_rust_utils::api_server::streaming_query::StreamingQueryEndEvent;
use clp_rust_utils::api_server::streaming_query::StreamingQueryErrorEvent;
use clp_rust_utils::api_server::streaming_query::StreamingQueryJobEvent;
use futures::Stream;
use futures::TryStreamExt;
use serde::de::DeserializeOwned;
use url::Url;

use crate::sse;

/// A client of the API server's streaming query route.
pub struct Client {
    http_client: reqwest::Client,
    stream_query_url: Url,
}

impl Client {
    /// Factory function.
    ///
    /// # Returns
    ///
    /// A client of the API server at `api_server_url` on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`BuildClientError`] if [`reqwest::ClientBuilder::build`] fails.
    pub fn new(mut api_server_url: Url) -> Result<Self, BuildClientError> {
        // `reqwest` sends requests through the proxy that `HTTP_PROXY` names, which would intercept
        // the requests to an API server on `localhost`.
        let http_client = reqwest::Client::builder()
            .no_proxy()
            .connect_timeout(CONNECT_TIMEOUT)
            .read_timeout(READ_TIMEOUT)
            .build()
            .map_err(|source| BuildClientError { source })?;
        api_server_url.set_path("/query/stream");
        Ok(Self {
            http_client,
            stream_query_url: api_server_url,
        })
    }

    /// Submits a query job through the streaming query route, and streams the job's events.
    ///
    /// The request is sent when the returned stream is first polled. Dropping the stream closes the
    /// connection, after which the API server requests the job's cancellation.
    ///
    /// # Returns
    ///
    /// A stream of the job's events, in the order the API server sends them. The stream ends when
    /// the response does, or after yielding its first error, which is:
    ///
    /// * [`Error::Request`] if the request can't be sent.
    /// * [`Error::Status`] if the API server responds with an unsuccessful status.
    /// * [`Error::Body`] if the response's body can't be read.
    /// * [`Error::UnexpectedLine`] if the response's body holds a line that the API server doesn't
    ///   send.
    /// * [`Error::UnknownEvent`] if an event's name is unknown.
    /// * [`Error::InvalidEventData`] if a named event's data doesn't deserialize.
    pub fn stream_query(
        &self,
        streaming_query_config: &StreamingQueryConfig,
    ) -> impl Stream<Item = Result<Event, Error>> + Send + use<> {
        let request = self
            .http_client
            .post(self.stream_query_url.clone())
            .header(reqwest::header::ACCEPT, "text/event-stream")
            .json(streaming_query_config);
        futures::stream::once(send(request))
            .map_ok(|response| {
                futures::stream::try_unfold(
                    ResponseBody {
                        response,
                        reader: sse::Reader::default(),
                    },
                    |mut body| async move {
                        Ok::<_, Error>(body.next_event().await?.map(|event| (event, body)))
                    },
                )
            })
            .try_flatten()
    }
}

/// An event of a streaming query.
pub enum Event {
    /// The `job` event, which comes first and identifies the query's job.
    Job(StreamingQueryJobEvent),

    /// An unnamed event, whose data is a result: a [`StreamingQueryResultEvent`] serialized as
    /// JSON, as the API server sent it.
    ///
    /// [`StreamingQueryResultEvent`]:
    ///   clp_rust_utils::api_server::streaming_query::StreamingQueryResultEvent
    Result(Bytes),

    /// The `end` event, which ends the stream with the job's terminal status.
    End(StreamingQueryEndEvent),

    /// The `error` event, which ends the stream in place of `end` if the API server fails after
    /// submitting the job.
    Error(StreamingQueryErrorEvent),
}

impl TryFrom<sse::Event> for Event {
    type Error = Error;

    fn try_from(event: sse::Event) -> Result<Self, Error> {
        let sse::Event { name, data } = event;
        match name.as_deref() {
            None => Ok(Self::Result(data)),
            Some(b"job") => deserialize_data("job", &data).map(Self::Job),
            Some(b"end") => deserialize_data("end", &data).map(Self::End),
            Some(b"error") => deserialize_data("error", &data).map(Self::Error),
            Some(name) => Err(Error::UnknownEvent {
                name: String::from_utf8_lossy(name).into_owned(),
            }),
        }
    }
}

/// An error that ends a stream of a streaming query's events.
#[derive(Debug, thiserror::Error)]
pub enum Error {
    #[error("failed to send the query to the API server: {}", display_with_sources(.0))]
    Request(#[source] reqwest::Error),

    #[error("the API server responded with status {status}: {body}")]
    Status {
        status: reqwest::StatusCode,
        body: String,
    },

    #[error("failed to read the API server's response: {}", display_with_sources(.0))]
    Body(#[source] reqwest::Error),

    #[error("the API server sent an unexpected line {line:?}")]
    UnexpectedLine { line: String },

    #[error("the API server sent an event with the unknown name {name:?}")]
    UnknownEvent { name: String },

    #[error("the API server sent a `{name}` event whose data is invalid: {source}")]
    InvalidEventData {
        name: &'static str,
        source: serde_json::Error,
    },
}

impl From<sse::UnexpectedLineError> for Error {
    fn from(error: sse::UnexpectedLineError) -> Self {
        Self::UnexpectedLine {
            line: String::from_utf8_lossy(&error.line).into_owned(),
        }
    }
}

/// An error returned when a [`Client`]'s HTTP client can't be built.
#[derive(Debug, thiserror::Error)]
#[error("failed to build the HTTP client: {}", display_with_sources(.source))]
pub struct BuildClientError {
    source: reqwest::Error,
}

/// The longest time that connecting to the API server may take, so that a stopped package fails
/// fast.
const CONNECT_TIMEOUT: Duration = Duration::from_secs(5);

/// The longest time that a read from the API server may take, which is longer than the interval of
/// the API server's keep-alive comments, so that a dead API server fails the search instead of
/// hanging it.
const READ_TIMEOUT: Duration = Duration::from_secs(60);

/// The longest body of an unsuccessful response that is read, so that the API server can't make the
/// client buffer without bound.
const MAX_ERROR_BODY_SIZE: usize = 64 * 1024;

/// The body of a successful response of the streaming query route.
struct ResponseBody {
    response: reqwest::Response,
    reader: sse::Reader,
}

impl ResponseBody {
    /// Reads the next event of the body.
    ///
    /// # Returns
    ///
    /// The next event, or `None` if the body ends, on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`Error::Body`] if the body can't be read.
    /// * Forwards [`sse::Reader::next_event`]'s return values on failure.
    /// * Forwards [`Event::try_from`]'s return values on failure.
    async fn next_event(&mut self) -> Result<Option<Event>, Error> {
        loop {
            if let Some(event) = self.reader.next_event()? {
                return Event::try_from(event).map(Some);
            }
            let Some(chunk) = self.response.chunk().await.map_err(Error::Body)? else {
                return Ok(None);
            };
            self.reader.push(&chunk);
        }
    }
}

/// Sends `request`, a request to the streaming query route.
///
/// # Returns
///
/// The response on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`Error::Request`] if the request can't be sent.
/// * [`Error::Status`] if the API server responds with an unsuccessful status.
/// * [`Error::Body`] if the body of an unsuccessful response can't be read.
async fn send(request: reqwest::RequestBuilder) -> Result<reqwest::Response, Error> {
    let response = request.send().await.map_err(Error::Request)?;
    let status = response.status();
    if status.is_success() {
        return Ok(response);
    }
    let body = read_error_body(response).await.map_err(Error::Body)?;
    Err(Error::Status { status, body })
}

/// Reads the body of an unsuccessful response, up to [`MAX_ERROR_BODY_SIZE`] bytes.
///
/// # Returns
///
/// The body, with invalid UTF-8 sequences replaced, on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`reqwest::Response::chunk`]'s return values on failure.
async fn read_error_body(mut response: reqwest::Response) -> Result<String, reqwest::Error> {
    let mut body = Vec::new();
    while body.len() < MAX_ERROR_BODY_SIZE
        && let Some(chunk) = response.chunk().await?
    {
        body.extend_from_slice(&chunk);
    }
    body.truncate(MAX_ERROR_BODY_SIZE);
    Ok(String::from_utf8_lossy(&body).into_owned())
}

/// Deserializes `data`, the data of the event named `name`.
///
/// # Type Parameters
///
/// * `DataType` - The type of the event's data.
///
/// # Returns
///
/// The deserialized data on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`Error::InvalidEventData`] if `data` doesn't deserialize into a `DataType`.
fn deserialize_data<DataType: DeserializeOwned>(
    name: &'static str,
    data: &[u8],
) -> Result<DataType, Error> {
    serde_json::from_slice(data).map_err(|source| Error::InvalidEventData { name, source })
}

/// # Returns
///
/// The message of `error` followed by the messages of its sources, each after `: `, since a
/// [`reqwest::Error`]'s message leaves out its source, such as why a connection failed.
fn display_with_sources(error: &(dyn std::error::Error + 'static)) -> String {
    std::iter::successors(Some(error), |error| error.source())
        .map(ToString::to_string)
        .collect::<Vec<_>>()
        .join(": ")
}

#[cfg(test)]
mod tests {
    use clp_rust_utils::job_config::QueryJobStatus;

    use super::*;

    /// An error whose source is the next error of a chain.
    #[derive(Debug, thiserror::Error)]
    #[error("{message}")]
    struct ChainedError {
        message: &'static str,
        #[source]
        source: Option<Box<Self>>,
    }

    /// # Returns
    ///
    /// The result of converting the event named `name` whose data is `data`.
    fn convert(name: Option<&'static str>, data: &'static str) -> Result<Event, Error> {
        Event::try_from(sse::Event {
            name: name.map(|name| Bytes::from_static(name.as_bytes())),
            data: Bytes::from_static(data.as_bytes()),
        })
    }

    #[test]
    fn event_from_sse_event_maps_each_event_name() -> anyhow::Result<()> {
        const RESULT: &str = r#"{"archive_id":"a","timestamp_millisecs":0,"message":"m"}"#;

        let Event::Job(job) = convert(Some("job"), r#"{"query_job_id":1}"#)? else {
            anyhow::bail!("a `job` event should be a job event");
        };
        assert_eq!(job.query_job_id, 1);

        let Event::Result(result) = convert(None, RESULT)? else {
            anyhow::bail!("an unnamed event should be a result");
        };
        assert_eq!(result, RESULT);

        let Event::End(end) = convert(
            Some("end"),
            r#"{"status":"Failed","num_results_emitted":2}"#,
        )?
        else {
            anyhow::bail!("an `end` event should be an end event");
        };
        assert_eq!(end.status, QueryJobStatus::Failed);
        assert_eq!(end.num_results_emitted, 2);

        let Event::Error(error) = convert(Some("error"), r#"{"message":"failed"}"#)? else {
            anyhow::bail!("an `error` event should be an error event");
        };
        assert_eq!(error.message, "failed");
        Ok(())
    }

    #[test]
    fn event_from_sse_event_rejects_an_unknown_event_name() {
        let error = convert(Some("results"), "{}")
            .err()
            .expect("an unknown event name should be rejected");

        assert!(
            matches!(&error, Error::UnknownEvent { name } if name == "results"),
            "unexpected error: {error:?}"
        );
    }

    #[test]
    fn event_from_sse_event_rejects_data_that_doesnt_deserialize() {
        for (name, data) in [
            ("job", r#"{"query_job_id":-1}"#),
            ("end", r#"{"status":"Done","num_results_emitted":0}"#),
            ("error", r#""failed""#),
        ] {
            let error = convert(Some(name), data)
                .err()
                .expect("invalid event data should be rejected");

            assert!(
                matches!(
                    &error,
                    Error::InvalidEventData { name: error_name, .. } if *error_name == name
                ),
                "unexpected error: {error:?}"
            );
        }
    }

    #[tokio::test]
    async fn read_error_body_reads_at_most_the_maximum_size() -> anyhow::Result<()> {
        for (body_size, expected_body_size) in [
            (0, 0),
            (MAX_ERROR_BODY_SIZE, MAX_ERROR_BODY_SIZE),
            (MAX_ERROR_BODY_SIZE + 1, MAX_ERROR_BODY_SIZE),
        ] {
            let response =
                reqwest::Response::from(axum::http::Response::new(vec![b'x'; body_size]));

            assert_eq!(
                read_error_body(response).await?,
                "x".repeat(expected_body_size),
                "body size: {body_size}"
            );
        }
        Ok(())
    }

    #[test]
    fn display_with_sources_displays_every_error_of_the_chain() {
        let error = ChainedError {
            message: "error sending request",
            source: Some(Box::new(ChainedError {
                message: "tcp connect error",
                source: Some(Box::new(ChainedError {
                    message: "Connection refused",
                    source: None,
                })),
            })),
        };

        assert_eq!(
            display_with_sources(&error),
            "error sending request: tcp connect error: Connection refused"
        );
    }
}

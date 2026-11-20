//! Streaming queries, whose results are sent as Server-Sent Events.

use axum::body::Bytes;
use clp_rust_utils::api_server::streaming_query::StreamingQueryEndEvent;
use clp_rust_utils::api_server::streaming_query::StreamingQueryErrorEvent;
use clp_rust_utils::api_server::streaming_query::StreamingQueryJobEvent;
use clp_rust_utils::job_config::QueryJobConfig;
use clp_rust_utils::job_config::QueryOutput;
use clp_rust_utils::job_config::SearchQuery;
use futures::Stream;
use futures::StreamExt;
use query_result_listener::session::SessionConfig;
use query_result_listener::session::SessionEvent;
use query_result_listener::session::SessionStats;

use crate::client::Client;
use crate::error::ClientError;
use crate::sse::encode_event;

impl Client {
    /// Submits a query job that runs `query` and streams its results to a new session of the
    /// client's query result listener.
    ///
    /// If the query fails, or the returned stream is dropped before the job terminates, the job's
    /// cancellation is requested.
    ///
    /// # Returns
    ///
    /// A stream of the job's events, encoded as Server-Sent Events, on success:
    ///
    /// * `job`, the first event, whose [`StreamingQueryJobEvent`] payload identifies the query job.
    /// * One unnamed event per result, whose [`StreamingQueryResultEvent`] payload carries the
    ///   result. Results from different archives can arrive in any order.
    /// * `end`, which closes the stream and reports the job's terminal status in a
    ///   [`StreamingQueryEndEvent`] payload.
    /// * `error`, which closes the stream in place of `end` if an error occurs after the job is
    ///   submitted, with a [`StreamingQueryErrorEvent`] payload.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`ResultListener::open_session`]'s return values on failure.
    /// * Forwards [`Client::submit_query`]'s return values on failure.
    ///
    /// # Cancel safety
    ///
    /// This method isn't cancel safe. If the returned future is dropped while the job is being
    /// submitted, the job may still be inserted: its tasks then can't stream their results, since
    /// the session is closed, and its cancellation isn't requested.
    ///
    /// [`StreamingQueryResultEvent`]:
    ///   clp_rust_utils::api_server::streaming_query::StreamingQueryResultEvent
    /// [`ResultListener::open_session`]:
    ///   query_result_listener::listener::ResultListener::open_session
    pub(crate) async fn submit_streaming_query(
        &self,
        query: SearchQuery,
    ) -> Result<impl Stream<Item = Result<Bytes, serde_json::Error>> + Send + use<>, ClientError>
    {
        let open_session = self
            .query_result_listener
            .open_session(SessionConfig::default())?;
        let query_job_config = QueryJobConfig {
            query,
            output: QueryOutput::Network(open_session.network_output().clone()),
        };
        let query_job_id = self.submit_query(&query_job_config).await?;
        tracing::info!(query_job_id, "Submitted a streaming query.");

        let wait_for_termination = {
            let client = self.clone();
            async move { client.wait_for_query_job_termination(query_job_id).await }
        };
        let request_cancellation = {
            let client = self.clone();
            async move {
                match client.cancel_query_job(query_job_id).await {
                    Ok(()) => tracing::info!(
                        query_job_id,
                        "Requested the cancellation of a streaming query's job."
                    ),
                    Err(ClientError::QueryJobAlreadyTerminated(_)) => tracing::info!(
                        query_job_id,
                        "Skipped the cancellation of a streaming query's job, which had already \
                         terminated."
                    ),
                    Err(error) => tracing::warn!(
                        query_job_id,
                        error = % error,
                        "Failed to request the cancellation of a streaming query's job."
                    ),
                }
            }
        };
        Ok(encode_streaming_query_events(
            query_job_id,
            open_session.into_events(wait_for_termination, request_cancellation),
        ))
    }
}

/// Encodes `event_stream`, a stream of the events of a streaming query's job, as Server-Sent
/// Events.
///
/// # Returns
///
/// A stream that yields a `job` event whose data is a [`StreamingQueryJobEvent`], then the events
/// that [`encode_session_event`] encodes.
fn encode_streaming_query_events(
    query_job_id: u64,
    event_stream: impl Stream<Item = SessionEvent<Bytes>>,
) -> impl Stream<Item = Result<Bytes, serde_json::Error>> {
    let job_event = encode_event("job", &StreamingQueryJobEvent { query_job_id });
    futures::stream::once(futures::future::ready(job_event))
        .chain(event_stream.map(move |event| encode_session_event(query_job_id, event)))
}

/// Encodes `event`, an event of the streaming query whose job is `query_job_id`, as a Server-Sent
/// Event.
///
/// A [`SessionEvent::Ended`] is logged with `query_job_id` and the session's statistics, of which
/// only the number of results streamed is sent to the client. A [`SessionEvent::Failed`] is logged
/// with `query_job_id`, and its error isn't sent to the client.
///
/// # Returns
///
/// On success:
///
/// * The batch of [`SessionEvent::Results`] as it is.
/// * An `end` event whose data is a [`StreamingQueryEndEvent`] for [`SessionEvent::Ended`].
/// * An `error` event whose data is a [`StreamingQueryErrorEvent`] for [`SessionEvent::Failed`].
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`encode_event`]'s return values on failure.
fn encode_session_event(
    query_job_id: u64,
    event: SessionEvent<Bytes>,
) -> Result<Bytes, serde_json::Error> {
    const STREAMING_QUERY_ERROR_MESSAGE: &str = "Internal server error";

    match event {
        SessionEvent::Results(batch) => Ok(batch),
        SessionEvent::Ended { status, stats } => {
            let SessionStats {
                num_results_emitted,
                num_duplicates_dropped,
            } = stats;
            tracing::info!(
                query_job_id,
                status = ? status,
                num_results_emitted,
                num_duplicates_dropped,
                "Streaming query ended."
            );
            encode_event(
                "end",
                &StreamingQueryEndEvent {
                    status,
                    num_results_emitted,
                },
            )
        }
        SessionEvent::Failed(error) => {
            tracing::error!(query_job_id, error = % error, "A streaming query failed.");
            encode_event(
                "error",
                &StreamingQueryErrorEvent {
                    message: STREAMING_QUERY_ERROR_MESSAGE.to_owned(),
                },
            )
        }
    }
}

#[cfg(test)]
mod tests {
    use clp_rust_utils::job_config::QueryJobStatus;
    use futures::TryStreamExt;

    use super::*;

    /// The ID of the query job whose events a test encodes.
    const QUERY_JOB_ID: u64 = 7;

    #[tokio::test]
    async fn streaming_query_events_are_the_job_event_the_batches_as_they_are_and_the_end()
    -> anyhow::Result<()> {
        use futures::stream;

        const FIRST_BATCH: &str = "data: a\n\n";
        const SECOND_BATCH: &str = "data: b\n\ndata: c\n\n";

        let event_stream = stream::iter([
            SessionEvent::Results(Bytes::from_static(FIRST_BATCH.as_bytes())),
            SessionEvent::Results(Bytes::from_static(SECOND_BATCH.as_bytes())),
            SessionEvent::Ended {
                status: QueryJobStatus::Cancelled,
                stats: SessionStats {
                    num_results_emitted: 3,
                    num_duplicates_dropped: 1,
                },
            },
        ]);

        let encoded_events = encode_streaming_query_events(QUERY_JOB_ID, event_stream)
            .try_collect::<Vec<_>>()
            .await?
            .concat();

        assert_eq!(
            String::from_utf8(encoded_events)?,
            [
                &format!("event: job\ndata: {{\"query_job_id\":{QUERY_JOB_ID}}}\n\n"),
                FIRST_BATCH,
                SECOND_BATCH,
                "event: end\ndata: ",
                r#"{"status":"Cancelled","num_results_emitted":3}"#,
                "\n\n",
            ]
            .concat()
        );
        Ok(())
    }

    #[tokio::test]
    async fn streaming_query_events_end_with_an_error_event_without_the_errors_details()
    -> anyhow::Result<()> {
        use futures::stream;
        use query_result_listener::session::SessionEventError;

        const BATCH: &str = "data: a\n\n";

        let event_stream = stream::iter([
            SessionEvent::Results(Bytes::from_static(BATCH.as_bytes())),
            SessionEvent::Failed(SessionEventError::Encoding("an internal detail".to_owned())),
        ]);

        let encoded_events = encode_streaming_query_events(QUERY_JOB_ID, event_stream)
            .try_collect::<Vec<_>>()
            .await?
            .concat();

        assert_eq!(
            String::from_utf8(encoded_events)?,
            [
                &format!("event: job\ndata: {{\"query_job_id\":{QUERY_JOB_ID}}}\n\n"),
                BATCH,
                "event: error\ndata: ",
                r#"{"message":"Internal server error"}"#,
                "\n\n",
            ]
            .concat()
        );
        Ok(())
    }

    #[tokio::test]
    async fn dropping_the_streaming_query_events_drops_the_jobs_events() -> anyhow::Result<()> {
        use futures::channel::mpsc;
        use query_result_listener::test_utils::TIMEOUT;

        let (event_sender, event_stream) = mpsc::unbounded::<SessionEvent<Bytes>>();
        let mut encoded_event_stream =
            Box::pin(encode_streaming_query_events(QUERY_JOB_ID, event_stream));
        tokio::time::timeout(TIMEOUT, encoded_event_stream.next())
            .await?
            .expect("the events should start with the job event")?;
        assert!(!event_sender.is_closed());

        drop(encoded_event_stream);

        assert!(event_sender.is_closed());
        Ok(())
    }
}

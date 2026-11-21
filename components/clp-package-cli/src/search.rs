//! Writing the results of a search, and the exit code of `clp-search` from how the search ends.

use std::io::ErrorKind;
use std::io::Write;
use std::pin::pin;

use clp_rust_utils::api_server::streaming_query::StreamingQueryEndEvent;
use clp_rust_utils::api_server::streaming_query::StreamingQueryErrorEvent;
use clp_rust_utils::api_server::streaming_query::StreamingQueryJobEvent;
use clp_rust_utils::job_config::QueryJobStatus;
use futures::Stream;
use futures::StreamExt;

use crate::api_server::Error;
use crate::api_server::Event;

/// The exit code of `clp-search`. Invalid arguments exit with code 2, as [`clap`] reports them.
#[derive(Debug, Eq, PartialEq)]
#[repr(u8)]
pub enum ExitCode {
    /// The query job succeeded, or the output was closed.
    Succeeded = 0,

    /// The query job failed, was cancelled or was killed.
    QueryJobNotSucceeded = 1,

    /// The API server can't be reached, rejected the query, failed, or broke the stream of the
    /// query's events.
    ApiServerFailed = 3,

    /// A local error, e.g., the package config can't be read, or a result can't be written.
    LocalError = 4,

    /// The search was interrupted by a signal.
    Interrupted = 130,
}

impl From<ExitCode> for std::process::ExitCode {
    fn from(exit_code: ExitCode) -> Self {
        Self::from(exit_code as u8)
    }
}

/// Writes the results of a streaming query to `output`, each as one line holding the result's data
/// as the API server sent it, until the stream of the query's events ends or `interrupt` completes.
///
/// Every failure is logged with its reason, and with the query job's ID once it's known.
///
/// # Type Parameters
///
/// * `EventStreamType` - The stream of the query's events, as
///   [`Client::stream_query`](crate::api_server::Client::stream_query) streams them.
/// * `OutputType` - The writer of the results.
/// * `InterruptType` - The future that completes when the search is interrupted.
///
/// # Returns
///
/// The exit code for how the search ended:
///
/// * [`ExitCode::Succeeded`] if the stream ends with an `end` event whose status is `Succeeded`, or
///   if `output` is closed, which stops the search quietly.
/// * [`ExitCode::QueryJobNotSucceeded`] if the stream ends with an `end` event whose status is
///   `Failed`, `Cancelled` or `Killed`.
/// * [`ExitCode::ApiServerFailed`] if:
///   * The stream yields an error or an `error` event.
///   * The stream ends without an `end` event, or with one whose status isn't terminal.
///   * The stream's first event isn't a `job` event, or a later one is.
/// * [`ExitCode::LocalError`] if a result can't be written to `output` for another reason.
/// * [`ExitCode::Interrupted`] if `interrupt` completes first.
pub async fn run<
    EventStreamType: Stream<Item = Result<Event, Error>> + Send,
    OutputType: Write + Send,
    InterruptType: Future<Output = ()> + Send,
>(
    event_stream: EventStreamType,
    mut output: OutputType,
    interrupt: InterruptType,
) -> ExitCode {
    let mut event_stream = pin!(event_stream);
    let mut interrupt = pin!(interrupt);

    let first_event = tokio::select! {
        () = &mut interrupt => {
            tracing::warn!("The search was interrupted before its query job's ID arrived.");
            return ExitCode::Interrupted;
        }
        event = event_stream.next() => event,
    };
    let query_job_id = match first_event {
        Some(Ok(Event::Job(StreamingQueryJobEvent { query_job_id }))) => query_job_id,
        Some(Ok(Event::Result(_) | Event::End(_) | Event::Error(_))) => {
            tracing::error!("The API server's first event isn't a `job` event.");
            return ExitCode::ApiServerFailed;
        }
        Some(Err(error)) => {
            tracing::error!(error = % error, "The search failed.");
            return ExitCode::ApiServerFailed;
        }
        None => {
            tracing::error!("The API server's response ended before its `job` event.");
            return ExitCode::ApiServerFailed;
        }
    };
    tracing::debug!(query_job_id, "Submitted the query job.");

    loop {
        let event = tokio::select! {
            () = &mut interrupt => {
                tracing::warn!(
                    query_job_id,
                    "The search was interrupted. The API server requests the query job's \
                     cancellation once the connection closes."
                );
                return ExitCode::Interrupted;
            }
            event = event_stream.next() => event,
        };
        match event {
            Some(Ok(Event::Result(data))) => match write_result(&mut output, &data) {
                Ok(()) => {}
                Err(error) if error.kind() == ErrorKind::BrokenPipe => {
                    tracing::debug!(query_job_id, "The output was closed. Stopping the search.");
                    return ExitCode::Succeeded;
                }
                Err(error) => {
                    tracing::error!(query_job_id, error = % error, "Failed to write a result.");
                    return ExitCode::LocalError;
                }
            },
            Some(Ok(Event::End(end_event))) => return end_exit_code(query_job_id, &end_event),
            Some(Ok(Event::Error(StreamingQueryErrorEvent { message }))) => {
                tracing::error!(
                    query_job_id,
                    message = % message,
                    "The API server failed during the search."
                );
                return ExitCode::ApiServerFailed;
            }
            Some(Ok(Event::Job(_))) => {
                tracing::error!(query_job_id, "The API server sent a second `job` event.");
                return ExitCode::ApiServerFailed;
            }
            Some(Err(error)) => {
                tracing::error!(query_job_id, error = % error, "The search failed.");
                return ExitCode::ApiServerFailed;
            }
            None => {
                tracing::error!(
                    query_job_id,
                    "The API server's response ended before its `end` event."
                );
                return ExitCode::ApiServerFailed;
            }
        }
    }
}

/// Writes `data`, a result's data, to `output` as one line, and flushes it.
///
/// # Type Parameters
///
/// * `OutputType` - The writer of the results.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`Write::write_all`]'s return values on failure.
/// * Forwards [`Write::flush`]'s return values on failure.
fn write_result<OutputType: Write>(output: &mut OutputType, data: &[u8]) -> std::io::Result<()> {
    output.write_all(data)?;
    output.write_all(b"\n")?;
    output.flush()
}

/// Logs how the query job whose ID is `query_job_id` ended, as `end_event` reports it.
///
/// # Returns
///
/// The exit code for a stream that ends with `end_event`.
fn end_exit_code(query_job_id: u64, end_event: &StreamingQueryEndEvent) -> ExitCode {
    let &StreamingQueryEndEvent {
        status,
        num_results_emitted,
    } = end_event;
    match status {
        QueryJobStatus::Succeeded => {
            tracing::debug!(
                query_job_id,
                num_results_emitted,
                "The query job succeeded."
            );
            ExitCode::Succeeded
        }
        QueryJobStatus::Failed | QueryJobStatus::Cancelled | QueryJobStatus::Killed => {
            tracing::error!(
                query_job_id,
                status = ? status,
                "The query job didn't succeed."
            );
            ExitCode::QueryJobNotSucceeded
        }
        // The API server ends the stream only once the job's status is terminal.
        QueryJobStatus::Pending
        | QueryJobStatus::Running
        | QueryJobStatus::CancelRequested
        | QueryJobStatus::Cancelling => {
            tracing::error!(
                query_job_id,
                status = ? status,
                "The API server's `end` event has a non-terminal status."
            );
            ExitCode::ApiServerFailed
        }
    }
}

#[cfg(test)]
mod tests {
    use std::future::pending;
    use std::future::ready;
    use std::time::Duration;

    use anyhow::Result;
    use bytes::Bytes;
    use futures::stream;

    use super::*;

    const QUERY_JOB_ID: u64 = 1;

    /// The longest time a test waits for a search to end.
    const TEST_TIMEOUT: Duration = Duration::from_secs(10);

    /// A writer whose every write fails with an error of kind `error_kind`.
    struct FailingWriter {
        error_kind: ErrorKind,
    }

    impl Write for FailingWriter {
        fn write(&mut self, _buf: &[u8]) -> std::io::Result<usize> {
            Err(self.error_kind.into())
        }

        fn flush(&mut self) -> std::io::Result<()> {
            Err(self.error_kind.into())
        }
    }

    /// # Returns
    ///
    /// The `job` event of the query job whose ID is [`QUERY_JOB_ID`].
    const fn job_event() -> Event {
        Event::Job(StreamingQueryJobEvent {
            query_job_id: QUERY_JOB_ID,
        })
    }

    /// # Returns
    ///
    /// An `end` event whose status is `status`.
    const fn end_event(status: QueryJobStatus) -> Event {
        Event::End(StreamingQueryEndEvent {
            status,
            num_results_emitted: 0,
        })
    }

    /// Runs a search that is never interrupted, whose stream yields `events` and then ends.
    ///
    /// # Returns
    ///
    /// A tuple containing, on success:
    ///
    /// * The search's exit code.
    /// * The search's output.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`tokio::time::timeout`]'s return values on failure.
    async fn run_to_end(events: Vec<Result<Event, Error>>) -> Result<(ExitCode, Vec<u8>)> {
        let mut output = Vec::new();
        let exit_code = tokio::time::timeout(
            TEST_TIMEOUT,
            run(stream::iter(events), &mut output, pending()),
        )
        .await?;
        Ok((exit_code, output))
    }

    #[tokio::test]
    async fn run_writes_each_result_as_it_is_on_its_own_line() -> Result<()> {
        const RESULTS: [&str; 2] = [
            r#"{"archive_id":"a","timestamp_millisecs":1,"message":"first \"quoted\""}"#,
            r#"{"archive_id":"b","timestamp_millisecs":-1,"message":"second\nline"}"#,
        ];

        let (exit_code, output) = run_to_end(vec![
            Ok(job_event()),
            Ok(Event::Result(Bytes::from_static(RESULTS[0].as_bytes()))),
            Ok(Event::Result(Bytes::from_static(RESULTS[1].as_bytes()))),
            Ok(end_event(QueryJobStatus::Succeeded)),
        ])
        .await?;

        assert_eq!(exit_code, ExitCode::Succeeded);
        assert_eq!(
            output,
            format!("{}\n{}\n", RESULTS[0], RESULTS[1]).as_bytes()
        );
        Ok(())
    }

    #[tokio::test]
    async fn run_maps_each_end_status_to_its_exit_code() -> Result<()> {
        for (status, expected_exit_code) in [
            (QueryJobStatus::Succeeded, ExitCode::Succeeded),
            (QueryJobStatus::Failed, ExitCode::QueryJobNotSucceeded),
            (QueryJobStatus::Cancelled, ExitCode::QueryJobNotSucceeded),
            (QueryJobStatus::Killed, ExitCode::QueryJobNotSucceeded),
            (QueryJobStatus::Pending, ExitCode::ApiServerFailed),
            (QueryJobStatus::Running, ExitCode::ApiServerFailed),
            (QueryJobStatus::CancelRequested, ExitCode::ApiServerFailed),
            (QueryJobStatus::Cancelling, ExitCode::ApiServerFailed),
        ] {
            let (exit_code, _) = run_to_end(vec![Ok(job_event()), Ok(end_event(status))]).await?;

            assert_eq!(exit_code, expected_exit_code, "status: {status:?}");
        }
        Ok(())
    }

    #[tokio::test]
    async fn run_fails_if_the_api_server_fails_or_breaks_the_stream() -> Result<()> {
        for (case, events) in [
            (
                "an `error` event",
                vec![
                    Ok(job_event()),
                    Ok(Event::Error(StreamingQueryErrorEvent {
                        message: "Internal server error".to_owned(),
                    })),
                ],
            ),
            (
                "an error",
                vec![
                    Ok(job_event()),
                    Err(Error::UnknownEvent {
                        name: "unknown".to_owned(),
                    }),
                ],
            ),
            (
                "an error before the `job` event",
                vec![Err(Error::UnknownEvent {
                    name: "unknown".to_owned(),
                })],
            ),
            ("no `job` event", vec![]),
            ("no `end` event", vec![Ok(job_event())]),
            (
                "a result before the `job` event",
                vec![
                    Ok(Event::Result(Bytes::from_static(b"{}"))),
                    Ok(job_event()),
                    Ok(end_event(QueryJobStatus::Succeeded)),
                ],
            ),
            (
                "a second `job` event",
                vec![
                    Ok(job_event()),
                    Ok(job_event()),
                    Ok(end_event(QueryJobStatus::Succeeded)),
                ],
            ),
        ] {
            let (exit_code, _) = run_to_end(events).await?;

            assert_eq!(exit_code, ExitCode::ApiServerFailed, "case: {case}");
        }
        Ok(())
    }

    #[tokio::test]
    async fn run_stops_quietly_once_the_output_is_closed() -> Result<()> {
        let search = run(
            stream::iter([
                Ok(job_event()),
                Ok(Event::Result(Bytes::from_static(b"{}"))),
                Ok(end_event(QueryJobStatus::Failed)),
            ]),
            FailingWriter {
                error_kind: ErrorKind::BrokenPipe,
            },
            pending(),
        );

        assert_eq!(
            tokio::time::timeout(TEST_TIMEOUT, search).await?,
            ExitCode::Succeeded
        );
        Ok(())
    }

    #[tokio::test]
    async fn run_fails_locally_if_a_result_cant_be_written() -> Result<()> {
        let search = run(
            stream::iter([
                Ok(job_event()),
                Ok(Event::Result(Bytes::from_static(b"{}"))),
                Ok(end_event(QueryJobStatus::Succeeded)),
            ]),
            FailingWriter {
                error_kind: ErrorKind::StorageFull,
            },
            pending(),
        );

        assert_eq!(
            tokio::time::timeout(TEST_TIMEOUT, search).await?,
            ExitCode::LocalError
        );
        Ok(())
    }

    #[tokio::test]
    async fn run_stops_once_interrupted() -> Result<()> {
        for events in [vec![], vec![Ok(job_event())]] {
            let mut output = Vec::new();
            let search = run(
                stream::iter(events).chain(stream::pending()),
                &mut output,
                ready(()),
            );

            assert_eq!(
                tokio::time::timeout(TEST_TIMEOUT, search).await?,
                ExitCode::Interrupted
            );
            assert_eq!(output, b"");
        }
        Ok(())
    }
}

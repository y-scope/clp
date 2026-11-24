//! `clp-search`: searches the compressed logs through the API server's `POST /query/stream` route,
//! and writes each result to stdout as a JSON object on its own line.

use std::num::NonZeroUsize;
use std::path::Path;
use std::path::PathBuf;
use std::process::ExitCode;
use std::time::Duration;

use anyhow::Context;
use bytes::BytesMut;
use clap::Parser;
use clap::builder::NonEmptyStringValueParser;
use clap::builder::TypedValueParser;
use clp_package_cli::INTERRUPTED_EXIT_CODE;
use clp_package_cli::TerminationSignals;
use clp_package_cli::resolve_config_path;
use clp_package_cli::run_on_runtime;
use clp_rust_utils::api_server::SSE_KEEP_ALIVE_INTERVAL;
use clp_rust_utils::api_server::streaming_query::StreamingQueryConfig;
use clp_rust_utils::api_server::streaming_query::StreamingQueryEndEvent;
use clp_rust_utils::api_server::streaming_query::StreamingQueryErrorEvent;
use clp_rust_utils::api_server::streaming_query::StreamingQueryJobEvent;
use clp_rust_utils::clp_config::package::config::ApiServer;
use clp_rust_utils::dataset::DatasetName;
use clp_rust_utils::job_config::QueryJobStatus;
use non_empty_string::NonEmptyString;
use reqwest::Url;
use serde::Deserialize;

/// The longest time that connecting to the API server may take.
const CONNECT_TIMEOUT: Duration = Duration::from_secs(5);

/// The longest time that a read from the API server may take: four times the interval of the API
/// server's keep-alive comments, so that an idle search isn't failed.
const READ_TIMEOUT: Duration = SSE_KEEP_ALIVE_INTERVAL.saturating_mul(4);

/// The most batches of results that can wait to be written to stdout.
const MAX_NUM_PENDING_BATCHES: NonZeroUsize = NonZeroUsize::new(16).expect("16 is nonzero");

/// Command-line arguments for `clp-search`.
#[derive(Parser)]
#[command(
    name = "clp-search",
    version,
    about = "Searches the compressed logs, and writes each result to stdout as a JSON object on \
             its own line."
)]
struct Args {
    /// The KQL query.
    #[arg(value_parser = NonEmptyStringValueParser::new().try_map(NonEmptyString::new))]
    query: NonEmptyString,

    /// A dataset to search. Can be specified multiple times.
    #[arg(
        long = "dataset",
        value_name = "NAME",
        default_value = DatasetName::default_name()
    )]
    datasets: Vec<DatasetName>,

    /// Only search log events at or after this time, in milliseconds since the Unix epoch.
    #[arg(long, value_name = "MILLISECS", allow_negative_numbers = true)]
    begin_time: Option<i64>,

    /// Only search log events at or before this time, in milliseconds since the Unix epoch.
    #[arg(long, value_name = "MILLISECS", allow_negative_numbers = true)]
    end_time: Option<i64>,

    /// Ignore case distinctions between values in the query and the compressed data.
    #[arg(long)]
    ignore_case: bool,

    /// The CLP package's config. Defaults to `$CLP_HOME/etc/clp-config.yaml`.
    #[arg(long, short, value_name = "PATH")]
    config: Option<PathBuf>,

    /// Enable debug logging.
    #[arg(long, short)]
    verbose: bool,
}

/// The part of the package config that configures the API server.
///
/// # NOTE
///
/// * This type is partially defined: unused fields are omitted and discarded through
///   deserialization.
#[derive(Deserialize)]
#[serde(default)]
struct PackageConfig {
    /// The API server's address, or `None` if a null or empty `api_server` disables it.
    api_server: Option<ApiServerAddress>,
}

impl Default for PackageConfig {
    fn default() -> Self {
        Self {
            api_server: Some(ApiServerAddress::default()),
        }
    }
}

/// The address part of [`ApiServer`].
///
/// # NOTE
///
/// * This type is partially defined: unused fields are omitted and discarded through
///   deserialization.
/// * The default values are taken from [`ApiServer::default`].
#[derive(Deserialize)]
#[serde(default)]
struct ApiServerAddress {
    host: String,
    port: u16,
}

impl Default for ApiServerAddress {
    fn default() -> Self {
        let ApiServer { host, port, .. } = ApiServer::default();
        Self { host, port }
    }
}

/// A failure of a search, which determines the exit code of `clp-search`.
enum Failure {
    /// The query job ended with a status other than [`QueryJobStatus::Succeeded`].
    QueryJob(anyhow::Error),

    /// The API server can't be reached, rejected the query, failed, or broke the stream of the
    /// query's events.
    ApiServer(anyhow::Error),

    /// A local error, e.g., the package config can't be read, or a result can't be written.
    Local(anyhow::Error),
}

impl Failure {
    /// # Returns
    ///
    /// A [`Failure::Local`] caused by `error`, without a query job ID, for a search that fails
    /// before its query job is submitted.
    const fn local_failure(error: anyhow::Error) -> (Self, Option<u64>) {
        (Self::Local(error), None)
    }

    /// Logs the failure, with the ID of the search's query job, `query_job_id`, once it's known.
    ///
    /// # Returns
    ///
    /// The exit code of a search that fails this way: 1 for [`Failure::QueryJob`], 3 for
    /// [`Failure::ApiServer`], and 4 for [`Failure::Local`].
    fn report(self, query_job_id: Option<u64>) -> ExitCode {
        let (exit_code, error) = match self {
            Self::QueryJob(error) => (1, error),
            Self::ApiServer(error) => (3, error),
            Self::Local(error) => (4, error),
        };
        tracing::error!(
            query_job_id,
            error = % format_args!("{error:#}"),
            "The search failed."
        );
        ExitCode::from(exit_code)
    }
}

/// How a search that didn't fail ended.
enum Outcome {
    /// The query job succeeded, or stdout was closed.
    Succeeded,

    /// SIGINT or SIGTERM interrupted the search, with the query job's ID once it's known.
    Interrupted { query_job_id: Option<u64> },
}

/// A query whose job the API server has submitted, with the rest of its events.
struct SubmittedQuery {
    /// The ID of the query's job.
    query_job_id: u64,

    /// The events after the `job` event.
    events: EventReader,
}

/// An event of a streaming query's response.
enum Event {
    /// The `job` event, which the API server sends first.
    Job(StreamingQueryJobEvent),

    /// A batch of results, each followed by a newline.
    Results(Vec<u8>),

    /// The `end` event, which ends the response once the query job ends.
    End(StreamingQueryEndEvent),

    /// The `error` event, which ends the response if the API server fails.
    Error(StreamingQueryErrorEvent),
}

/// A reader of the events of a streaming query's response, a successful response of the streaming
/// query route.
///
/// The response body is read a chunk at a time. An `event: <name>` line names the event of the next
/// `data: <data>` line, and a `data:` line without a name is a result. Comments and blank lines are
/// skipped, and any other line is an error. The results that a chunk completes are read as one
/// batch, so that they show up as they arrive without a write per result.
struct EventReader {
    response: reqwest::Response,

    /// The received bytes from the first one that isn't handled yet.
    unhandled: BytesMut,

    /// The number of bytes at the start of `unhandled` that are known to hold no newline.
    num_bytes_scanned: usize,

    /// The event, or the failure, of a handled line that came after results, to read once the
    /// results are read.
    pending: Option<Result<Event, Failure>>,

    /// The state of handling the lines.
    parser: LineParser,
}

impl EventReader {
    /// Factory function.
    ///
    /// # Returns
    ///
    /// A newly created reader of `response`'s events.
    fn new(response: reqwest::Response) -> Self {
        Self {
            response,
            unhandled: BytesMut::new(),
            num_bytes_scanned: 0,
            pending: None,
            parser: LineParser::default(),
        }
    }

    /// Reads the next event.
    ///
    /// # Returns
    ///
    /// The next event, or `None` if the response body ends, on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`LineParser::handle_line`]'s return values on failure.
    /// * Forwards [`reqwest::Response::chunk`]'s return values as [`Failure::ApiServer`] on
    ///   failure.
    async fn next_event(&mut self) -> Result<Option<Event>, Failure> {
        use bytes::Buf;

        if let Some(event) = self.pending.take() {
            return event.map(Some);
        }
        loop {
            let mut line_start = 0;
            let mut search_start = self.num_bytes_scanned;
            let mut event = None;
            while let Some(newline_pos) = self.unhandled[search_start..]
                .iter()
                .position(|&byte| byte == b'\n')
            {
                let line_end = search_start + newline_pos;
                event = self
                    .parser
                    .handle_line(&self.unhandled[line_start..line_end])
                    .transpose();
                line_start = line_end + 1;
                search_start = line_start;
                if event.is_some() {
                    break;
                }
            }
            self.unhandled.advance(line_start);
            if let Some(event) = event {
                self.num_bytes_scanned = 0;
                let Some(results) = self.parser.take_results() else {
                    return event.map(Some);
                };
                self.pending = Some(event);
                return Ok(Some(Event::Results(results)));
            }
            self.num_bytes_scanned = self.unhandled.len();
            // The received chunk is handled, so its results are read before the next chunk.
            if let Some(results) = self.parser.take_results() {
                return Ok(Some(Event::Results(results)));
            }
            let Some(chunk) = self
                .response
                .chunk()
                .await
                .context("failed to read the API server's response")
                .map_err(Failure::ApiServer)?
            else {
                return Ok(None);
            };
            self.unhandled.extend_from_slice(&chunk);
        }
    }
}

/// The state of handling the lines of a streaming query's response.
#[derive(Default)]
struct LineParser {
    /// The name of the event of the next `data:` line, from the `event:` line before it.
    event_name: Option<Vec<u8>>,

    /// The results of the handled lines that aren't taken yet.
    results: Vec<u8>,
}

impl LineParser {
    /// # Returns
    ///
    /// The results of the handled lines that aren't taken yet, or `None` if there are none.
    fn take_results(&mut self) -> Option<Vec<u8>> {
        if self.results.is_empty() {
            return None;
        }
        Some(std::mem::take(&mut self.results))
    }

    /// Handles `line`, a line of the response without its newline. A result's data is added to the
    /// results as it is, followed by a newline.
    ///
    /// # Returns
    ///
    /// The event whose data the line is, or `None` if the line isn't the data of a named event, on
    /// success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`Failure::ApiServer`] if:
    ///   * The line isn't an `event:` or `data:` line, a comment, or a blank line.
    ///   * The line is the data of an event with an unknown name.
    ///   * The data of a `job`, `end` or `error` event doesn't deserialize.
    fn handle_line(&mut self, line: &[u8]) -> Result<Option<Event>, Failure> {
        let line = line.strip_suffix(b"\r").unwrap_or(line);
        if line.is_empty() || line.starts_with(b":") {
            return Ok(None);
        }
        if let Some(name) = line.strip_prefix(b"event: ") {
            self.event_name = Some(name.to_vec());
            return Ok(None);
        }
        let Some(data) = line.strip_prefix(b"data: ") else {
            return Err(Failure::ApiServer(anyhow::anyhow!(
                "the API server sent an unexpected line {:?}",
                String::from_utf8_lossy(line)
            )));
        };
        let event = match self.event_name.take().as_deref() {
            None => {
                self.results.extend_from_slice(data);
                self.results.push(b'\n');
                return Ok(None);
            }
            Some(b"job") => Event::Job(
                serde_json::from_slice(data)
                    .context("invalid `job` event")
                    .map_err(Failure::ApiServer)?,
            ),
            Some(b"end") => Event::End(
                serde_json::from_slice(data)
                    .context("invalid `end` event")
                    .map_err(Failure::ApiServer)?,
            ),
            Some(b"error") => Event::Error(
                serde_json::from_slice(data)
                    .context("invalid `error` event")
                    .map_err(Failure::ApiServer)?,
            ),
            Some(name) => {
                return Err(Failure::ApiServer(anyhow::anyhow!(
                    "the API server sent an unknown event `{}`",
                    String::from_utf8_lossy(name)
                )));
            }
        };
        Ok(Some(event))
    }
}

/// # Returns
///
/// The exit code of the search: 0 if it succeeds or stdout is closed, [`Failure::report`]'s if it
/// fails, and [`INTERRUPTED_EXIT_CODE`] if a signal interrupts it. Invalid arguments exit with code
/// 2.
///
/// # Panics
///
/// Panics if `--dataset` gives no dataset names, which can't happen since it has a default value.
fn main() -> ExitCode {
    use clap::CommandFactory;
    use clap::error::ErrorKind;
    use clp_package_cli::set_up_logging;
    use clp_rust_utils::dataset::NonEmptyDatasetNames;
    use clp_rust_utils::job_config::SearchQuery;
    use clp_rust_utils::job_config::TimeRange;

    let args = Args::parse();
    let time_range = TimeRange::new(args.begin_time, args.end_time).unwrap_or_else(|error| {
        Args::command()
            .error(ErrorKind::ArgumentConflict, error)
            .exit()
    });
    let streaming_query_config = StreamingQueryConfig {
        query: SearchQuery {
            datasets: NonEmptyDatasetNames::try_from(args.datasets)
                .expect("`--dataset` should default to the default dataset"),
            query_string: args.query,
            time_range,
            ignore_case: args.ignore_case,
        },
    };

    set_up_logging(env!("CARGO_CRATE_NAME"), args.verbose);
    match run(args.config, &streaming_query_config) {
        Ok(Outcome::Succeeded) => ExitCode::SUCCESS,
        Ok(Outcome::Interrupted { query_job_id }) => {
            tracing::warn!(
                query_job_id,
                "The search was interrupted. The API server requests the query job's cancellation."
            );
            ExitCode::from(INTERRUPTED_EXIT_CODE)
        }
        Err((failure, query_job_id)) => failure.report(query_job_id),
    }
}

/// Runs `streaming_query_config`'s search through the API server that the package config at
/// `host_config_path`, or the default package config, configures, until the search ends or SIGINT
/// or SIGTERM interrupts it. A failure comes with the query job's ID once it's known.
///
/// # Returns
///
/// How the search ended on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`resolve_config_path`]'s return values as [`Failure::Local`] on failure.
/// * Forwards [`read_stream_query_url`]'s return values as [`Failure::Local`] on failure.
/// * Forwards [`reqwest::ClientBuilder::build`]'s return values as [`Failure::Local`] on failure.
/// * Forwards [`run_on_runtime`]'s return values as [`Failure::Local`] on failure.
/// * Forwards [`search`]'s return values on failure.
fn run(
    host_config_path: Option<PathBuf>,
    streaming_query_config: &StreamingQueryConfig,
) -> Result<Outcome, (Failure, Option<u64>)> {
    let config_path = resolve_config_path(host_config_path).map_err(Failure::local_failure)?;
    let stream_query_url = read_stream_query_url(&config_path).map_err(Failure::local_failure)?;
    let http_client = reqwest::Client::builder()
        .no_proxy()
        .connect_timeout(CONNECT_TIMEOUT)
        .read_timeout(READ_TIMEOUT)
        .build()
        .context("failed to build the HTTP client")
        .map_err(Failure::local_failure)?;
    let outcome = run_on_runtime(search(
        &http_client,
        stream_query_url,
        streaming_query_config,
    ))
    .map_err(Failure::local_failure)??;
    Ok(outcome)
}

/// Reads the URL of the API server's streaming query route from the package config at
/// `config_path`.
///
/// A config without `api_server`, or an empty config, configures the API server with its default
/// host and port.
///
/// # Returns
///
/// The URL of the streaming query route on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`anyhow::Error`] if the config disables the API server.
/// * Forwards [`clp_rust_utils::serde::yaml::from_path`]'s return values on failure.
/// * Forwards [`Url::parse`]'s return values on failure.
fn read_stream_query_url(config_path: &Path) -> anyhow::Result<Url> {
    let config: Option<PackageConfig> = clp_rust_utils::serde::yaml::from_path(config_path)
        .with_context(|| format!("failed to read {}", config_path.display()))?;
    let Some(ApiServerAddress { host, port }) = config.unwrap_or_default().api_server else {
        anyhow::bail!(
            "the API server is disabled (`api_server` is null) in {}",
            config_path.display()
        );
    };
    let url = format!("http://{host}:{port}/query/stream");
    Url::parse(&url).with_context(|| format!("invalid API server URL {url}"))
}

/// Submits `streaming_query_config`'s query through `http_client` to the streaming query route at
/// `stream_query_url`, and writes the results to stdout, until the query job ends or SIGINT or
/// SIGTERM interrupts the search. A failure comes with the query job's ID once it's known.
///
/// # Returns
///
/// How the search ended on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`TerminationSignals::listen`]'s return values as [`Failure::Local`] on failure.
/// * Forwards [`submit`]'s return values on failure.
/// * Forwards [`stream_results`]'s return values on failure.
async fn search(
    http_client: &reqwest::Client,
    stream_query_url: Url,
    streaming_query_config: &StreamingQueryConfig,
) -> Result<Outcome, (Failure, Option<u64>)> {
    let mut signals = TerminationSignals::listen().map_err(Failure::local_failure)?;
    let Some(submitted) = signals
        .until_signal(submit(
            http_client,
            stream_query_url,
            streaming_query_config,
        ))
        .await
    else {
        return Ok(Outcome::Interrupted { query_job_id: None });
    };
    let SubmittedQuery {
        query_job_id,
        events,
    } = submitted.map_err(|failure| (failure, None))?;
    tracing::debug!(query_job_id, "Submitted the query job.");
    signals
        .until_signal(stream_results(events))
        .await
        .map_or_else(
            || {
                Ok(Outcome::Interrupted {
                    query_job_id: Some(query_job_id),
                })
            },
            |result| {
                result
                    .map(|()| Outcome::Succeeded)
                    .map_err(|failure| (failure, Some(query_job_id)))
            },
        )
}

/// Submits `streaming_query_config`'s query through `http_client` to the streaming query route at
/// `stream_query_url`, and reads the response's first event, which must be the `job` event.
///
/// # Returns
///
/// The submitted query on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`Failure::ApiServer`] if:
///   * The API server responds with an unsuccessful status.
///   * The response's first event is an `error` event, or another event than `job`.
///   * The response body ends before any event.
/// * Forwards [`reqwest::RequestBuilder::send`]'s return values as [`Failure::ApiServer`] on
///   failure.
/// * Forwards [`reqwest::Response::text`]'s return values as [`Failure::ApiServer`] on failure.
/// * Forwards [`EventReader::next_event`]'s return values on failure.
///
/// # Cancel safety
///
/// Dropping the future closes the connection, after which the API server requests the cancellation
/// of the query job, if it has submitted one.
async fn submit(
    http_client: &reqwest::Client,
    stream_query_url: Url,
    streaming_query_config: &StreamingQueryConfig,
) -> Result<SubmittedQuery, Failure> {
    let response = http_client
        .post(stream_query_url)
        .header(reqwest::header::ACCEPT, "text/event-stream")
        .json(streaming_query_config)
        .send()
        .await
        .context("failed to send the query to the API server")
        .map_err(Failure::ApiServer)?;
    let status = response.status();
    if !status.is_success() {
        let body = response
            .text()
            .await
            .context("failed to read the API server's response")
            .map_err(Failure::ApiServer)?;
        return Err(Failure::ApiServer(anyhow::anyhow!(
            "the API server responded with status {status}: {body}"
        )));
    }
    let mut events = EventReader::new(response);
    match events.next_event().await? {
        Some(Event::Job(StreamingQueryJobEvent { query_job_id })) => Ok(SubmittedQuery {
            query_job_id,
            events,
        }),
        Some(Event::Error(StreamingQueryErrorEvent { message })) => Err(Failure::ApiServer(
            anyhow::anyhow!("the API server failed during the search: {message}"),
        )),
        Some(_) => Err(Failure::ApiServer(anyhow::anyhow!(
            "the API server's first event isn't the `job` event"
        ))),
        None => Err(Failure::ApiServer(anyhow::anyhow!(
            "the API server's response ended before any event"
        ))),
    }
}

/// Writes the data of each result that `events` reads to stdout as it is, on its own line, until
/// the query job ends.
///
/// The results are written by [`write_batches`] on a blocking thread, so that a blocked write
/// (e.g., by an idle pager) doesn't keep the runtime from handling signals. Once stdout is closed,
/// reading stops and the function succeeds.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`Failure::Local`] if the writer of the results panics.
/// * Forwards [`read_results`]'s return values on failure.
/// * Forwards [`write_batches`]'s return values as [`Failure::Local`] on failure.
///
/// # Cancel safety
///
/// Dropping the future closes the connection, after which the API server requests the query job's
/// cancellation. It also detaches the writer of the results, which keeps writing the batches that
/// wait, or stays blocked on a stdout that isn't read, until the runtime is shut down without
/// waiting for it (see [`run_on_runtime`]).
async fn stream_results(events: EventReader) -> Result<(), Failure> {
    let (batch_sender, batch_receiver) = tokio::sync::mpsc::channel(MAX_NUM_PENDING_BATCHES.get());
    let writer = tokio::task::spawn_blocking(move || write_batches(batch_receiver));
    let read_result = read_results(events, batch_sender).await;
    match writer
        .await
        .context("the writer of the results panicked")
        .map_err(Failure::Local)?
    {
        Err(error) if error.kind() == std::io::ErrorKind::BrokenPipe => Ok(()),
        write_result => {
            write_result
                .context("failed to write the results")
                .map_err(Failure::Local)?;
            read_result
        }
    }
}

/// Writes each batch of results that `batch_receiver` receives to stdout, until all its senders are
/// dropped. Batches that wait are written together: stdout is only flushed once no batch waits.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`std::io::Write::write_all`]'s return values on failure.
/// * Forwards [`std::io::Write::flush`]'s return values on failure.
fn write_batches(mut batch_receiver: tokio::sync::mpsc::Receiver<Vec<u8>>) -> std::io::Result<()> {
    use std::io::Write;

    let mut stdout = std::io::BufWriter::new(std::io::stdout().lock());
    while let Some(batch) = batch_receiver.blocking_recv() {
        stdout.write_all(&batch)?;
        if batch_receiver.is_empty() {
            stdout.flush()?;
        }
    }
    stdout.flush()
}

/// Reads the events after the `job` event from `events`, and sends each batch of results through
/// `batch_sender`, until the query job ends or the batches' receiver is dropped. Dropping
/// `batch_sender` on return closes the channel, which tells the writer that no batch follows.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`Failure::QueryJob`] if the `end` event's status isn't [`QueryJobStatus::Succeeded`].
/// * [`Failure::ApiServer`] if:
///   * The API server sends an `error` event, or another `job` event.
///   * The response body ends before an `end` or `error` event.
/// * Forwards [`EventReader::next_event`]'s return values on failure.
async fn read_results(
    mut events: EventReader,
    batch_sender: tokio::sync::mpsc::Sender<Vec<u8>>,
) -> Result<(), Failure> {
    loop {
        let Some(event) = events.next_event().await? else {
            return Err(Failure::ApiServer(anyhow::anyhow!(
                "the API server's response ended before the query job did"
            )));
        };
        match event {
            Event::Results(results) => {
                if batch_sender.send(results).await.is_err() {
                    // The writer of the results stopped, e.g., because stdout is closed.
                    return Ok(());
                }
            }
            Event::End(StreamingQueryEndEvent {
                status,
                num_results_emitted,
            }) => {
                tracing::debug!(status = ? status, num_results_emitted, "The query job ended.");
                if status == QueryJobStatus::Succeeded {
                    return Ok(());
                }
                return Err(Failure::QueryJob(anyhow::anyhow!(
                    "the query job ended with status `{status:?}`"
                )));
            }
            Event::Error(StreamingQueryErrorEvent { message }) => {
                return Err(Failure::ApiServer(anyhow::anyhow!(
                    "the API server failed during the search: {message}"
                )));
            }
            Event::Job(_) => {
                return Err(Failure::ApiServer(anyhow::anyhow!(
                    "the API server sent another `job` event"
                )));
            }
        }
    }
}

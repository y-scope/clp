//! `clp-search`: searches the compressed logs through the API server's `POST /query/stream` route,
//! and writes each result to stdout as a JSON object on its own line.

use std::ops::ControlFlow;
use std::path::Path;
use std::path::PathBuf;
use std::process::ExitCode;
use std::time::Duration;

use anyhow::Context;
use clap::Parser;
use clap::builder::NonEmptyStringValueParser;
use clap::builder::TypedValueParser;
use clp_package_cli::INTERRUPTED_EXIT_CODE;
use clp_package_cli::resolve_config_path;
use clp_package_cli::run_until_signal;
use clp_package_cli::set_up_logging;
use clp_rust_utils::api_server::SSE_KEEP_ALIVE_INTERVAL;
use clp_rust_utils::api_server::streaming_query::StreamingQueryConfig;
use clp_rust_utils::api_server::streaming_query::StreamingQueryEndEvent;
use clp_rust_utils::api_server::streaming_query::StreamingQueryErrorEvent;
use clp_rust_utils::api_server::streaming_query::StreamingQueryJobEvent;
use clp_rust_utils::clp_config::package::config::ApiServer;
use clp_rust_utils::dataset::DatasetName;
use clp_rust_utils::dataset::NonEmptyDatasetNames;
use clp_rust_utils::job_config::QueryJobStatus;
use clp_rust_utils::job_config::SearchQuery;
use clp_rust_utils::job_config::TimeRange;
use non_empty_string::NonEmptyString;
use reqwest::Url;
use serde::Deserialize;

/// The longest time that connecting to the API server may take.
const CONNECT_TIMEOUT: Duration = Duration::from_secs(5);

/// The longest time that a read from the API server may take: four times the interval of the API
/// server's keep-alive comments, so that an idle search isn't failed.
const READ_TIMEOUT: Duration = SSE_KEEP_ALIVE_INTERVAL.saturating_mul(4);

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
#[derive(Default, Deserialize)]
#[serde(default)]
struct PackageConfig {
    api_server: ApiServerAddress,
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
    /// Logs the failure.
    ///
    /// # Returns
    ///
    /// The exit code of a search that fails this way: 1 for [`Failure::QueryJob`], 3 for
    /// [`Failure::ApiServer`], and 4 for [`Failure::Local`].
    fn report(self) -> ExitCode {
        let (exit_code, error) = match self {
            Self::QueryJob(error) => (1, error),
            Self::ApiServer(error) => (3, error),
            Self::Local(error) => (4, error),
        };
        tracing::error!(error = % format_args!("{error:#}"), "The search failed.");
        ExitCode::from(exit_code)
    }

    /// # Returns
    ///
    /// The failure, with `context` added to its error.
    fn context(self, context: String) -> Self {
        match self {
            Self::QueryJob(error) => Self::QueryJob(error.context(context)),
            Self::ApiServer(error) => Self::ApiServer(error.context(context)),
            Self::Local(error) => Self::Local(error.context(context)),
        }
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
    run(args.config.as_deref(), &streaming_query_config).unwrap_or_else(Failure::report)
}

/// Runs `streaming_query_config`'s search through the API server that the package config at
/// `host_config_path`, or the default package config, configures, until the search ends or SIGINT
/// or SIGTERM interrupts it.
///
/// # Returns
///
/// The exit code of the search on success: [`ExitCode::SUCCESS`], or [`INTERRUPTED_EXIT_CODE`] if a
/// signal interrupts it.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`resolve_config_path`]'s return values as [`Failure::Local`] on failure.
/// * Forwards [`read_stream_query_url`]'s return values as [`Failure::Local`] on failure.
/// * Forwards [`reqwest::ClientBuilder::build`]'s return values as [`Failure::Local`] on failure.
/// * Forwards [`run_until_signal`]'s return values as [`Failure::Local`] on failure.
/// * Forwards [`search`]'s return values on failure.
fn run(
    host_config_path: Option<&Path>,
    streaming_query_config: &StreamingQueryConfig,
) -> Result<ExitCode, Failure> {
    let config_path = resolve_config_path(host_config_path).map_err(Failure::Local)?;
    let stream_query_url = read_stream_query_url(&config_path).map_err(Failure::Local)?;
    let http_client = reqwest::Client::builder()
        .no_proxy()
        .connect_timeout(CONNECT_TIMEOUT)
        .read_timeout(READ_TIMEOUT)
        .build()
        .context("failed to build the HTTP client")
        .map_err(Failure::Local)?;
    let Some(result) = run_until_signal(search(
        &http_client,
        stream_query_url,
        streaming_query_config,
    ))
    .map_err(Failure::Local)?
    else {
        tracing::warn!(
            "The search was interrupted. The API server requests the query job's cancellation."
        );
        return Ok(ExitCode::from(INTERRUPTED_EXIT_CODE));
    };
    result.map(|()| ExitCode::SUCCESS)
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
/// * Forwards [`clp_rust_utils::serde::yaml::from_path`]'s return values on failure.
/// * Forwards [`Url::parse`]'s return values on failure.
fn read_stream_query_url(config_path: &Path) -> anyhow::Result<Url> {
    let config: Option<PackageConfig> = clp_rust_utils::serde::yaml::from_path(config_path)
        .with_context(|| format!("failed to read {}", config_path.display()))?;
    let ApiServerAddress { host, port } = config.unwrap_or_default().api_server;
    let url = format!("http://{host}:{port}/query/stream");
    Url::parse(&url).with_context(|| format!("invalid API server URL {url}"))
}

/// Submits `streaming_query_config`'s query through `http_client` to the streaming query route at
/// `stream_query_url`, and writes the data of each result to stdout as it is, on its own line,
/// until the query job ends.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`Failure::ApiServer`] if the API server responds with an unsuccessful status.
/// * Forwards [`reqwest::RequestBuilder::send`]'s return values as [`Failure::ApiServer`] on
///   failure.
/// * Forwards [`reqwest::Response::text`]'s return values as [`Failure::ApiServer`] on failure.
/// * Forwards [`write_results`]'s return values on failure, with the query job's ID once it's
///   known.
///
/// # Cancel safety
///
/// Dropping the future closes the connection, after which the API server requests the query job's
/// cancellation.
async fn search(
    http_client: &reqwest::Client,
    stream_query_url: Url,
    streaming_query_config: &StreamingQueryConfig,
) -> Result<(), Failure> {
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
    let mut query_job_id = None;
    write_results(response, &mut query_job_id)
        .await
        .map_err(|failure| match query_job_id {
            Some(id) => failure.context(format!("query job {id}")),
            None => failure,
        })
}

/// Reads the events of a streaming query from `response`, a successful response of the streaming
/// query route, and writes the data of each result to stdout as it is, on its own line, until the
/// query job ends. The `job` event's query job ID is stored in `query_job_id`.
///
/// The response body is read a chunk at a time, and the results that a chunk completes are written
/// to stdout together, so that they show up as they arrive without a write per result. Once stdout
/// is closed, writing stops and the function succeeds.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`Failure::ApiServer`] if the response body ends before an `end` or `error` event.
/// * [`Failure::Local`] if the results can't be written to stdout.
/// * Forwards [`reqwest::Response::chunk`]'s return values as [`Failure::ApiServer`] on failure.
/// * Forwards [`handle_line`]'s return values on failure.
async fn write_results(
    mut response: reqwest::Response,
    query_job_id: &mut Option<u64>,
) -> Result<(), Failure> {
    use std::io::Write;

    let mut event_name = None;
    // The received bytes that aren't handled yet, i.e., the start of an incomplete line.
    let mut unhandled = Vec::new();
    // The results that the received chunk completes.
    let mut results = Vec::new();
    while let Some(chunk) = response
        .chunk()
        .await
        .context("failed to read the API server's response")
        .map_err(Failure::ApiServer)?
    {
        unhandled.extend_from_slice(&chunk);
        let Some(last_newline_pos) = unhandled.iter().rposition(|&byte| byte == b'\n') else {
            continue;
        };
        let mut outcome = Ok(ControlFlow::Continue(()));
        for line in unhandled[..last_newline_pos].split(|&byte| byte == b'\n') {
            outcome = handle_line(line, &mut event_name, &mut results, query_job_id);
            if !matches!(outcome, Ok(ControlFlow::Continue(()))) {
                break;
            }
        }
        unhandled.drain(..=last_newline_pos);

        if !results.is_empty() {
            let mut stdout = std::io::stdout().lock();
            match stdout.write_all(&results).and_then(|()| stdout.flush()) {
                Err(error) if error.kind() == std::io::ErrorKind::BrokenPipe => return Ok(()),
                result => result
                    .context("failed to write the results")
                    .map_err(Failure::Local)?,
            }
            results.clear();
        }
        if outcome?.is_break() {
            return Ok(());
        }
    }
    Err(Failure::ApiServer(anyhow::anyhow!(
        "the API server's response ended before the query job did"
    )))
}

/// Handles `line`, a line of a streaming query's response without its newline.
///
/// An `event: <name>` line names the event of the next `data: <data>` line in `event_name`, and a
/// `data:` line without a name is a result, whose data is appended to `results` as it is, on its
/// own line. The `job` event's query job ID is stored in `query_job_id`. Comments and blank lines
/// are skipped, and any other line is an error.
///
/// # Returns
///
/// [`ControlFlow::Break`] if the `end` event says that the query job succeeded, or
/// [`ControlFlow::Continue`] if the query job hasn't ended, on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`Failure::QueryJob`] if the `end` event's status isn't [`QueryJobStatus::Succeeded`].
/// * [`Failure::ApiServer`] if:
///   * The data of a `job`, `end` or `error` event doesn't deserialize.
///   * The line is an `error` event, or an event with an unknown name.
///   * The line isn't an `event:` or `data:` line, a comment, or a blank line.
fn handle_line(
    line: &[u8],
    event_name: &mut Option<Vec<u8>>,
    results: &mut Vec<u8>,
    query_job_id: &mut Option<u64>,
) -> Result<ControlFlow<()>, Failure> {
    let line = line.strip_suffix(b"\r").unwrap_or(line);
    if line.is_empty() || line.starts_with(b":") {
        return Ok(ControlFlow::Continue(()));
    }
    if let Some(name) = line.strip_prefix(b"event: ") {
        *event_name = Some(name.to_vec());
        return Ok(ControlFlow::Continue(()));
    }
    let Some(data) = line.strip_prefix(b"data: ") else {
        return Err(Failure::ApiServer(anyhow::anyhow!(
            "the API server sent an unexpected line {:?}",
            String::from_utf8_lossy(line)
        )));
    };
    match event_name.take().as_deref() {
        None => {
            results.extend_from_slice(data);
            results.push(b'\n');
        }
        Some(b"job") => {
            let job: StreamingQueryJobEvent = serde_json::from_slice(data)
                .context("invalid `job` event")
                .map_err(Failure::ApiServer)?;
            tracing::debug!(query_job_id = job.query_job_id, "Submitted the query job.");
            *query_job_id = Some(job.query_job_id);
        }
        Some(b"end") => {
            let StreamingQueryEndEvent {
                status,
                num_results_emitted,
            } = serde_json::from_slice(data)
                .context("invalid `end` event")
                .map_err(Failure::ApiServer)?;
            tracing::debug!(status = ? status, num_results_emitted, "The query job ended.");
            if status == QueryJobStatus::Succeeded {
                return Ok(ControlFlow::Break(()));
            }
            return Err(Failure::QueryJob(anyhow::anyhow!(
                "ended with status `{status:?}`"
            )));
        }
        Some(b"error") => {
            let StreamingQueryErrorEvent { message } = serde_json::from_slice(data)
                .context("invalid `error` event")
                .map_err(Failure::ApiServer)?;
            return Err(Failure::ApiServer(anyhow::anyhow!(
                "the API server failed during the search: {message}"
            )));
        }
        Some(name) => {
            return Err(Failure::ApiServer(anyhow::anyhow!(
                "the API server sent an unknown event `{}`",
                String::from_utf8_lossy(name)
            )));
        }
    }
    Ok(ControlFlow::Continue(()))
}

//! `clp-search`: searches the compressed logs through the API server's `POST /query/stream` route,
//! and writes each result to stdout as a JSON object on its own line.

use std::io::Write;
use std::path::Path;
use std::path::PathBuf;
use std::process::ExitCode;
use std::time::Duration;

use anyhow::Context;
use clap::Parser;
use clap::builder::NonEmptyStringValueParser;
use clap::builder::TypedValueParser;
use clp_rust_utils::api_server::streaming_query::StreamingQueryConfig;
use clp_rust_utils::api_server::streaming_query::StreamingQueryEndEvent;
use clp_rust_utils::api_server::streaming_query::StreamingQueryErrorEvent;
use clp_rust_utils::api_server::streaming_query::StreamingQueryJobEvent;
use clp_rust_utils::clp_config::package::DEFAULT_CONFIG_FILE_PATH;
use clp_rust_utils::clp_config::package::config::ApiServer;
use clp_rust_utils::dataset::DatasetName;
use clp_rust_utils::dataset::NonEmptyDatasetNames;
use clp_rust_utils::job_config::QueryJobStatus;
use clp_rust_utils::job_config::SearchQuery;
use clp_rust_utils::job_config::TimeRange;
use non_empty_string::NonEmptyString;
use reqwest::Url;
use serde::Deserialize;
use tokio::io::AsyncBufReadExt;

/// The longest time that connecting to the API server may take.
const CONNECT_TIMEOUT: Duration = Duration::from_secs(5);

/// The longest time that a read from the API server may take. It must be longer than the 15 s
/// interval of the API server's keep-alive comments, so that an idle search isn't failed.
const READ_TIMEOUT: Duration = Duration::from_secs(60);

/// The directory at which the container mounts the host's root directory.
const CONTAINER_HOST_ROOT: &str = "/mnt/host";

/// The exit code of a search that a signal interrupted.
const INTERRUPTED_EXIT_CODE: u8 = 130;

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

/// The part of the package config, `clp_py_utils.clp_config.ClpConfig`, that configures the API
/// server.
///
/// # NOTE
///
/// * This type is partially defined: unused fields are omitted and discarded through
///   deserialization.
/// * The default values must be kept in sync with the Python definition.
#[derive(Default, Deserialize)]
#[serde(default)]
struct PackageConfig {
    api_server: ApiServer,
}

/// # Returns
///
/// The exit code of the search: 0 if it succeeds or stdout is closed, 1 if it fails, and
/// [`INTERRUPTED_EXIT_CODE`] if a signal interrupts it. Invalid arguments exit with code 2.
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

    set_up_logging(args.verbose);
    run(args.config.as_deref(), &streaming_query_config).unwrap_or_else(|error| {
        tracing::error!(error = % format_args!("{error:#}"), "The search failed.");
        ExitCode::FAILURE
    })
}

/// Logs to stderr, at the debug level for this binary if `is_verbose`, and at the info level
/// otherwise.
fn set_up_logging(is_verbose: bool) {
    use std::io::IsTerminal;

    let filter = if is_verbose {
        "info,clp_search=debug"
    } else {
        "info"
    };
    tracing_subscriber::fmt()
        .with_env_filter(tracing_subscriber::EnvFilter::new(filter))
        .with_writer(std::io::stderr)
        .with_ansi(std::io::stderr().is_terminal())
        .with_target(false)
        .init();
}

// yscope-rust-linter: allow(errors-doc): `search` is forwarded through a `tokio::select!` arm
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
/// * Forwards [`resolve_config_path`]'s return values on failure.
/// * Forwards [`read_stream_query_url`]'s return values on failure.
/// * Forwards [`tokio::runtime::Builder::build`]'s return values on failure.
/// * Forwards [`tokio::signal::unix::signal`]'s return values on failure.
/// * Forwards [`search`]'s return values on failure.
fn run(
    host_config_path: Option<&Path>,
    streaming_query_config: &StreamingQueryConfig,
) -> anyhow::Result<ExitCode> {
    use tokio::signal::unix::SignalKind;
    use tokio::signal::unix::signal;

    let config_path = resolve_config_path(host_config_path)?;
    let stream_query_url = read_stream_query_url(&config_path)?;
    let runtime = tokio::runtime::Builder::new_current_thread()
        .enable_all()
        .build()
        .context("failed to create the runtime")?;
    runtime.block_on(async {
        // As the container's PID 1, the process has no default action for these signals.
        let mut interrupt =
            signal(SignalKind::interrupt()).context("failed to listen for SIGINT")?;
        let mut terminate =
            signal(SignalKind::terminate()).context("failed to listen for SIGTERM")?;
        let signal_name = tokio::select! {
            result = search(stream_query_url, streaming_query_config) => {
                return result.map(|()| ExitCode::SUCCESS);
            }
            _ = interrupt.recv() => "SIGINT",
            _ = terminate.recv() => "SIGTERM",
        };
        tracing::warn!(
            signal = signal_name,
            "The search was interrupted. The API server cancels the query job."
        );
        Ok(ExitCode::from(INTERRUPTED_EXIT_CODE))
    })
}

/// Resolves the path at which the container reads the package config.
///
/// Without `host_config_path`, the path is [`DEFAULT_CONFIG_FILE_PATH`] joined onto `CLP_HOME`.
/// Otherwise, `host_config_path` is a path on the host: a leading `~` component is replaced with
/// `HOME`, a relative path is joined onto `CLP_PWD_HOST`, and the resulting absolute path is joined
/// under [`CONTAINER_HOST_ROOT`], where the container mounts the host's root directory. Symlinks
/// and `..` components are left as they are.
///
/// # Returns
///
/// The path of the package config in the container on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`anyhow::Error`] if:
///   * An environment variable that the resolution needs isn't set.
///   * The path on the host is still relative once it's joined onto `CLP_PWD_HOST`.
fn resolve_config_path(host_config_path: Option<&Path>) -> anyhow::Result<PathBuf> {
    use std::env::var_os;

    let Some(host_config_path) = host_config_path else {
        let clp_home = var_os("CLP_HOME").context("`CLP_HOME` isn't set")?;
        return Ok(Path::new(&clp_home).join(DEFAULT_CONFIG_FILE_PATH));
    };
    let host_config_path = match host_config_path.strip_prefix("~") {
        Ok(path_in_home) => {
            let home = var_os("HOME").context("`HOME` isn't set")?;
            Path::new(&home).join(path_in_home)
        }
        Err(_) => host_config_path.to_path_buf(),
    };
    let host_config_path = if host_config_path.is_relative() {
        let pwd_host = var_os("CLP_PWD_HOST").context("`CLP_PWD_HOST` isn't set")?;
        Path::new(&pwd_host).join(host_config_path)
    } else {
        host_config_path
    };
    let host_config_path_from_root = host_config_path
        .strip_prefix("/")
        .with_context(|| format!("the host path {} is relative", host_config_path.display()))?;
    Ok(Path::new(CONTAINER_HOST_ROOT).join(host_config_path_from_root))
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
    let ApiServer { host, port, .. } = config.unwrap_or_default().api_server;
    let url = format!("http://{host}:{port}/query/stream");
    Url::parse(&url).with_context(|| format!("invalid API server URL {url}"))
}

/// Submits `streaming_query_config`'s query to the streaming query route at `stream_query_url`, and
/// writes the data of each result to stdout as it is, on its own line, until the query job ends.
///
/// The response body is read line by line. An `event: <name>` line names the event of the next
/// `data: <data>` line, and a `data:` line without a name is a result. Other lines, such as
/// comments and blank lines, are skipped. Once stdout is closed, writing stops and the function
/// succeeds.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`anyhow::Error`] if:
///   * The API server responds with an unsuccessful status.
///   * The `end` event's status isn't `Succeeded`.
///   * The API server sends an `error` event, or an event with an unknown name.
///   * The response body ends before an `end` or `error` event.
/// * Forwards [`reqwest::ClientBuilder::build`]'s return values on failure.
/// * Forwards [`reqwest::RequestBuilder::send`]'s return values on failure.
/// * Forwards [`reqwest::Response::text`]'s return values on failure.
/// * Forwards [`tokio::io::Lines::next_line`]'s return values on failure.
/// * Forwards [`serde_json::from_str`]'s return values on failure.
/// * Forwards [`Write::flush`]'s return values on failure.
///
/// # Cancel safety
///
/// Dropping the future closes the connection, after which the API server cancels the query job.
async fn search(
    stream_query_url: Url,
    streaming_query_config: &StreamingQueryConfig,
) -> anyhow::Result<()> {
    use futures::TryStreamExt;

    let response = reqwest::Client::builder()
        .no_proxy()
        .connect_timeout(CONNECT_TIMEOUT)
        .read_timeout(READ_TIMEOUT)
        .build()
        .context("failed to build the HTTP client")?
        .post(stream_query_url)
        .header(reqwest::header::ACCEPT, "text/event-stream")
        .json(streaming_query_config)
        .send()
        .await
        .context("failed to send the query to the API server")?;
    let status = response.status();
    if !status.is_success() {
        let body = response
            .text()
            .await
            .context("failed to read the API server's response")?;
        anyhow::bail!("the API server responded with status {status}: {body}");
    }
    let mut lines =
        tokio_util::io::StreamReader::new(response.bytes_stream().map_err(std::io::Error::other))
            .lines();
    let mut stdout = std::io::stdout();
    let mut event_name = None;
    let mut query_job_id = None;
    while let Some(line) = lines
        .next_line()
        .await
        .context("failed to read the API server's response")?
    {
        if let Some(name) = line.strip_prefix("event: ") {
            event_name = Some(name.to_owned());
            continue;
        }
        let Some(data) = line.strip_prefix("data: ") else {
            continue;
        };
        match event_name.take().as_deref() {
            None => match writeln!(stdout, "{data}").and_then(|()| stdout.flush()) {
                Err(error) if error.kind() == std::io::ErrorKind::BrokenPipe => return Ok(()),
                result => result.context("failed to write a result")?,
            },
            Some("job") => {
                let job: StreamingQueryJobEvent =
                    serde_json::from_str(data).context("invalid `job` event")?;
                tracing::debug!(query_job_id = job.query_job_id, "Submitted the query job.");
                query_job_id = Some(job.query_job_id);
            }
            Some("end") => {
                let StreamingQueryEndEvent { status, .. } =
                    serde_json::from_str(data).context("invalid `end` event")?;
                if status == QueryJobStatus::Succeeded {
                    return Ok(());
                }
                match query_job_id {
                    Some(id) => anyhow::bail!("query job {id} ended with status `{status:?}`"),
                    None => anyhow::bail!("the query job ended with status `{status:?}`"),
                }
            }
            Some("error") => {
                let StreamingQueryErrorEvent { message } =
                    serde_json::from_str(data).context("invalid `error` event")?;
                anyhow::bail!("the API server failed during the search: {message}");
            }
            Some(name) => anyhow::bail!("the API server sent an unknown event `{name}`"),
        }
    }
    anyhow::bail!("the API server's response ended before the query job did")
}

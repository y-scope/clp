//! `clp-search`: searches the compressed logs through the API server's streaming query route, and
//! writes each result to stdout as a JSON object on its own line.

use std::path::Path;
use std::path::PathBuf;

use clap::Parser;
use clap::builder::NonEmptyStringValueParser;
use clap::builder::TypedValueParser;
use clp_package_cli::api_server::Client;
use clp_package_cli::config;
use clp_package_cli::config::HostEnv;
use clp_package_cli::search;
use clp_package_cli::search::ExitCode;
use clp_rust_utils::api_server::streaming_query::StreamingQueryConfig;
use clp_rust_utils::dataset::DatasetName;
use clp_rust_utils::dataset::NonEmptyDatasetNames;
use clp_rust_utils::job_config::SearchQuery;
use clp_rust_utils::job_config::TimeRange;
use non_empty_string::NonEmptyString;

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

fn main() -> std::process::ExitCode {
    use clap::CommandFactory;
    use clap::error::ErrorKind;

    let Args {
        query,
        datasets,
        begin_time,
        end_time,
        ignore_case,
        config,
        verbose,
    } = Args::parse();
    let time_range = TimeRange::new(begin_time, end_time).unwrap_or_else(|error| {
        Args::command()
            .error(ErrorKind::ArgumentConflict, error)
            .exit()
    });
    let streaming_query_config = StreamingQueryConfig {
        query: SearchQuery {
            datasets: NonEmptyDatasetNames::try_from(datasets)
                .expect("`--dataset` should default to the default dataset"),
            query_string: query,
            time_range,
            ignore_case,
        },
    };

    set_up_logging(verbose);
    run_search(config.as_deref(), &streaming_query_config).into()
}

/// Logs to stderr, at the debug level for this package's crates if `is_verbose`, and at the info
/// level otherwise.
fn set_up_logging(is_verbose: bool) {
    use std::io::IsTerminal;

    let filter = if is_verbose {
        "info,clp_package_cli=debug,clp_search=debug"
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

/// Runs `streaming_query_config`'s search through the API server that the package config at
/// `host_config_path`, or the default package config, configures.
///
/// # Returns
///
/// The exit code of the search.
fn run_search(
    host_config_path: Option<&Path>,
    streaming_query_config: &StreamingQueryConfig,
) -> ExitCode {
    let Ok(config_path) = HostEnv::from_env()
        .config_path(host_config_path)
        .inspect_err(|error| {
            tracing::error!(error = % error, "Failed to locate the package config.");
        })
    else {
        return ExitCode::LocalError;
    };
    let api_server_url = match config::read_api_server_url(&config_path) {
        Ok(Some(api_server_url)) => api_server_url,
        Ok(None) => {
            tracing::error!(
                config_path = % config_path.display(),
                "Search requires the API server, which the package config disables \
                 (`api_server: null`)."
            );
            return ExitCode::LocalError;
        }
        Err(error) => {
            tracing::error!(
                config_path = % config_path.display(),
                error = % error,
                "Failed to read the API server's address from the package config."
            );
            return ExitCode::LocalError;
        }
    };
    let Ok(client) = Client::new(api_server_url).inspect_err(|error| {
        tracing::error!(error = % error, "Failed to create the API server's client.");
    }) else {
        return ExitCode::LocalError;
    };
    let Ok(runtime) = tokio::runtime::Builder::new_current_thread()
        .enable_all()
        .build()
        .inspect_err(|error| {
            tracing::error!(error = % error, "Failed to create the runtime.");
        })
    else {
        return ExitCode::LocalError;
    };

    runtime.block_on(async {
        let Ok(interrupt) = interrupt_signal().inspect_err(|error| {
            tracing::error!(error = % error, "Failed to listen for signals.");
        }) else {
            return ExitCode::LocalError;
        };
        search::run(
            client.stream_query(streaming_query_config),
            std::io::stdout(),
            interrupt,
        )
        .await
    })
}

/// Listens for SIGINT and SIGTERM, which the process doesn't otherwise act on when it runs as a
/// container's PID 1.
///
/// # Returns
///
/// A future that completes when the process receives SIGINT or SIGTERM on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`tokio::signal::unix::signal`]'s return values on failure.
fn interrupt_signal() -> std::io::Result<impl Future<Output = ()> + Send + use<>> {
    use tokio::signal::unix::SignalKind;
    use tokio::signal::unix::signal;

    let mut interrupt = signal(SignalKind::interrupt())?;
    let mut terminate = signal(SignalKind::terminate())?;
    Ok(async move {
        tokio::select! {
            _ = interrupt.recv() => {}
            _ = terminate.recv() => {}
        }
    })
}

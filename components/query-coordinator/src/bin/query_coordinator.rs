//! The query coordinator executable.

use std::path::PathBuf;
use std::time::Duration;

use clap::Parser;
use clp_rust_utils::clp_config::package::{self};
use clp_rust_utils::database::mysql::create_clp_db_mysql_pool;
use clp_rust_utils::serde::yaml;
use query_coordinator::coordination::Coordinator;
use tokio_util::sync::CancellationToken;

/// Command-line arguments for the query coordinator.
#[derive(Debug, Parser)]
#[command(about = "Run the query coordinator.")]
struct Cli {
    /// Path to the configuration file.
    #[arg(short, long, value_name = "PATH")]
    config: PathBuf,
}

/// Runs the query coordinator with the configuration file given on the command line until it
/// returns or a shutdown signal arrives.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`anyhow::Error`] if the query coordinator or Spider configuration is missing.
/// * Forwards [`yaml::from_path`]'s return values on failure.
/// * Forwards [`read_database_credentials`]'s return values on failure.
/// * Forwards [`create_clp_db_mysql_pool`]'s return values on failure.
/// * Forwards [`Coordinator::new`]'s return values on failure.
/// * Forwards [`run_until_shutdown`]'s return values on failure.
#[tokio::main]
async fn main() -> anyhow::Result<()> {
    let args = Cli::parse();

    let _guard = clp_rust_utils::logging::set_up_logging("query_coordinator.log");

    let config: package::config::Config = yaml::from_path(args.config).inspect_err(|e| {
        tracing::error!(error = % e, "Failed to load the configuration file.");
    })?;

    let database_credentials = read_database_credentials()?;

    let coordinator_config = config.query_coordinator.ok_or_else(|| {
        tracing::error!("Query coordinator configuration is missing.");
        anyhow::anyhow!("query coordinator configuration is missing")
    })?;

    let spider_config = config.spider.ok_or_else(|| {
        tracing::error!("Spider configuration is missing.");
        anyhow::anyhow!("spider configuration is missing")
    })?;

    let db_pool = create_clp_db_mysql_pool(
        &config.database,
        &database_credentials,
        coordinator_config.database_connection_pool_size.get(),
    )
    .await
    .inspect_err(|e| tracing::error!(error = % e, "Failed to create the database pool."))?;

    let (coordinator, cancellation_token) = Coordinator::new(
        &coordinator_config,
        &spider_config,
        &config.results_cache,
        &config.archive_output,
        db_pool,
        config.database,
    )
    .await
    .inspect_err(|e| tracing::error!(error = % e, "Failed to create the query coordinator."))?;

    run_until_shutdown(
        coordinator,
        cancellation_token,
        Duration::from_secs(coordinator_config.termination_timeout_secs.get()),
    )
    .await
}

/// Reads the CLP database credentials from the `CLP_DB_USER` and `CLP_DB_PASS` environment
/// variables.
///
/// # Returns
///
/// The database credentials on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`std::env::var`]'s return values on failure.
fn read_database_credentials() -> anyhow::Result<package::credentials::Database> {
    Ok(package::credentials::Database {
        password: secrecy::SecretString::new(
            std::env::var("CLP_DB_PASS")
                .inspect_err(|e| {
                    tracing::error!(
                        error = % e,
                        "Failed to read the database password from `CLP_DB_PASS`."
                    );
                })?
                .into_boxed_str(),
        ),
        user: std::env::var("CLP_DB_USER").inspect_err(|e| {
            tracing::error!(
                error = % e,
                "Failed to read the database user from `CLP_DB_USER`."
            );
        })?,
    })
}

/// Runs the coordinator until it returns or a shutdown signal arrives, then requests a graceful
/// stop and waits for it up to `termination_timeout`.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`anyhow::Error`] if:
///   * The coordinator returns on error.
///   * The coordinator task cannot be joined.
///
/// # Panics
///
/// Panics if listening for `SIGTERM` fails.
async fn run_until_shutdown(
    coordinator: Coordinator,
    cancellation_token: CancellationToken,
    termination_timeout: Duration,
) -> anyhow::Result<()> {
    let mut coordinator_handle = tokio::spawn(coordinator.run());

    let mut sigterm = tokio::signal::unix::signal(tokio::signal::unix::SignalKind::terminate())
        .expect("failed to listen for SIGTERM");

    // `None` if a shutdown signal arrived while the coordinator is still running; `Some` if the
    // coordinator returned on its own (an early exit, possibly on error).
    let early_exit_result = tokio::select! {
        _ = sigterm.recv() => {
            tracing::info!("Received SIGTERM.");
            None
        }
        result = tokio::signal::ctrl_c() => {
            if let Err(e) = result {
                tracing::error!(error = % e, "Failed to listen to ctrl-c.");
            }
            tracing::info!("Forcefully shutting down.");
            None
        }
        join_result = &mut coordinator_handle => Some(join_result),
    };

    // Request a graceful stop. A no-op if the coordinator has already returned.
    cancellation_token.cancel();

    let join_result = if let Some(join_result) = early_exit_result {
        join_result
    } else if let Ok(join_result) =
        tokio::time::timeout(termination_timeout, &mut coordinator_handle).await
    {
        join_result
    } else {
        tracing::warn!(
            "The query coordinator did not stop within {termination_timeout:?}. Aborting."
        );
        coordinator_handle.abort();
        return Ok(());
    };

    match join_result {
        Ok(Ok(())) => {
            tracing::info!("Query coordinator stopped.");
            Ok(())
        }
        Ok(Err(e)) => {
            tracing::error!(error = % e, "Query coordinator returned on error.");
            Err(anyhow::anyhow!("query coordinator returned on error"))
        }
        Err(e) => {
            tracing::error!(error = % e, "Failed to join the query coordinator.");
            Err(anyhow::anyhow!("failed to join the query coordinator"))
        }
    }
}

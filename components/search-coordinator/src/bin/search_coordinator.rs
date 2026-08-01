use std::path::PathBuf;
use std::time::Duration;

use clap::Parser;
use clp_rust_utils::clp_config::package;
use clp_rust_utils::serde::yaml;
use search_coordinator::coordination::SearchCoordinator;

/// Command-line arguments for the search coordinator.
#[derive(Debug, Parser)]
#[command(about = "Run the search coordinator.")]
struct Cli {
    /// Path to the configuration file.
    #[arg(short, long, value_name = "PATH")]
    config: PathBuf,
}

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    let args = Cli::parse();

    let _guard = clp_rust_utils::logging::set_up_logging("search_coordinator.log");

    let _: package::config::Config = yaml::from_path(args.config).inspect_err(|e| {
        tracing::error!(error = % e, "Failed to load the configuration file.");
    })?;

    let (coordinator, cancellation_token) = SearchCoordinator::new();
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
    } else {
        const TERMINATION_TIMEOUT: Duration = Duration::from_secs(30);
        if let Ok(join_result) =
            tokio::time::timeout(TERMINATION_TIMEOUT, &mut coordinator_handle).await
        {
            join_result
        } else {
            tracing::warn!(
                "The search coordinator did not stop within {TERMINATION_TIMEOUT:?}. Aborting."
            );
            coordinator_handle.abort();
            return Ok(());
        }
    };

    match join_result {
        Ok(Ok(())) => {
            tracing::info!("Search coordinator stopped.");
            Ok(())
        }
        Ok(Err(e)) => {
            tracing::error!(error = % e, "Search coordinator returned on error.");
            Err(anyhow::anyhow!("Search coordinator returned on error."))
        }
        Err(err) => {
            const ERROR_MESSAGE: &str = "Failed to join the search coordinator.";
            tracing::error!(error = % err, ERROR_MESSAGE);
            Err(anyhow::anyhow!(ERROR_MESSAGE))
        }
    }
}

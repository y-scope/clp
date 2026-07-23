use std::{path::PathBuf, time::Duration};

use clap::Parser;
use clp_rust_utils::{
    clp_config::package::{self},
    database::mysql::create_clp_db_mysql_pool,
    serde::yaml,
};

/// Command-line arguments for the compression coordinator.
#[derive(Debug, Parser)]
#[command(about = "Run the compression coordinator.")]
struct Cli {
    /// Path to the configuration file.
    #[arg(short, long, value_name = "PATH")]
    config: PathBuf,
}

#[allow(clippy::too_many_lines)]
#[tokio::main]
async fn main() -> anyhow::Result<()> {
    let args = Cli::parse();

    let _guard = clp_rust_utils::logging::set_up_logging("compression_coordinator.log");

    let config: package::config::Config = yaml::from_path(args.config).inspect_err(|e| {
        tracing::error!(error = % e, "Failed to load the configuration file.");
    })?;

    let credentials = package::credentials::Credentials {
        database: package::credentials::Database {
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
        },
    };

    let coordinator_config = config.compression_coordinator.ok_or_else(|| {
        const ERROR_MESSAGE: &str = "Compression coordinator configuration is missing.";
        tracing::error!(ERROR_MESSAGE);
        anyhow::anyhow!(ERROR_MESSAGE)
    })?;

    let spider_config = config.spider.ok_or_else(|| {
        const ERROR_MESSAGE: &str = "Spider configuration is missing.";
        tracing::error!(ERROR_MESSAGE);
        anyhow::anyhow!(ERROR_MESSAGE)
    })?;

    let db_pool = create_clp_db_mysql_pool(
        &config.database,
        &credentials.database,
        coordinator_config.database_connection_pool_size.get(),
    )
    .await
    .inspect_err(|e| tracing::error!(error = % e, "Failed to create the database pool."))?;

    let (coordinator, cancellation_token) =
        compression_coordinator::coordination::Coordinator::new(
            &coordinator_config,
            &spider_config,
            db_pool,
            config.database,
        )
        .await?;

    let mut coordinator_handle = tokio::spawn(coordinator.run());

    let mut sigterm = tokio::signal::unix::signal(tokio::signal::unix::SignalKind::terminate())
        .expect("failed to listen for SIGTERM");

    // `None` if a shutdown signal arrived while the coordinator is still running; `Some` if the
    // coordinator returned on its own (an early exit, possibly on error).
    let early_exit = tokio::select! {
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

    let join_result = if let Some(join_result) = early_exit {
        join_result
    } else {
        let termination_timeout =
            Duration::from_secs(coordinator_config.termination_timeout_secs.get());
        if let Ok(join_result) =
            tokio::time::timeout(termination_timeout, &mut coordinator_handle).await
        {
            join_result
        } else {
            tracing::warn!(
                "The compression coordinator did not stop within {termination_timeout:?}. \
                 Aborting."
            );
            coordinator_handle.abort();
            return Ok(());
        }
    };

    match join_result {
        Ok(Ok(())) => {
            tracing::info!("Compression coordinator stopped.");
            Ok(())
        }
        Ok(Err(e)) => {
            tracing::error!(error = % e, "Compression coordinator returned on error.");
            Err(anyhow::anyhow!(
                "Compression coordinator returned on error."
            ))
        }
        Err(err) => {
            const ERROR_MESSAGE: &str = "Failed to join the compression coordinator.";
            tracing::error!(error = % err, ERROR_MESSAGE);
            Err(anyhow::anyhow!(ERROR_MESSAGE))
        }
    }
}

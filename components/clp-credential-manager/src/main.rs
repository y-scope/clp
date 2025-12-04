mod config;
mod error;
mod routes;
mod service;

use std::path::PathBuf;

use anyhow::Context;
use clap::Parser;
use tokio::net::TcpListener;
use tracing::info;
use tracing_appender::rolling::{RollingFileAppender, Rotation};
use tracing_subscriber::{self, fmt::writer::MakeWriterExt};

use crate::{
    config::CredentialManagerConfig,
    routes::build_router,
    service::{CredentialManagerService, CredentialManagerServiceState},
};

/// CLI arguments accepted by the credential manager binary.
#[derive(Debug, Parser)]
#[command(author, version, about = "CLP Credential Manager service", long_about = None)]
struct Args {
    #[arg(
        short = 'c',
        long = "config",
        value_name = "FILE",
        default_value = "credential-manager-config.yml"
    )]
    config: PathBuf,
}

/// Binary entry point that configures logging, loads config, and starts Axum.
///
/// # Returns
///
/// `Ok(())` once the HTTP server shuts down gracefully.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`CredentialManagerConfig::from_file`]'s return values on failure.
/// * Forwards [`CredentialManagerService::new`]'s return values on failure.
/// * Forwards [`TcpListener::bind`]'s return values on failure.
/// * Forwards [`axum::serve`]'s return values on failure.
#[tokio::main]
async fn main() -> anyhow::Result<()> {
    let args = Args::parse();

    let _guard = set_up_logging()?;

    let config = CredentialManagerConfig::from_file(&args.config)?;
    let server_config = config.server.clone();
    let service = CredentialManagerService::new(&config).await?;
    let shared_state = CredentialManagerServiceState::new(service);

    let router = build_router().with_state(shared_state);

    let bind_address = server_config.bind_address;
    let bind_port = server_config.port;
    info!(
        address = %format!("{}:{}", bind_address, bind_port),
        "starting credential manager service",
    );

    let listener = TcpListener::bind((bind_address.as_str(), bind_port)).await?;
    axum::serve(listener, router).await?;

    Ok(())
}

/// Sets up JSON-formatted tracing using environment filters.
///
/// The subscriber honors the `RUST_LOG` environment variable when present and otherwise
/// defaults to the `info` level so local development remains verbose enough.
///
/// # Returns
///
/// `Ok(())` once the global subscriber is installed.
///
/// # Errors
///
/// Returns an error if:
///
/// * Setting up the tracing subscriber fails because it was already initialized or another
///   subscriber error occurs.
fn set_up_logging() -> anyhow::Result<tracing_appender::non_blocking::WorkerGuard> {
    let logs_directory =
        std::env::var("CLP_LOGS_DIR").context("Expect `CLP_LOGS_DIR` environment variable.")?;
    let logs_directory = std::path::Path::new(logs_directory.as_str());
    let file_appender =
        RollingFileAppender::new(Rotation::HOURLY, logs_directory, "log_ingestor.log");
    let (non_blocking_writer, guard) = tracing_appender::non_blocking(file_appender);
    tracing_subscriber::fmt()
        .event_format(
            tracing_subscriber::fmt::format()
                .with_level(true)
                .with_target(false)
                .with_file(true)
                .with_line_number(true)
                .json(),
        )
        .with_env_filter(tracing_subscriber::EnvFilter::from_default_env())
        .with_ansi(false)
        .with_writer(std::io::stdout.and(non_blocking_writer))
        .init();
    Ok(guard)
}

mod config;
mod error;
mod routes;
mod service;

use std::path::PathBuf;

use clap::Parser;
use tokio::net::TcpListener;
use tracing::info;
use tracing_subscriber::EnvFilter;

use crate::{config::AppConfig, routes::build_router, service::CredentialManagerService};

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
/// # Errors:
///
/// Returns an error if:
///
/// * Forwards [`AppConfig::from_file`]'s return values on failure.
/// * Forwards [`CredentialManagerService::new`]'s return values on failure.
/// * Forwards [`TcpListener::bind`]'s return values on failure.
/// * Forwards [`axum::serve`]'s return values on failure.
#[tokio::main]
async fn main() -> anyhow::Result<()> {
    let args = Args::parse();

    init_tracing()?;

    let config = AppConfig::from_file(&args.config)?;
    let server_config = config.server.clone();
    let service = CredentialManagerService::new(&config).await?;
    let shared_service = service.clone_shared();

    let router = build_router().with_state(shared_service);

    let bind_address = server_config.bind_address;
    let bind_port = server_config.port;
    info!(address = %format!("{}:{}", bind_address, bind_port), "starting credential manager service");

    let listener = TcpListener::bind((bind_address.as_str(), bind_port)).await?;
    axum::serve(listener, router).await?;

    Ok(())
}

/// Sets up JSON-formatted tracing using environment filters.
///
/// The subscriber honors the `RUST_LOG` environment variable when present and otherwise
/// defaults to the `info` level so local development remains verbose enough.
fn init_tracing() -> anyhow::Result<()> {
    let env_filter = EnvFilter::try_from_default_env().unwrap_or_else(|_| EnvFilter::new("info"));

    tracing_subscriber::fmt()
        .with_env_filter(env_filter)
        .with_target(false)
        .json()
        .try_init()
        .map_err(|err| anyhow::Error::msg(err.to_string()))
}

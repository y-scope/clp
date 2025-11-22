mod audit;
mod config;
mod db;
mod error;
mod models;
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
#[tokio::main]
async fn main() -> anyhow::Result<()> {
    let args = Args::parse();

    init_tracing();

    let config = AppConfig::from_file(&args.config)?;
    let server_config = config.server.clone();
    let service = CredentialManagerService::new(&config).await?;
    let shared_service = service.clone_shared();

    let router = build_router().with_state(shared_service);

    let addr = server_config.socket_addr()?;
    info!(address = %addr, "starting credential manager service");

    let listener = TcpListener::bind(addr).await?;
    axum::serve(listener, router).await?;

    Ok(())
}

/// Sets up JSON-formatted tracing using environment filters.
fn init_tracing() {
    let env_filter = EnvFilter::try_from_default_env().unwrap_or_else(|_| EnvFilter::new("info"));

    let _ = tracing_subscriber::fmt()
        .with_env_filter(env_filter)
        .with_target(false)
        .json()
        .try_init();
}

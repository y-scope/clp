use anyhow::Context;
use clap::Parser;
use clp_rust_utils::{clp_config::package, serde::yaml};
use log_ingestor::{ingestion_job_manager::IngestionJobManagerState, routes::create_router};
use tracing_subscriber;

#[derive(Parser)]
#[command(version, about = "log-ingestor for CLP.")]
struct Args {
    /// Path to the CLP config file.
    #[arg(long)]
    config: String,

    /// Host to bind the server to.
    #[arg(long)]
    host: String,

    /// Port to bind the server to.
    #[arg(long)]
    port: u16,
}

fn read_config_and_credentials(
    args: &Args,
) -> anyhow::Result<(package::config::Config, package::credentials::Credentials)> {
    let config_path = std::path::Path::new(args.config.as_str());
    let config: package::config::Config = yaml::from_path(config_path).context(format!(
        "Failed to load config file {}",
        config_path.display()
    ))?;

    let credentials = package::credentials::Credentials {
        database: package::credentials::Database {
            password: secrecy::SecretString::new(
                std::env::var("CLP_DB_PASS")
                    .context("Expect `CLP_DB_PASS` env variable")?
                    .into_boxed_str(),
            ),
            user: std::env::var("CLP_DB_USER").context("Expect `CLP_DB_USER` env variable")?,
        },
    };
    Ok((config, credentials))
}

fn set_up_logging() {
    // Logs to stdout only - Docker's fluentd driver captures and forwards to Fluent Bit
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
        .with_writer(std::io::stdout)
        .init();
}

async fn shutdown_signal() {
    let mut sigterm = tokio::signal::unix::signal(tokio::signal::unix::SignalKind::terminate())
        .expect("failed to listen for SIGTERM");
    tokio::select! {
        _ = sigterm.recv() => {
        }
        _ = tokio::signal::ctrl_c()=> {
        }
    }
}

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    let args = Args::parse();

    let (config, credentials) = read_config_and_credentials(&args)?;
    set_up_logging();

    let addr = format!("{}:{}", args.host, args.port);
    let listener = tokio::net::TcpListener::bind(&addr)
        .await
        .context(format!("Cannot listen to {addr}"))?;

    let log_ingestor_manager_state = IngestionJobManagerState::from_config(config, credentials)
        .await
        .inspect_err(|err| {
            tracing::error!(err = ? err, "Failed to create ingestion job manager from CLP config.");
        })?;

    let log_ingestor_router = create_router()
        .inspect_err(|err| {
            tracing::error!(err = ? err, "Failed to create router.");
        })?
        .with_state(log_ingestor_manager_state);
    tracing::info!("Server started at {addr}");
    axum::serve(listener, log_ingestor_router)
        .with_graceful_shutdown(shutdown_signal())
        .await?;
    Ok(())
}

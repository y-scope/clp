use anyhow::Context;
use axum::{Router, routing::get};
use clap::Parser;
use clp_rust_utils::{clp_config::package, serde::yaml};
use tracing_appender::rolling::{RollingFileAppender, Rotation};
use tracing_subscriber::{self, fmt::writer::MakeWriterExt};

fn read_config_and_credentials(
    args: &Args,
) -> anyhow::Result<(package::config::Config, package::credentials::Credentials)> {
    let config_path = std::path::Path::new(args.config.as_str());
    let config: package::config::Config = yaml::from_path(config_path).context(format!(
        "Config file {} does not exist",
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

fn set_up_logging() -> anyhow::Result<()> {
    let logs_directory =
        std::env::var("CLP_LOGS_DIR").context("Expect `CLP_LOGS_DIR` environment variable.")?;
    let logs_directory = std::path::Path::new(logs_directory.as_str());
    let file_appender =
        RollingFileAppender::new(Rotation::HOURLY, logs_directory, "log_ingestor.log");
    let (non_blocking_writer, _guard) = tracing_appender::non_blocking(file_appender);
    tracing_subscriber::fmt()
        .with_env_filter(tracing_subscriber::EnvFilter::from_default_env())
        .with_ansi(false)
        .with_writer(std::io::stdout.and(non_blocking_writer))
        .init();
    Ok(())
}

#[derive(Parser)]
#[command(version)]
struct Args {
    #[arg(long)]
    config: String,

    #[arg(long)]
    host: String,

    #[arg(long)]
    port: u16,
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

    let (_config, _credentials) = read_config_and_credentials(&args)?;
    set_up_logging()?;

    let addr = format!("{}:{}", args.host, args.port);
    let listener = tokio::net::TcpListener::bind(&addr)
        .await
        .context(format!("Cannot listen to {addr}"))?;

    let app = Router::new()
        .route("/", get(health))
        .route("/health", get(health));

    tracing::info!("Server started at {addr}");
    axum::serve(listener, app)
        .with_graceful_shutdown(shutdown_signal())
        .await?;
    Ok(())
}

async fn health() -> String {
    "Log ingestor is running".to_owned()
}

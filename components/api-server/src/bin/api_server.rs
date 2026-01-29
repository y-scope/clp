use anyhow::Context;
use clap::Parser;
use clp_rust_utils::{clp_config::package, serde::yaml};
use tracing_appender::{
    non_blocking::WorkerGuard,
    rolling::{RollingFileAppender, Rotation},
};
use tracing_subscriber::{self, fmt::writer::MakeWriterExt};

#[derive(Parser)]
#[command(version, about = "API Server for CLP.")]
struct Args {
    #[arg(long)]
    config: String,

    #[arg(long)]
    host: Option<String>,

    #[arg(long)]
    port: Option<u16>,
}

fn read_config_and_credentials(
    args: &Args,
) -> anyhow::Result<(package::config::Config, package::credentials::Credentials)> {
    let config_path = std::path::Path::new(args.config.as_str());
    let config: package::config::Config = yaml::from_path(config_path)
        .context(format!("cannot load config file {}", config_path.display()))?;

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

fn set_up_logging() -> anyhow::Result<WorkerGuard> {
    let logs_directory =
        std::env::var("CLP_LOGS_DIR").context("Expect `CLP_LOGS_DIR` environment variable.")?;
    let logs_directory = std::path::Path::new(logs_directory.as_str());
    let file_appender =
        RollingFileAppender::new(Rotation::HOURLY, logs_directory, "api_server.log");
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
    let _guard = set_up_logging()?;

    let api_server_config = config
        .api_server
        .as_ref()
        .expect("api_server configuration is missing");
    let addr = format!(
        "{}:{}",
        args.host.unwrap_or_else(|| api_server_config.host.clone()),
        args.port.unwrap_or(api_server_config.port)
    );
    let listener = tokio::net::TcpListener::bind(&addr)
        .await
        .context(format!("Cannot listen to {addr}"))?;

    let client = api_server::client::Client::connect(&config, &credentials)
        .await
        .context("Cannot connect to CLP")?;

    let router = api_server::routes::from_client(client, &config)?;

    tracing::info!("Server started at {addr}");
    axum::serve(listener, router)
        .with_graceful_shutdown(shutdown_signal())
        .await?;
    Ok(())
}

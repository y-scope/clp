use anyhow::Context;
use axum::{
    Json,
    Router,
    extract::{Path, State},
    http::StatusCode,
    response::{
        IntoResponse,
        Sse,
        sse::{Event, KeepAlive},
    },
    routing::{get, post},
};
use clap::Parser;
use clp_rust_utils::{clp_config::package, serde::yaml};
use futures::{Stream, StreamExt};
use thiserror::Error;
use tracing_appender::rolling::{RollingFileAppender, Rotation};
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
        RollingFileAppender::new(Rotation::HOURLY, logs_directory, "api_server.log");
    let (non_blocking_writer, _guard) = tracing_appender::non_blocking(file_appender);
    tracing_subscriber::fmt()
        .with_env_filter(tracing_subscriber::EnvFilter::from_default_env())
        .with_ansi(false)
        .with_writer(std::io::stdout.and(non_blocking_writer))
        .init();
    Ok(())
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
    set_up_logging()?;

    let addr = format!(
        "{}:{}",
        args.host.unwrap_or_else(|| config.api_server.host.clone()),
        args.port.unwrap_or(config.api_server.port)
    );
    let listener = tokio::net::TcpListener::bind(&addr)
        .await
        .context(format!("Cannot listen to {addr}"))?;

    let client = clp_client::Client::connect(&config, &credentials)
        .await
        .context("Cannot connect to CLP")?;

    let app = Router::new()
        .route("/", get(health))
        .route("/health", get(health))
        .route("/query", post(query))
        .route("/query_results/{search_job_id}", get(query_results))
        .with_state(client);

    tracing::info!("Server started at {addr}");
    axum::serve(listener, app)
        .with_graceful_shutdown(shutdown_signal())
        .await?;
    Ok(())
}

async fn health() -> String {
    "API server is running".to_owned()
}

async fn query(
    State(client): State<clp_client::Client>,
    Json(query_config): Json<clp_client::QueryConfig>,
) -> Result<Json<serde_json::Value>, HandlerError> {
    let search_job_id = client.submit_query(query_config).await?;
    let uri = format!("/query_results/{search_job_id}");
    Ok(Json(serde_json::json!({"query_results_uri": uri})))
}

async fn query_results(
    State(client): State<clp_client::Client>,
    Path(search_job_id): Path<u64>,
) -> Result<Sse<impl Stream<Item = Result<Event, HandlerError>>>, HandlerError> {
    let results_stream = client.fetch_results(search_job_id).await?;
    let event_stream = results_stream.map(|res| {
        let message = res?;
        let trimmed_message = message.trim();
        if trimmed_message.lines().count() != 1 {
            return Err(HandlerError::InternalServer);
        }
        Ok(Event::default().data(trimmed_message))
    });
    Ok(Sse::new(event_stream).keep_alive(KeepAlive::default()))
}

/// Generic errors for request handlers.
#[derive(Error, Debug)]
enum HandlerError {
    #[error("Internal server error")]
    InternalServer,
}

trait IntoHandlerError {}

impl IntoHandlerError for axum::Error {}

impl IntoHandlerError for clp_client::ClientError {}

impl<T: IntoHandlerError> From<T> for HandlerError {
    fn from(_: T) -> Self {
        Self::InternalServer
    }
}

/// Converts [`HandlerError`] into an HTTP response.
impl IntoResponse for HandlerError {
    fn into_response(self) -> axum::response::Response {
        StatusCode::INTERNAL_SERVER_ERROR.into_response()
    }
}

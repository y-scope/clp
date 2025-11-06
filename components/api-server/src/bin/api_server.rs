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
use tracing_subscriber::{self, prelude::*};

#[derive(Parser)]
#[command(version, about = "API Server for CLP.")]
struct Args {}

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    tracing_subscriber::registry()
        .with(tracing_subscriber::fmt::layer())
        .with(tracing_subscriber::EnvFilter::from_default_env())
        .init();

    let _ = Args::parse();
    let home = std::env::var("CLP_HOME").context("Expect `CLP_HOME` env variable")?;
    let home = std::path::Path::new(&home);

    let config_path = home.join(package::DEFAULT_CONFIG_FILE_PATH);
    let config: package::config::Config = yaml::from_path(&config_path).context(format!(
        "Config file {} does not exist",
        config_path.display()
    ))?;

    let credentials_path = home.join(package::DEFAULT_CREDENTIALS_FILE_PATH);
    let credentials: package::credentials::Credentials = yaml::from_path(&credentials_path)
        .context(format!(
            "Credentials file {} does not exist",
            credentials_path.display()
        ))?;

    let addr = format!("{}:{}", &config.api_server.host, &config.api_server.port);
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
        .with_graceful_shutdown(async {
            tokio::signal::ctrl_c()
                .await
                .expect("failed to listen for event");
        })
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

impl IntoResponse for HandlerError {
    fn into_response(self) -> axum::response::Response {
        StatusCode::INTERNAL_SERVER_ERROR.into_response()
    }
}

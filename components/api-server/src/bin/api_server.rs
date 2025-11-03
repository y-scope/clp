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
use clp_rust_utils::clp_config::package;
use futures::{Stream, StreamExt};
use thiserror::Error;
use tracing_subscriber::{self, prelude::*};

#[derive(Parser)]
#[command(version, about)]
struct Args {
    #[arg(long)]
    package_root: String,
}

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    tracing_subscriber::registry()
        .with(tracing_subscriber::fmt::layer())
        .with(tracing_subscriber::EnvFilter::from_default_env())
        .init();

    let args = Args::parse();
    let package_root = std::path::Path::new(&args.package_root);

    let config_path = package_root.join(package::DEFAULT_CONFIG_FILE_PATH);
    let config_string = std::fs::read_to_string(config_path)?;
    let config: package::config::Config = serde_yaml::from_str(&config_string)?;

    let credentials_path = package_root.join(package::DEFAULT_CREDENTIALS_FILE_PATH);
    let credentials_string = std::fs::read_to_string(credentials_path)?;
    let credentials: package::credentials::Credentials = serde_yaml::from_str(&credentials_string)?;

    let client = clp_client::Client::connect(&config, &credentials).await?;

    let addr = format!("{}:{}", &config.api_server.host, &config.api_server.port);
    let listener = tokio::net::TcpListener::bind(&addr).await?;
    tracing::event!(tracing::Level::INFO, "Server started at {addr}");

    let app = Router::new()
        .route("/", get(root))
        .route("/query", post(query))
        .route("/query_results/{search_job_id}", get(query_results))
        .with_state(client);
    axum::serve(listener, app).await?;
    Ok(())
}

async fn root() -> String {
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
    let stream = client.fetch_results(search_job_id).await?;
    Ok(
        Sse::new(stream.map(|res| Ok(Event::default().json_data(res?)?)))
            .keep_alive(KeepAlive::default()),
    )
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

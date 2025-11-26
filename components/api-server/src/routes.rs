use axum::{
    Json,
    extract::{Path, State},
    http::StatusCode,
    response::{
        IntoResponse,
        Sse,
        sse::{Event, KeepAlive},
    },
    routing::get,
};
use futures::{Stream, StreamExt};
use serde::{Deserialize, Serialize};
use thiserror::Error;
use tower_http::cors::{Any, CorsLayer};
use utoipa::{OpenApi, ToSchema};
use utoipa_axum::{router::OpenApiRouter, routes};

use crate::client::{Client, ClientError, QueryConfig};

// `utoipa::OpenApi` triggers `clippy::needless_for_each`
#[allow(clippy::needless_for_each)]
mod api_doc {
    #[derive(utoipa::OpenApi)]
    #[openapi(info(
        title = "API Server",
        description = "API Server for CLP",
        contact(name = "YScope")
    ))]
    pub struct ApiDoc;
}
use api_doc::ApiDoc;

#[utoipa::path(
    get,
    path = "/health",
    responses((status = OK, body = String))
)]
async fn health() -> String {
    "API server is running".to_owned()
}

#[utoipa::path(
    post,
    path = "/query",
    description = "Submits a new query job.",
    request_body(
        content= QueryConfig,
        example = json!({
            "query_string": "*",
            "dataset": "default",
            "begin_timestamp": 0,
            "end_timestamp": 17_356_896,
            "ignore_case": true,
            "max_num_results": 0,
            "write_to_file": false
        })),
    responses(
        (
            status = OK,
            body = QueryResultsUri,
            description = "The URI to fetch the results of the submitted query.",
            example = json!({"query_results_uri":"/query_results/1"})
        ),
        (status = INTERNAL_SERVER_ERROR)
    )
)]
async fn query(
    State(client): State<Client>,
    Json(query_config): Json<QueryConfig>,
) -> Result<Json<QueryResultsUri>, HandlerError> {
    let search_job_id = client.submit_query(query_config).await?;
    let uri = format!("/query_results/{search_job_id}");
    Ok(Json(QueryResultsUri {
        query_results_uri: uri,
    }))
}

#[derive(Clone, Serialize, Deserialize, ToSchema)]
#[serde(deny_unknown_fields)]
struct QueryResultsUri {
    /// The uri to get the query results.
    query_results_uri: String,
}

#[utoipa::path(
    get,
    path = "/query_results/{search_job_id}",
    description = "Streams the results of a previously submitted query as Server-Sent Events \
        (SSE).",
    responses(
        (
            status = OK,
            body = String,
            content_type = "text/event-stream",
            description = "Server-Sent Events stream of query results. Each event contains a \
                single line of the query result in JSON format.",
            example = r#"data: {"timestamp": 1633036800, "message": "Example log message"}\n"#
        ),
        (status = INTERNAL_SERVER_ERROR)
    )
)]
async fn query_results(
    State(client): State<Client>,
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

impl IntoHandlerError for ClientError {}

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

/// Factory method to create an Axum router configured with all API routes.
///
/// # Returns
///
/// A newly created [`Router`] instance configured with all application routes.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`serde_json::Error`] if serializing the `OpenAPI` documentation fails.
pub fn from_client(client: Client) -> Result<axum::Router, serde_json::Error> {
    let (router, api) = OpenApiRouter::with_openapi(ApiDoc::openapi())
        .route("/", get(health))
        .routes(routes!(health))
        .routes(routes!(query))
        .routes(routes!(query_results))
        .with_state(client)
        .split_for_parts();
    let api_json = api.to_json()?;
    let router = router
        .route(
            "/openapi.json",
            get(|| async { (StatusCode::OK, api_json) }),
        )
        .layer(CorsLayer::new().allow_origin(Any));
    Ok(router)
}

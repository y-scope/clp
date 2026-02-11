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

/// Factory method to create an Axum router configured with all API routes.
///
/// # Returns
///
/// A newly created [`axum::Router`] instance configured with all application routes.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`OpenApi::to_json`]'s return values on failure.
pub fn from_client(client: Client) -> Result<axum::Router, serde_json::Error> {
    let (router, api) = OpenApiRouter::with_openapi(ApiDoc::openapi())
        .route("/", get(health))
        .routes(routes!(health))
        .routes(routes!(query))
        .routes(routes!(query_results))
        .routes(routes!(cancel_query))
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

// `utoipa::OpenApi` triggers `clippy::needless_for_each`
#[allow(clippy::needless_for_each)]
mod api_doc {
    // Using `super::...` can cause `super` to appear as a tag in the generated OpenAPI
    // documentation. Importing the paths directly prevents this issue.
    use super::{__path_cancel_query, __path_health, __path_query, __path_query_results};

    #[derive(utoipa::OpenApi)]
    #[openapi(
        info(
            title = "API Server",
            description = "API Server for CLP",
            contact(name = "YScope")
        ),
        paths(health, query, query_results, cancel_query)
    )]
    pub struct ApiDoc;
}
pub use api_doc::*;

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
            "time_range_begin_millisecs": 0,
            "time_range_end_millisecs": 17_356_896,
            "ignore_case": true,
            "max_num_results": 0,
            "buffer_results_in_mongodb": true
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
    tracing::info!("Submitting query: {:?}", query_config);
    let search_job_id = match client.submit_query(query_config).await {
        Ok(id) => {
            tracing::info!("Submitted query with search job ID: {}", id);
            id
        }
        Err(err) => {
            tracing::error!("Failed to submit query: {:?}", err);
            return Err(err.into());
        }
    };
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
    tracing::info!("Fetching results for search job ID: {}", search_job_id);
    let results_stream = match client.fetch_results(search_job_id).await {
        Ok(stream) => {
            tracing::info!(
                "Successfully initiated result stream for search job ID {}",
                search_job_id
            );
            stream
        }
        Err(err) => {
            tracing::error!(
                "Failed to fetch results for search job ID {}: {:?}",
                search_job_id,
                err
            );
            return Err(err.into());
        }
    };
    let event_stream = results_stream.map(|res| {
        let message = res?;
        let trimmed_message = message.trim();
        if trimmed_message.lines().count() != 1 {
            tracing::error!("Received malformed log line:\n{}", trimmed_message);
            return Err(HandlerError::InternalServer);
        }
        Ok(Event::default().data(trimmed_message))
    });
    Ok(Sse::new(event_stream).keep_alive(KeepAlive::default()))
}

#[utoipa::path(
    delete,
    path = "/query/{search_job_id}",
    description = "Cancels a previously submitted query job.",
    responses(
        (
            status = OK,
            description = "The cancellation request was submitted successfully."
        ),
        (
            status = NOT_FOUND,
            description = "No cancellable query job with the given ID was found."
        ),
        (status = INTERNAL_SERVER_ERROR)
    )
)]
async fn cancel_query(
    State(client): State<Client>,
    Path(search_job_id): Path<u64>,
) -> Result<StatusCode, HandlerError> {
    tracing::info!("Cancelling search job ID: {}", search_job_id);
    match client.cancel_search_job(search_job_id).await {
        Ok(()) => {
            tracing::info!(
                "Successfully submitted cancellation request for search job ID: {}",
                search_job_id
            );
            Ok(StatusCode::OK)
        }
        Err(err) => {
            tracing::error!(
                "Failed to cancel search job ID {}: {:?}",
                search_job_id,
                err
            );
            Err(err.into())
        }
    }
}

/// Generic errors for request handlers.
#[derive(Error, Debug)]
enum HandlerError {
    #[error("Internal server error")]
    InternalServer,
    #[error("Not found")]
    NotFound,
}

impl From<axum::Error> for HandlerError {
    fn from(_: axum::Error) -> Self {
        Self::InternalServer
    }
}

impl From<ClientError> for HandlerError {
    fn from(err: ClientError) -> Self {
        match err {
            ClientError::SearchJobNotFound(_) => Self::NotFound,
            _ => Self::InternalServer,
        }
    }
}

/// Converts [`HandlerError`] into an HTTP response.
impl IntoResponse for HandlerError {
    fn into_response(self) -> axum::response::Response {
        match self {
            Self::NotFound => StatusCode::NOT_FOUND.into_response(),
            Self::InternalServer => StatusCode::INTERNAL_SERVER_ERROR.into_response(),
        }
    }
}

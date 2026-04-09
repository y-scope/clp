use axum::{
    Json,
    extract::{Path, Query, State},
    http::StatusCode,
    response::{
        IntoResponse, Sse,
        sse::{Event, KeepAlive},
    },
    routing::get,
};
use futures::{Stream, StreamExt};
use serde::{Deserialize, Serialize};
use thiserror::Error;
use tower_http::cors::{Any, CorsLayer};
use utoipa::{IntoParams, OpenApi, ToSchema};
use utoipa_axum::{router::OpenApiRouter, routes};

use crate::client::{
    Client, ClientError, CompressionUsageRow, DEFAULT_JOB_STATUSES, QueryConfig, resolve_job_status,
};

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
        .routes(routes!(compression_usage))
        .route(
            "/column_metadata/{dataset_name}/timestamp",
            get(get_timestamp_column_names),
        )
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
    use super::{
        __path_cancel_query, __path_compression_usage, __path_health, __path_query,
        __path_query_results, CompressionUsageRow,
    };

    #[derive(utoipa::OpenApi)]
    #[openapi(
        info(
            title = "API Server",
            description = "API Server for CLP",
            contact(name = "YScope")
        ),
        paths(health, query, query_results, cancel_query, compression_usage),
        components(schemas(CompressionUsageRow))
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
            "datasets": ["default"],
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

async fn get_timestamp_column_names(
    State(client): State<Client>,
    Path(dataset_name): Path<String>,
) -> Result<Json<Vec<String>>, HandlerError> {
    let names = client
        .get_timestamp_column_names(&dataset_name)
        .await
        .map_err(|err| {
            tracing::error!(
                "Failed to get timestamp column names for dataset '{}': {:?}",
                dataset_name,
                err
            );
            HandlerError::from(err)
        })?;
    Ok(Json(names))
}

#[derive(Deserialize, IntoParams)]
#[into_params(parameter_in = Query)]
struct CompressionUsageParams {
    /// Start of usage window (epoch milliseconds, inclusive).
    begin_timestamp: i64,
    /// End of usage window (epoch milliseconds, inclusive).
    end_timestamp: i64,
    /// Job statuses to include as a comma-separated list (e.g.
    /// `job_status=SUCCEEDED,FAILED`). Recognized values:
    /// `RUNNING`, `SUCCEEDED`, `FAILED`, `KILLED`.
    /// Defaults to `SUCCEEDED,FAILED,KILLED` (all terminal states).
    #[serde(default)]
    job_status: Option<String>,
}

/// Validates compression usage parameters and resolves the requested job
/// statuses into their integer codes.
fn validate_compression_usage_params(
    params: &CompressionUsageParams,
) -> Result<Vec<i32>, HandlerError> {
    if params.begin_timestamp > params.end_timestamp {
        return Err(HandlerError::BadRequest(
            "begin_timestamp must be <= end_timestamp".to_owned(),
        ));
    }
    let statuses = match &params.job_status {
        Some(s) => s
            .split(',')
            .map(str::trim)
            .filter(|t| !t.is_empty())
            .map(|token| {
                resolve_job_status(token)
                    .ok_or_else(|| HandlerError::BadRequest(format!("Unknown job_status: {token}")))
            })
            .collect::<Result<Vec<_>, _>>()?,
        None => DEFAULT_JOB_STATUSES.to_vec(),
    };
    if statuses.is_empty() {
        return Err(HandlerError::BadRequest(
            "job_status must contain at least one valid status".to_owned(),
        ));
    }
    Ok(statuses)
}

#[utoipa::path(
    get,
    path = "/usage/compression",
    description = "Returns one row per compression job within the given \
        epoch-millisecond time range.",
    params(CompressionUsageParams),
    responses(
        (status = OK, body = Vec<CompressionUsageRow>),
        (status = BAD_REQUEST, description = "Invalid query parameters (e.g., begin_timestamp > end_timestamp, missing required fields)"),
        (status = INTERNAL_SERVER_ERROR)
    )
)]
async fn compression_usage(
    State(client): State<Client>,
    Query(params): Query<CompressionUsageParams>,
) -> Result<Json<Vec<CompressionUsageRow>>, HandlerError> {
    let job_statuses = validate_compression_usage_params(&params)?;
    tracing::info!(
        "Fetching compression usage: begin={}, end={}, job_statuses={:?}",
        params.begin_timestamp,
        params.end_timestamp,
        job_statuses,
    );
    client
        .get_compression_usage(params.begin_timestamp, params.end_timestamp, &job_statuses)
        .await
        .map(Json)
        .map_err(|err| {
            tracing::error!("Failed to fetch compression usage: {:?}", err);
            HandlerError::from(err)
        })
}

/// Generic errors for request handlers.
#[derive(Error, Debug)]
enum HandlerError {
    #[error("Internal server error")]
    InternalServer,
    #[error("Not found")]
    NotFound,
    #[error("Bad request: {0}")]
    BadRequest(String),
}

impl From<axum::Error> for HandlerError {
    fn from(_: axum::Error) -> Self {
        Self::InternalServer
    }
}

impl From<ClientError> for HandlerError {
    fn from(err: ClientError) -> Self {
        match err {
            ClientError::SearchJobNotFound(_) | ClientError::DatasetNotFound(_) => Self::NotFound,
            ClientError::InvalidDatasetName => Self::BadRequest(format!("{err}")),
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
            Self::BadRequest(msg) => (StatusCode::BAD_REQUEST, msg).into_response(),
        }
    }
}

#[cfg(test)]
mod tests {
    use axum::body::Body;
    use axum::http::{Request, StatusCode};
    use axum::routing::get;
    use http_body_util::BodyExt;
    use tower::ServiceExt;

    use super::*;

    /// Builds a minimal Axum app that calls [`validate_compression_usage_params`]
    /// (the shared production validation function) and returns the resolved
    /// status codes on success. No real database is needed.
    fn test_app() -> axum::Router {
        axum::Router::new().route(
            "/usage/compression",
            get(|Query(params): Query<CompressionUsageParams>| async move {
                let statuses = validate_compression_usage_params(&params)?;
                Ok::<_, HandlerError>(axum::Json(statuses))
            }),
        )
    }

    async fn get_body(response: axum::response::Response) -> String {
        let bytes = response
            .into_body()
            .collect()
            .await
            .expect("failed to read body")
            .to_bytes();
        String::from_utf8(bytes.to_vec()).expect("body is not utf-8")
    }

    #[tokio::test]
    async fn reject_begin_greater_than_end() {
        let app = test_app();
        let response = app
            .oneshot(
                Request::builder()
                    .uri("/usage/compression?begin_timestamp=200&end_timestamp=100")
                    .body(Body::empty())
                    .unwrap(),
            )
            .await
            .unwrap();
        assert_eq!(response.status(), StatusCode::BAD_REQUEST);
        let body = get_body(response).await;
        assert!(body.contains("begin_timestamp must be <= end_timestamp"));
    }

    #[tokio::test]
    async fn reject_unknown_job_status() {
        let app = test_app();
        let response = app
            .oneshot(
                Request::builder()
                    .uri(
                        "/usage/compression?begin_timestamp=0&end_timestamp=100&job_status=UNKNOWN",
                    )
                    .body(Body::empty())
                    .unwrap(),
            )
            .await
            .unwrap();
        assert_eq!(response.status(), StatusCode::BAD_REQUEST);
        let body = get_body(response).await;
        assert!(body.contains("Unknown job_status: UNKNOWN"));
    }

    #[tokio::test]
    async fn reject_lowercase_job_status() {
        let app = test_app();
        let response = app
            .oneshot(
                Request::builder()
                    .uri("/usage/compression?begin_timestamp=0&end_timestamp=100&job_status=succeeded")
                    .body(Body::empty())
                    .unwrap(),
            )
            .await
            .unwrap();
        assert_eq!(response.status(), StatusCode::BAD_REQUEST);
        let body = get_body(response).await;
        assert!(body.contains("Unknown job_status: succeeded"));
    }

    #[tokio::test]
    async fn accept_valid_params_with_defaults() {
        let app = test_app();
        let response = app
            .oneshot(
                Request::builder()
                    .uri("/usage/compression?begin_timestamp=0&end_timestamp=100")
                    .body(Body::empty())
                    .unwrap(),
            )
            .await
            .unwrap();
        assert_eq!(response.status(), StatusCode::OK);
        let body = get_body(response).await;
        assert_eq!(body, "[2,3,4]"); // SUCCEEDED, FAILED, KILLED
    }

    #[tokio::test]
    async fn accept_comma_separated_job_status() {
        let app = test_app();
        let response = app
            .oneshot(
                Request::builder()
                    .uri("/usage/compression?begin_timestamp=0&end_timestamp=100&job_status=SUCCEEDED,RUNNING")
                    .body(Body::empty())
                    .unwrap(),
            )
            .await
            .unwrap();
        assert_eq!(response.status(), StatusCode::OK);
        let body = get_body(response).await;
        assert_eq!(body, "[2,1]"); // SUCCEEDED, RUNNING
    }

    #[tokio::test]
    async fn accept_spaces_around_commas() {
        let app = test_app();
        let response = app
            .oneshot(
                Request::builder()
                    .uri("/usage/compression?begin_timestamp=0&end_timestamp=100&job_status=SUCCEEDED%2C+FAILED")
                    .body(Body::empty())
                    .unwrap(),
            )
            .await
            .unwrap();
        assert_eq!(response.status(), StatusCode::OK);
        let body = get_body(response).await;
        assert_eq!(body, "[2,3]"); // SUCCEEDED, FAILED
    }

    #[tokio::test]
    async fn accept_trailing_comma() {
        let app = test_app();
        let response = app
            .oneshot(
                Request::builder()
                    .uri("/usage/compression?begin_timestamp=0&end_timestamp=100&job_status=SUCCEEDED,")
                    .body(Body::empty())
                    .unwrap(),
            )
            .await
            .unwrap();
        assert_eq!(response.status(), StatusCode::OK);
        let body = get_body(response).await;
        assert_eq!(body, "[2]"); // trailing comma ignored
    }

    #[tokio::test]
    async fn accept_single_job_status() {
        let app = test_app();
        let response = app
            .oneshot(
                Request::builder()
                    .uri("/usage/compression?begin_timestamp=0&end_timestamp=100&job_status=KILLED")
                    .body(Body::empty())
                    .unwrap(),
            )
            .await
            .unwrap();
        assert_eq!(response.status(), StatusCode::OK);
        let body = get_body(response).await;
        assert_eq!(body, "[4]");
    }

    #[tokio::test]
    async fn reject_missing_begin_timestamp() {
        let app = test_app();
        let response = app
            .oneshot(
                Request::builder()
                    .uri("/usage/compression?end_timestamp=100")
                    .body(Body::empty())
                    .unwrap(),
            )
            .await
            .unwrap();
        assert_eq!(response.status(), StatusCode::BAD_REQUEST);
        let body = get_body(response).await;
        assert!(
            body.contains("begin_timestamp") || body.contains("deserialize"),
            "expected error about missing begin_timestamp, got: {body}"
        );
    }

    #[tokio::test]
    async fn reject_empty_job_status() {
        let app = test_app();
        let response = app
            .oneshot(
                Request::builder()
                    .uri("/usage/compression?begin_timestamp=0&end_timestamp=100&job_status=")
                    .body(Body::empty())
                    .unwrap(),
            )
            .await
            .unwrap();
        assert_eq!(response.status(), StatusCode::BAD_REQUEST);
        let body = get_body(response).await;
        assert!(
            body.contains("at least one valid status"),
            "expected error about empty job_status, got: {body}"
        );
    }
}

mod webui;

use axum::Json;
use axum::extract::FromRef;
use axum::extract::Path;
use axum::extract::Query;
use axum::extract::State;
use axum::http::StatusCode;
use axum::response::IntoResponse;
use axum::response::Sse;
use axum::response::sse::Event;
use axum::response::sse::KeepAlive;
use axum::routing::get;
use clp_rust_utils::logs_input::LogsInputError;
use futures::Stream;
use futures::StreamExt;
use serde::Deserialize;
use serde::Serialize;
use thiserror::Error;
use tower_http::cors::Any;
use tower_http::cors::CorsLayer;
use utoipa::IntoParams;
use utoipa::OpenApi;
use utoipa::ToSchema;
use utoipa_axum::router::OpenApiRouter;
use utoipa_axum::routes;

use crate::client::Client;
use crate::client::ClientError;
use crate::client::CompressionUsage;
use crate::client::CompressionUsageParams;
use crate::client::QueryConfig;
use crate::client::ValidatedCompressionUsageParams;
use crate::webui_client::WebuiClient;

/// Shared application state passed to all route handlers.
#[derive(Clone)]
pub struct AppState {
    /// Client for the public API routes.
    pub client: Client,
    /// Client for the `WebUI`'s routes.
    pub webui_client: WebuiClient,
}

impl FromRef<AppState> for Client {
    fn from_ref(state: &AppState) -> Self {
        state.client.clone()
    }
}

impl FromRef<AppState> for WebuiClient {
    fn from_ref(state: &AppState) -> Self {
        state.webui_client.clone()
    }
}

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
/// * Forwards [`utoipa::openapi::OpenApi::to_json`]'s return values on failure.
pub fn from_app_state(state: AppState) -> Result<axum::Router, serde_json::Error> {
    let (public_routes, api) = OpenApiRouter::with_openapi(ApiDoc::openapi())
        .route("/", get(health))
        .routes(routes!(health))
        .routes(routes!(query))
        .routes(routes!(query_results))
        .routes(routes!(cancel_query))
        .routes(routes!(compression_usage))
        .split_for_parts();
    let api_json = api.to_json()?;
    let public_routes = public_routes
        .route(
            "/openapi.json",
            get(|| async { (StatusCode::OK, api_json) }),
        )
        .layer(CorsLayer::new().allow_origin(Any));
    let (webui_routes, _) = webui::router().split_for_parts();
    Ok(public_routes.merge(webui_routes).with_state(state))
}

// `utoipa::OpenApi` triggers `clippy::needless_for_each`
#[allow(clippy::needless_for_each)]
mod api_doc {
    // Using `super::...` can cause `super` to appear as a tag in the generated OpenAPI
    // documentation. Importing the paths directly prevents this issue.
    use super::__path_cancel_query;
    use super::__path_compression_usage;
    use super::__path_health;
    use super::__path_query;
    use super::__path_query_results;
    use super::CompressionUsage;
    use super::OpenApi;
    use super::OpenApiRouter;
    use super::webui;
    use crate::client::CompressionJobStatus;

    #[derive(utoipa::OpenApi)]
    #[openapi(
        info(
            title = "API Server",
            description = "API Server for CLP",
            contact(name = "YScope")
        ),
        paths(health, query, query_results, cancel_query, compression_usage),
        components(schemas(CompressionUsage, CompressionJobStatus))
    )]
    pub struct ApiDoc;

    /// The `WebUI`'s `OpenAPI` document: [`ApiDoc`] extended with the routes that back the `WebUI`.
    pub struct WebUiApiDoc;

    impl OpenApi for WebUiApiDoc {
        fn openapi() -> utoipa::openapi::OpenApi {
            let (_, api) = OpenApiRouter::with_openapi(ApiDoc::openapi())
                .merge(webui::router())
                .split_for_parts();
            api
        }
    }
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
            "buffer_results_in_mongodb": true,
            "count_by_time_bucket_size_millisecs": null
        })),
    responses(
        (
            status = OK,
            body = QueryResultsUri,
            description = "The URI to fetch the results of the submitted query.",
            example = json!({"query_results_uri":"query_results/1"})
        ),
        (status = BAD_REQUEST, description = "Invalid query config (e.g., empty query_string, \
            time_range_begin_millisecs > time_range_end_millisecs, \
            count_by_time_bucket_size_millisecs <= 0)"),
        (status = INTERNAL_SERVER_ERROR)
    )
)]
async fn query(
    State(client): State<Client>,
    Json(query_config): Json<QueryConfig>,
) -> Result<Json<QueryResultsUri>, HandlerError> {
    tracing::info!("Submitting query: {:?}", query_config);
    let query_job_id = match client.submit_query(query_config).await {
        Ok(id) => {
            tracing::info!("Submitted query job ID: {}", id);
            id
        }
        Err(err) => {
            tracing::error!("Failed to submit query: {:?}", err);
            return Err(err.into());
        }
    };
    let uri = format!("query_results/{query_job_id}");
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

/// Query parameters for the query results endpoint.
#[derive(Deserialize, IntoParams)]
#[into_params(parameter_in = Query)]
struct QueryResultsParams {
    /// When `true`, each SSE event contains the raw result document from the results cache
    /// serialized as JSON (including metadata such as the timestamp and the original file path);
    /// otherwise, each event contains only the log message. Only applies to query jobs whose
    /// results are buffered in `MongoDB`.
    #[serde(default)]
    raw_docs: bool,

    /// When `true`, results buffered in `MongoDB` are streamed sorted by timestamp descending;
    /// otherwise, they're streamed in insertion order. Only applies to query jobs whose results
    /// are buffered in `MongoDB`.
    #[serde(default)]
    sorted: bool,
}

#[utoipa::path(
    get,
    path = "/query_results/{query_job_id}",
    description = "Streams the results of a previously submitted query as Server-Sent Events \
        (SSE).",
    params(QueryResultsParams),
    responses(
        (
            status = OK,
            body = String,
            content_type = "text/event-stream",
            description = "Server-Sent Events stream of query results. Each event contains a \
                single line of the query result in JSON format.",
            example = r#"data: {"timestamp": 1633036800, "message": "Example log message"}\n"#
        ),
        (
            status = NOT_FOUND,
            description = "No query job with the given ID was found."
        ),
        (
            status = CONFLICT,
            description = "The query job streamed its results to a network listener, so they \
                aren't stored."
        ),
        (status = INTERNAL_SERVER_ERROR)
    )
)]
async fn query_results(
    State(client): State<Client>,
    Path(query_job_id): Path<u64>,
    Query(params): Query<QueryResultsParams>,
) -> Result<Sse<impl Stream<Item = Result<Event, HandlerError>>>, HandlerError> {
    tracing::info!("Fetching results for query job ID: {}", query_job_id);
    let results_stream = match client
        .fetch_results(query_job_id, params.raw_docs, params.sorted)
        .await
    {
        Ok(stream) => {
            tracing::info!(
                "Successfully initiated result stream for query job ID {}",
                query_job_id
            );
            stream
        }
        Err(err) => {
            tracing::error!(
                "Failed to fetch results for query job ID {}: {:?}",
                query_job_id,
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
    path = "/query/{query_job_id}",
    description = "Cancels a previously submitted query job.",
    responses(
        (
            status = OK,
            description = "The cancellation request was submitted successfully."
        ),
        (
            status = NOT_FOUND,
            description = "No query job with the given ID was found."
        ),
        (
            status = CONFLICT,
            description = "The query job with the given ID has terminated without being cancelled."
        ),
        (status = INTERNAL_SERVER_ERROR)
    )
)]
async fn cancel_query(
    State(client): State<Client>,
    Path(query_job_id): Path<u64>,
) -> Result<StatusCode, HandlerError> {
    tracing::info!("Cancelling query job ID: {}", query_job_id);
    match client.cancel_query_job(query_job_id).await {
        Ok(()) => {
            tracing::info!(
                "Successfully submitted cancellation request for query job ID: {}",
                query_job_id
            );
            Ok(StatusCode::OK)
        }
        Err(err) => {
            tracing::error!("Failed to cancel query job ID {}: {:?}", query_job_id, err);
            Err(err.into())
        }
    }
}

#[utoipa::path(
    get,
    path = "/usage/compression",
    description = "Gets resource usage statistics for compression jobs \
        within the given time range.",
    params(CompressionUsageParams),
    responses(
        (status = OK, body = Vec<CompressionUsage>),
        (status = BAD_REQUEST, description = "Invalid query parameters \
            (e.g., time_range_begin_millisecs > time_range_end_millisecs, \
            missing required fields)"),
        (status = INTERNAL_SERVER_ERROR)
    )
)]
async fn compression_usage(
    State(client): State<Client>,
    Query(params): Query<CompressionUsageParams>,
) -> Result<Json<Vec<CompressionUsage>>, HandlerError> {
    let validated = ValidatedCompressionUsageParams::try_from(params)?;
    tracing::info!(
        "Fetching compression usage: begin={}, end={}, job_statuses={:?}",
        validated.time_range_begin.timestamp_millis(),
        validated.time_range_end.timestamp_millis(),
        validated.job_statuses,
    );
    Ok(Json(
        client
            .get_compression_usage(&validated)
            .await
            .inspect_err(|err| {
                tracing::error!("Failed to fetch compression usage: {:?}", err);
            })?,
    ))
}

/// Generic errors for request handlers.
#[derive(Error, Debug)]
enum HandlerError {
    #[error("Internal server error")]
    InternalServer,
    #[error("Not found")]
    NotFound,
    #[error("Conflict")]
    Conflict,
    #[error("Bad request: {0}")]
    BadRequest(String),
    #[error("Gateway timeout: {0}")]
    GatewayTimeout(String),
}

impl From<axum::Error> for HandlerError {
    fn from(_: axum::Error) -> Self {
        Self::InternalServer
    }
}

impl From<ClientError> for HandlerError {
    fn from(err: ClientError) -> Self {
        match err {
            ClientError::QueryJobNotFound(_)
            | ClientError::DatasetNotFound(_)
            | ClientError::LogEventNotFound { .. }
            | ClientError::LogsInput(LogsInputError::NotFound(_)) => Self::NotFound,
            ClientError::QueryJobAlreadyTerminated(_) | ClientError::QueryResultsNotStored(_) => {
                Self::Conflict
            }
            ClientError::InvalidInput(_)
            | ClientError::LogsInput(LogsInputError::OutsideMountPoint(_)) => {
                Self::BadRequest(format!("{err}"))
            }
            ClientError::StreamExtractionTimeout(_) => Self::GatewayTimeout(format!("{err}")),
            ClientError::Sql(_)
            | ClientError::Mongo(_)
            | ClientError::QueryNotSucceeded
            | ClientError::Io(_)
            | ClientError::MalformedData
            | ClientError::TaskJoin(_)
            | ClientError::Aws { .. }
            | ClientError::ExtractionJobFailed { .. }
            | ClientError::ExtractionJobNotFound(_)
            | ClientError::LogsInput(
                LogsInputError::MountPointUnavailable(_)
                | LogsInputError::RelativeContainerPath(_)
                | LogsInputError::UnsupportedName(_)
                | LogsInputError::Unresolvable { .. },
            )
            | ClientError::CompressionJobIdOutOfRange(_)
            | ClientError::Telemetry(_) => Self::InternalServer,
        }
    }
}

/// Converts [`HandlerError`] into an HTTP response.
impl IntoResponse for HandlerError {
    fn into_response(self) -> axum::response::Response {
        match self {
            Self::NotFound => StatusCode::NOT_FOUND.into_response(),
            Self::Conflict => StatusCode::CONFLICT.into_response(),
            Self::InternalServer => StatusCode::INTERNAL_SERVER_ERROR.into_response(),
            Self::BadRequest(msg) => (StatusCode::BAD_REQUEST, msg).into_response(),
            Self::GatewayTimeout(msg) => (StatusCode::GATEWAY_TIMEOUT, msg).into_response(),
        }
    }
}

#[cfg(test)]
mod tests {
    use std::path::PathBuf;

    use axum::body::Body;
    use axum::http::HeaderValue;
    use axum::http::Method;
    use axum::http::Request;
    use axum::http::StatusCode;
    use axum::http::header::ACCESS_CONTROL_ALLOW_ORIGIN;
    use axum::http::header::CONTENT_TYPE;
    use axum::http::header::ORIGIN;
    use axum::routing::get;
    use clp_rust_utils::clp_config::package::config::Config;
    use clp_rust_utils::clp_config::package::config::Package;
    use clp_rust_utils::clp_config::package::config::StorageEngine;
    use clp_rust_utils::dataset::DatasetName;
    use clp_rust_utils::job_config::QueryJobStatus;
    use http_body_util::BodyExt;
    use tower::ServiceExt;

    use super::*;

    /// Builds a minimal Axum app that validates compression usage params via
    /// [`TryFrom<CompressionUsageParams>`] and returns the resolved
    /// status integer codes on success. No real database is needed.
    fn compression_usage_test_app() -> axum::Router {
        axum::Router::new().route(
            "/usage/compression",
            get(|Query(params): Query<CompressionUsageParams>| async move {
                let validated = ValidatedCompressionUsageParams::try_from(params)?;
                let codes: Vec<i32> = validated.job_statuses.into_iter().map(i32::from).collect();
                Ok::<_, HandlerError>(axum::Json(codes))
            }),
        )
    }

    /// Builds the application router for the CLP-S storage engine, with clients whose `MySQL` and
    /// `MongoDB` connections are only established on first use.
    async fn clp_s_app_with_lazy_clients() -> axum::Router {
        let config = Config {
            package: Package {
                storage_engine: StorageEngine::ClpS,
            },
            ..Config::default()
        };
        let mongodb_client = mongodb::Client::with_uri_str("mongodb://localhost:27017")
            .await
            .expect("the MongoDB URI should be valid");
        let sql_pool = sqlx::mysql::MySqlPoolOptions::new()
            .connect_lazy("mysql://localhost/clp-db")
            .expect("the MySQL URI should be valid");
        from_app_state(AppState {
            client: Client::new(config.clone(), mongodb_client.clone(), sql_pool.clone()),
            webui_client: WebuiClient::new(config, sql_pool, mongodb_client).await,
        })
        .expect("the OpenAPI document should serialize")
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
    async fn cors_allows_any_origin_on_public_routes_but_not_on_webui_routes() -> anyhow::Result<()>
    {
        let app = clp_s_app_with_lazy_clients().await;

        let public_response = app
            .clone()
            .oneshot(
                Request::builder()
                    .uri("/health")
                    .header(ORIGIN, "https://unrelated.example")
                    .body(Body::empty())?,
            )
            .await?;
        assert_eq!(public_response.status(), StatusCode::OK);
        assert_eq!(
            public_response.headers().get(ACCESS_CONTROL_ALLOW_ORIGIN),
            Some(&HeaderValue::from_static("*"))
        );

        let webui_response = app
            .oneshot(
                Request::builder()
                    .uri("/metadata/time_range")
                    .header(ORIGIN, "https://unrelated.example")
                    .body(Body::empty())?,
            )
            .await?;
        assert_eq!(webui_response.status(), StatusCode::OK);
        assert_eq!(
            webui_response.headers().get(ACCESS_CONTROL_ALLOW_ORIGIN),
            None
        );
        Ok(())
    }

    #[tokio::test]
    async fn timestamp_column_names_route_rejects_invalid_dataset_name() -> anyhow::Result<()> {
        let app = clp_s_app_with_lazy_clients().await;
        let response = app
            .oneshot(
                Request::builder()
                    .uri("/metadata/column_metadata/bad-name/timestamp")
                    .body(Body::empty())?,
            )
            .await?;
        assert_eq!(response.status(), StatusCode::BAD_REQUEST);
        let body = get_body(response).await;
        let parse_error = "bad-name"
            .parse::<DatasetName>()
            .expect_err("name should be an invalid dataset name");
        assert!(
            body.contains(&parse_error.to_string()),
            "unexpected body: {body}"
        );
        Ok(())
    }

    #[test]
    fn client_errors_map_to_their_statuses() -> anyhow::Result<()> {
        for (error, expected_status) in [
            (ClientError::QueryJobNotFound(1), StatusCode::NOT_FOUND),
            (
                ClientError::QueryJobAlreadyTerminated(1),
                StatusCode::CONFLICT,
            ),
            (ClientError::QueryResultsNotStored(1), StatusCode::CONFLICT),
            (
                ClientError::DatasetNotFound("logs".parse()?),
                StatusCode::NOT_FOUND,
            ),
            (
                ClientError::LogEventNotFound {
                    archive_id: "3f1c1a5e-5b6e-4a43-9d6b-0f2b2f5a8c11".parse()?,
                    log_event_idx: 42,
                },
                StatusCode::NOT_FOUND,
            ),
            (
                ClientError::MalformedData,
                StatusCode::INTERNAL_SERVER_ERROR,
            ),
            (
                ClientError::ExtractionJobFailed {
                    job_id: 1,
                    status: QueryJobStatus::Failed,
                },
                StatusCode::INTERNAL_SERVER_ERROR,
            ),
            (
                ClientError::ExtractionJobNotFound(1),
                StatusCode::INTERNAL_SERVER_ERROR,
            ),
            (
                ClientError::LogsInput(LogsInputError::NotFound(PathBuf::from("/app.log"))),
                StatusCode::NOT_FOUND,
            ),
            (
                ClientError::LogsInput(LogsInputError::Unresolvable {
                    path: PathBuf::from("/loop-link"),
                    source: std::io::Error::from(std::io::ErrorKind::PermissionDenied),
                }),
                StatusCode::INTERNAL_SERVER_ERROR,
            ),
            (
                ClientError::LogsInput(LogsInputError::MountPointUnavailable(
                    std::io::Error::from(std::io::ErrorKind::NotFound),
                )),
                StatusCode::INTERNAL_SERVER_ERROR,
            ),
            (
                ClientError::CompressionJobIdOutOfRange(u64::MAX),
                StatusCode::INTERNAL_SERVER_ERROR,
            ),
        ] {
            let description = format!("{error:?}");
            assert_eq!(
                HandlerError::from(error).into_response().status(),
                expected_status,
                "error: {description}"
            );
        }
        Ok(())
    }

    #[tokio::test]
    async fn stream_file_extraction_route_rejects_an_index_above_i64_max() -> anyhow::Result<()> {
        let app = clp_s_app_with_lazy_clients().await;
        let response = app
            .oneshot(
                Request::builder()
                    .method(Method::POST)
                    .uri("/stream_files/extract")
                    .header(CONTENT_TYPE, "application/json")
                    .body(Body::from(
                        serde_json::json!({
                            "archive_id": "3f1c1a5e-5b6e-4a43-9d6b-0f2b2f5a8c11",
                            "log_event_idx": i64::MAX.unsigned_abs() + 1,
                        })
                        .to_string(),
                    ))?,
            )
            .await?;
        assert_eq!(response.status(), StatusCode::BAD_REQUEST);
        Ok(())
    }

    #[tokio::test]
    async fn logs_input_path_outside_mount_point_maps_to_bad_request_with_its_message() {
        let error = LogsInputError::OutsideMountPoint(PathBuf::from("/outside-link"));
        let expected = error.to_string();
        let response = HandlerError::from(ClientError::LogsInput(error)).into_response();
        assert_eq!(response.status(), StatusCode::BAD_REQUEST);
        assert_eq!(get_body(response).await, expected);
    }

    #[tokio::test]
    async fn stream_extraction_timeout_maps_to_gateway_timeout_with_its_message() {
        let error = ClientError::StreamExtractionTimeout(1800);
        let expected = error.to_string();
        let response = HandlerError::from(error).into_response();
        assert_eq!(response.status(), StatusCode::GATEWAY_TIMEOUT);
        assert_eq!(get_body(response).await, expected);
    }

    #[tokio::test]
    async fn reject_begin_greater_than_end() -> anyhow::Result<()> {
        let app = compression_usage_test_app();
        let response = app
            .oneshot(
                Request::builder()
                    .uri(
                        "/usage/compression?time_range_begin_millisecs=200&\
                         time_range_end_millisecs=100",
                    )
                    .body(Body::empty())?,
            )
            .await?;
        assert_eq!(response.status(), StatusCode::BAD_REQUEST);
        let body = get_body(response).await;
        assert!(body.contains("time_range_begin_millisecs must be <= time_range_end_millisecs"));
        Ok(())
    }

    #[tokio::test]
    async fn reject_unknown_job_status() -> anyhow::Result<()> {
        let app = compression_usage_test_app();
        let response = app
            .oneshot(
                Request::builder()
                    .uri(
                        "/usage/compression?time_range_begin_millisecs=0&\
                         time_range_end_millisecs=100&job_status=UNKNOWN",
                    )
                    .body(Body::empty())?,
            )
            .await?;
        assert_eq!(response.status(), StatusCode::BAD_REQUEST);
        let body = get_body(response).await;
        assert!(body.contains("Unknown job_status: UNKNOWN"));
        Ok(())
    }

    #[tokio::test]
    async fn accept_lowercase_job_status() -> anyhow::Result<()> {
        let app = compression_usage_test_app();
        let response = app
            .oneshot(
                Request::builder()
                    .uri(
                        "/usage/compression?time_range_begin_millisecs=0&\
                         time_range_end_millisecs=100&job_status=succeeded",
                    )
                    .body(Body::empty())?,
            )
            .await?;
        assert_eq!(response.status(), StatusCode::OK);
        let body = get_body(response).await;
        assert_eq!(body, "[2]"); // succeeded → 2
        Ok(())
    }

    #[tokio::test]
    async fn accept_valid_params_with_defaults() -> anyhow::Result<()> {
        let app = compression_usage_test_app();
        let response = app
            .oneshot(
                Request::builder()
                    .uri(
                        "/usage/compression?time_range_begin_millisecs=0&\
                         time_range_end_millisecs=100",
                    )
                    .body(Body::empty())?,
            )
            .await?;
        assert_eq!(response.status(), StatusCode::OK);
        let body = get_body(response).await;
        assert_eq!(body, "[2,3,4]"); // SUCCEEDED, FAILED, KILLED
        Ok(())
    }

    #[tokio::test]
    async fn accept_comma_separated_job_status() -> anyhow::Result<()> {
        let app = compression_usage_test_app();
        let response = app
            .oneshot(
                Request::builder()
                    .uri(
                        "/usage/compression?time_range_begin_millisecs=0&\
                         time_range_end_millisecs=100&job_status=SUCCEEDED,RUNNING",
                    )
                    .body(Body::empty())?,
            )
            .await?;
        assert_eq!(response.status(), StatusCode::OK);
        let body = get_body(response).await;
        assert_eq!(body, "[2,1]"); // SUCCEEDED, RUNNING
        Ok(())
    }

    #[tokio::test]
    async fn accept_spaces_around_commas() -> anyhow::Result<()> {
        let app = compression_usage_test_app();
        let response = app
            .oneshot(
                Request::builder()
                    .uri(
                        "/usage/compression?time_range_begin_millisecs=0&\
                         time_range_end_millisecs=100&job_status=SUCCEEDED%2C+FAILED",
                    )
                    .body(Body::empty())?,
            )
            .await?;
        assert_eq!(response.status(), StatusCode::OK);
        let body = get_body(response).await;
        assert_eq!(body, "[2,3]"); // SUCCEEDED, FAILED
        Ok(())
    }

    #[tokio::test]
    async fn accept_trailing_comma() -> anyhow::Result<()> {
        let app = compression_usage_test_app();
        let response = app
            .oneshot(
                Request::builder()
                    .uri(
                        "/usage/compression?time_range_begin_millisecs=0&\
                         time_range_end_millisecs=100&job_status=SUCCEEDED,",
                    )
                    .body(Body::empty())?,
            )
            .await?;
        assert_eq!(response.status(), StatusCode::OK);
        let body = get_body(response).await;
        assert_eq!(body, "[2]"); // trailing comma ignored
        Ok(())
    }

    #[tokio::test]
    async fn accept_single_job_status() -> anyhow::Result<()> {
        let app = compression_usage_test_app();
        let response = app
            .oneshot(
                Request::builder()
                    .uri(
                        "/usage/compression?time_range_begin_millisecs=0&\
                         time_range_end_millisecs=100&job_status=KILLED",
                    )
                    .body(Body::empty())?,
            )
            .await?;
        assert_eq!(response.status(), StatusCode::OK);
        let body = get_body(response).await;
        assert_eq!(body, "[4]");
        Ok(())
    }

    #[tokio::test]
    async fn reject_missing_time_range_begin_millisecs() -> anyhow::Result<()> {
        let app = compression_usage_test_app();
        let response = app
            .oneshot(
                Request::builder()
                    .uri("/usage/compression?time_range_end_millisecs=100")
                    .body(Body::empty())?,
            )
            .await?;
        assert_eq!(response.status(), StatusCode::BAD_REQUEST);
        let body = get_body(response).await;
        assert!(
            body.contains("time_range_begin_millisecs") || body.contains("deserialize"),
            "expected error about missing time_range_begin_millisecs, got: {body}"
        );
        Ok(())
    }

    #[tokio::test]
    async fn reject_empty_job_status() -> anyhow::Result<()> {
        let app = compression_usage_test_app();
        let response = app
            .oneshot(
                Request::builder()
                    .uri(
                        "/usage/compression?time_range_begin_millisecs=0&\
                         time_range_end_millisecs=100&job_status=",
                    )
                    .body(Body::empty())?,
            )
            .await?;
        assert_eq!(response.status(), StatusCode::BAD_REQUEST);
        let body = get_body(response).await;
        assert!(
            body.contains("at least one valid status"),
            "expected error about empty job_status, got: {body}"
        );
        Ok(())
    }

    #[tokio::test]
    async fn reject_zero_limit() -> anyhow::Result<()> {
        let app = compression_usage_test_app();
        let response = app
            .oneshot(
                Request::builder()
                    .uri(
                        "/usage/compression?time_range_begin_millisecs=0&\
                         time_range_end_millisecs=100&limit=0",
                    )
                    .body(Body::empty())?,
            )
            .await?;
        assert_eq!(response.status(), StatusCode::BAD_REQUEST);
        let body = get_body(response).await;
        assert!(
            body.contains("limit must be > 0"),
            "expected error about limit, got: {body}"
        );
        Ok(())
    }

    #[tokio::test]
    async fn reject_negative_limit() -> anyhow::Result<()> {
        let app = compression_usage_test_app();
        let response = app
            .oneshot(
                Request::builder()
                    .uri(
                        "/usage/compression?time_range_begin_millisecs=0&\
                         time_range_end_millisecs=100&limit=-1",
                    )
                    .body(Body::empty())?,
            )
            .await?;
        assert_eq!(response.status(), StatusCode::BAD_REQUEST);
        let body = get_body(response).await;
        assert!(
            body.contains("limit must be > 0"),
            "expected error about limit, got: {body}"
        );
        Ok(())
    }

    #[tokio::test]
    async fn reject_duplicate_job_status() -> anyhow::Result<()> {
        let app = compression_usage_test_app();
        let response = app
            .oneshot(
                Request::builder()
                    .uri(
                        "/usage/compression?time_range_begin_millisecs=0&\
                         time_range_end_millisecs=100&job_status=SUCCEEDED,SUCCEEDED",
                    )
                    .body(Body::empty())?,
            )
            .await?;
        assert_eq!(response.status(), StatusCode::BAD_REQUEST);
        let body = get_body(response).await;
        assert!(
            body.contains("Duplicate job_status"),
            "expected error about duplicate job_status, got: {body}"
        );
        Ok(())
    }
}

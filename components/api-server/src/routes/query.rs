//! Routes that submit search queries, one for each output of their results.

use axum::Json;
use axum::extract::State;
use clp_rust_utils::job_config::QueryJobConfig;
use clp_rust_utils::job_config::ResultsCacheOutput;
use serde::Deserialize;
use serde::Serialize;
use utoipa::ToSchema;
use utoipa_axum::router::OpenApiRouter;
use utoipa_axum::routes;

use crate::client::Client;
use crate::query_request::FileOutputOptions;
use crate::query_request::NetworkOutputOptions;
use crate::query_request::QueryRequest;
use crate::routes::AppState;
use crate::routes::HandlerError;

/// Factory function.
///
/// # Returns
///
/// A newly created [`OpenApiRouter`] with all query submission routes registered.
pub(super) fn router() -> OpenApiRouter<AppState> {
    OpenApiRouter::new()
        .routes(routes!(submit_query_to_results_cache))
        .routes(routes!(submit_query_to_file))
        .routes(routes!(submit_query_to_stream))
}

#[derive(Clone, Serialize, Deserialize, ToSchema)]
#[serde(deny_unknown_fields)]
struct QueryResultsUri {
    /// The uri to get the query results.
    query_results_uri: String,
}

#[utoipa::path(
    post,
    path = "/query/results_cache",
    description = "Submits a new query job whose results are buffered in the results cache \
        (`MongoDB`).",
    request_body(
        content = QueryRequest<ResultsCacheOutput>,
        example = json!({
            "datasets": ["default"],
            "query": {
                "query_string": "*",
                "time_range": {
                    "begin_timestamp_millisecs": 0,
                    "end_timestamp_millisecs": 17_356_896
                },
                "ignore_case": true
            },
            "output": {"max_num_results": 100}
        })),
    responses(
        (
            status = OK,
            body = QueryResultsUri,
            description = "The URI to fetch the results of the submitted query.",
            example = json!({"query_results_uri":"query_results/1"})
        ),
        (status = BAD_REQUEST, body = String, content_type = "text/plain",
            description = "The request body is not valid JSON or could not be read."),
        (status = PAYLOAD_TOO_LARGE, body = String, content_type = "text/plain",
            description = "The request body exceeds the configured size limit."),
        (status = UNSUPPORTED_MEDIA_TYPE, body = String, content_type = "text/plain",
            description = "The Content-Type header is missing or is not a JSON media type."),
        (status = UNPROCESSABLE_ENTITY, body = String, content_type = "text/plain",
            description = "The JSON body has missing or unknown fields, or fields with invalid \
                types or values (e.g., an empty `datasets` or `query.query_string`, a \
                `query.time_range` whose begin timestamp exceeds its end timestamp, or a \
                non-positive `output.max_num_results`)."),
        (status = INTERNAL_SERVER_ERROR)
    )
)]
async fn submit_query_to_results_cache(
    State(client): State<Client>,
    Json(query_request): Json<QueryRequest<ResultsCacheOutput>>,
) -> Result<Json<QueryResultsUri>, HandlerError> {
    submit_query_with_stored_results(client, query_request.into_query_job_config(())).await
}

#[utoipa::path(
    post,
    path = "/query/file",
    description = "Submits a new query job whose results are buffered in files in the stream \
        output storage (the file system or S3).",
    request_body(
        content = QueryRequest<FileOutputOptions>,
        example = json!({
            "datasets": ["default"],
            "query": {
                "query_string": "*",
                "time_range": {
                    "begin_timestamp_millisecs": 0,
                    "end_timestamp_millisecs": 17_356_896
                },
                "ignore_case": true
            }
        })),
    responses(
        (
            status = OK,
            body = QueryResultsUri,
            description = "The URI to fetch the results of the submitted query.",
            example = json!({"query_results_uri":"query_results/1"})
        ),
        (status = BAD_REQUEST, body = String, content_type = "text/plain",
            description = "The request body is not valid JSON or could not be read."),
        (status = PAYLOAD_TOO_LARGE, body = String, content_type = "text/plain",
            description = "The request body exceeds the configured size limit."),
        (status = UNSUPPORTED_MEDIA_TYPE, body = String, content_type = "text/plain",
            description = "The Content-Type header is missing or is not a JSON media type."),
        (status = UNPROCESSABLE_ENTITY, body = String, content_type = "text/plain",
            description = "The JSON body has missing or unknown fields, or fields with invalid \
                types or values (e.g., an empty `datasets` or `query.query_string`, or a \
                `query.time_range` whose begin timestamp exceeds its end timestamp)."),
        (status = INTERNAL_SERVER_ERROR)
    )
)]
async fn submit_query_to_file(
    State(client): State<Client>,
    Json(query_request): Json<QueryRequest<FileOutputOptions>>,
) -> Result<Json<QueryResultsUri>, HandlerError> {
    submit_query_with_stored_results(client, query_request.into_query_job_config(())).await
}

#[utoipa::path(
    post,
    path = "/query/stream",
    description = "Submits a new query job whose results are streamed back to the client. Not \
        available yet.",
    request_body(
        content = QueryRequest<NetworkOutputOptions>,
        example = json!({
            "datasets": ["default"],
            "query": {
                "query_string": "*",
                "time_range": {
                    "begin_timestamp_millisecs": 0,
                    "end_timestamp_millisecs": 17_356_896
                },
                "ignore_case": true
            }
        })),
    responses(
        (status = BAD_REQUEST, body = String, content_type = "text/plain",
            description = "The request body is not valid JSON or could not be read."),
        (status = PAYLOAD_TOO_LARGE, body = String, content_type = "text/plain",
            description = "The request body exceeds the configured size limit."),
        (status = UNSUPPORTED_MEDIA_TYPE, body = String, content_type = "text/plain",
            description = "The Content-Type header is missing or is not a JSON media type."),
        (status = UNPROCESSABLE_ENTITY, body = String, content_type = "text/plain",
            description = "The JSON body has missing or unknown fields, or fields with invalid \
                types or values (e.g., an empty `datasets` or `query.query_string`, or a \
                `query.time_range` whose begin timestamp exceeds its end timestamp)."),
        (status = NOT_IMPLEMENTED, body = String, content_type = "text/plain",
            description = "Streaming queries are not available yet.")
    )
)]
async fn submit_query_to_stream(
    Json(_query_request): Json<QueryRequest<NetworkOutputOptions>>,
) -> HandlerError {
    HandlerError::NotImplemented("streaming queries are not available yet".to_owned())
}

/// Submits a query job with the given config.
///
/// The config's output must store the results, since the client fetches them later through the
/// returned URI.
///
/// # Returns
///
/// The URI to fetch the results of the submitted query job on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`Client::submit_query`]'s return values on failure.
async fn submit_query_with_stored_results(
    client: Client,
    query_job_config: QueryJobConfig,
) -> Result<Json<QueryResultsUri>, HandlerError> {
    tracing::info!("Submitting query: {:?}", query_job_config);
    let query_job_id = match client.submit_query(&query_job_config).await {
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

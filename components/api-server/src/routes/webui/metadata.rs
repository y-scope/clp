//! Routes that serve read-only metadata about the CLP package's datasets, archives, and jobs.

use std::collections::HashSet;

use axum::Json;
use axum::extract::Path;
use axum::extract::Query;
use axum::extract::State;
use clp_rust_utils::dataset::DatasetName;
use serde::Deserialize;
use utoipa::IntoParams;
use utoipa_axum::router::OpenApiRouter;
use utoipa_axum::routes;

use super::log_unexpected_error;
use crate::error::ClientError;
use crate::routes::AppState;
use crate::routes::HandlerError;
use crate::webui_client::CompressionMetadata;
use crate::webui_client::IngestionDetails;
use crate::webui_client::QuerySpeed;
use crate::webui_client::SpaceSavings;
use crate::webui_client::TimeRange;
use crate::webui_client::WebuiClient;

/// Factory function.
///
/// # Returns
///
/// A newly created [`OpenApiRouter`] with all metadata routes registered.
pub(super) fn router() -> OpenApiRouter<AppState> {
    OpenApiRouter::new()
        .routes(routes!(datasets))
        .routes(routes!(time_range))
        .routes(routes!(space_savings))
        .routes(routes!(ingestion_details))
        .routes(routes!(query_speed))
        .routes(routes!(timestamp_column_names))
        .routes(routes!(compression_metadata))
}

/// Query parameters for the dataset-scoped metadata endpoints.
#[derive(Deserialize, IntoParams)]
#[into_params(parameter_in = Query)]
struct DatasetsParams {
    /// Comma-separated list of dataset names (CLP-S only). Ignored for the CLP storage engine.
    #[serde(default)]
    dataset: Option<String>,
}

/// Query parameters for the query speed endpoint.
#[derive(Deserialize, IntoParams)]
#[into_params(parameter_in = Query)]
struct QuerySpeedParams {
    /// Comma-separated list of dataset names (CLP-S only). Ignored for the CLP storage engine.
    #[serde(default)]
    dataset: Option<String>,
    /// The search job ID whose scan speed should be computed.
    search_job_id: i64,
}

/// Path parameters for the timestamp column names endpoint.
///
/// # NOTE
///
/// * [`DatasetName`] deserializes through `Option<String>`, and axum's top-level path deserializer
///   doesn't support optional values, so extracting `Path<DatasetName>` directly fails for every
///   request. Deserializing the dataset name as a struct field avoids this.
#[derive(Deserialize, IntoParams)]
#[into_params(parameter_in = Path)]
struct TimestampColumnNamesPath {
    #[param(value_type = String)]
    dataset_name: DatasetName,
}

#[utoipa::path(
    get,
    path = "/metadata/datasets",
    description = "Gets the names of all datasets.",
    responses(
        (status = OK, body = Vec<String>),
        (status = INTERNAL_SERVER_ERROR)
    )
)]
async fn datasets(
    State(client): State<WebuiClient>,
) -> Result<Json<Vec<DatasetName>>, HandlerError> {
    let names = client
        .get_dataset_names()
        .await
        .inspect_err(|err| log_unexpected_error("datasets", err))?;
    Ok(Json(names))
}

#[utoipa::path(
    get,
    path = "/metadata/time_range",
    description = "Gets the earliest and latest log entry timestamps across the given \
        datasets. For the CLP storage engine, the `dataset` parameter is ignored. Returns \
        `null` when no archives exist yet.",
    params(DatasetsParams),
    responses(
        (status = OK, body = Option<TimeRange>),
        (status = BAD_REQUEST, description = "Invalid dataset name"),
        (status = INTERNAL_SERVER_ERROR)
    )
)]
async fn time_range(
    State(client): State<WebuiClient>,
    Query(params): Query<DatasetsParams>,
) -> Result<Json<Option<TimeRange>>, HandlerError> {
    let datasets = parse_datasets(params.dataset.as_deref())?;
    let time_range = client
        .get_time_range(&datasets)
        .await
        .inspect_err(|err| log_unexpected_error("time_range", err))?;
    Ok(Json(time_range))
}

#[utoipa::path(
    get,
    path = "/metadata/space_savings",
    description = "Gets aggregated space-savings statistics (total uncompressed and \
        compressed sizes) across all datasets.",
    responses(
        (status = OK, body = SpaceSavings),
        (status = INTERNAL_SERVER_ERROR)
    )
)]
async fn space_savings(
    State(client): State<WebuiClient>,
) -> Result<Json<SpaceSavings>, HandlerError> {
    let space_savings = client
        .get_space_savings()
        .await
        .inspect_err(|err| log_unexpected_error("space_savings", err))?;
    Ok(Json(space_savings))
}

#[utoipa::path(
    get,
    path = "/metadata/ingestion_details",
    description = "Gets ingestion details (timestamp range, file count, message count) \
        across all datasets. Returns `null` when no data has been ingested yet.",
    responses(
        (status = OK, body = Option<IngestionDetails>),
        (status = INTERNAL_SERVER_ERROR)
    )
)]
async fn ingestion_details(
    State(client): State<WebuiClient>,
) -> Result<Json<Option<IngestionDetails>>, HandlerError> {
    let ingestion_details = client
        .get_ingestion_details()
        .await
        .inspect_err(|err| log_unexpected_error("ingestion_details", err))?;
    Ok(Json(ingestion_details))
}

#[utoipa::path(
    get,
    path = "/metadata/query_speed",
    description = "Gets the query speed (total uncompressed bytes scanned and job duration) \
        for a search job across the given datasets. Returns `null` when the job hasn't \
        scanned any archives or hasn't finished yet.",
    params(QuerySpeedParams),
    responses(
        (status = OK, body = Option<QuerySpeed>),
        (status = BAD_REQUEST, description = "Invalid dataset name"),
        (status = INTERNAL_SERVER_ERROR)
    )
)]
async fn query_speed(
    State(client): State<WebuiClient>,
    Query(params): Query<QuerySpeedParams>,
) -> Result<Json<Option<QuerySpeed>>, HandlerError> {
    let datasets = parse_datasets(params.dataset.as_deref())?;
    let query_speed = client
        .get_query_speed(&datasets, params.search_job_id)
        .await
        .inspect_err(|err| log_unexpected_error("query_speed", err))?;
    Ok(Json(query_speed))
}

#[utoipa::path(
    get,
    path = "/metadata/column_metadata/{dataset_name}/timestamp",
    description = "Gets the timestamp column names for a given dataset (CLP-S only).",
    params(TimestampColumnNamesPath),
    responses(
        (status = OK, body = Vec<String>),
        (status = BAD_REQUEST, description = "Invalid dataset name"),
        (status = NOT_FOUND, description = "Dataset not found"),
        (status = INTERNAL_SERVER_ERROR)
    )
)]
async fn timestamp_column_names(
    State(client): State<WebuiClient>,
    Path(TimestampColumnNamesPath { dataset_name }): Path<TimestampColumnNamesPath>,
) -> Result<Json<Vec<String>>, HandlerError> {
    let names = client
        .get_timestamp_column_names(&dataset_name)
        .await
        .inspect_err(|err| log_unexpected_error("timestamp_column_names", err))?;
    Ok(Json(names))
}

#[utoipa::path(
    get,
    path = "/metadata/compression_jobs",
    description = "Gets recent compression-job metadata (most recent first), with the \
        decoded CLP IO config for each job.",
    responses(
        (status = OK, body = Vec<CompressionMetadata>),
        (status = INTERNAL_SERVER_ERROR)
    )
)]
async fn compression_metadata(
    State(client): State<WebuiClient>,
) -> Result<Json<Vec<CompressionMetadata>>, HandlerError> {
    let compression_metadata = client
        .get_compression_metadata()
        .await
        .inspect_err(|err| log_unexpected_error("compression_metadata", err))?;
    Ok(Json(compression_metadata))
}

/// Parses a comma-separated list of dataset names.
///
/// # Returns
///
/// The trimmed, non-empty dataset names in first-seen order, without duplicates, on success. An
/// absent list gives no dataset names.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`ClientError::InvalidDatasetName`] if any name isn't a valid dataset name.
fn parse_datasets(datasets: Option<&str>) -> Result<Vec<DatasetName>, ClientError> {
    let Some(datasets) = datasets else {
        return Ok(Vec::new());
    };
    let mut seen = HashSet::new();
    datasets
        .split(',')
        .map(str::trim)
        .filter(|name| !name.is_empty())
        .filter(|name| seen.insert(*name))
        .map(|name| name.parse().map_err(ClientError::from))
        .collect()
}

#[cfg(test)]
mod tests {
    use clp_rust_utils::dataset::DatasetName;
    use clp_rust_utils::dataset::ParseDatasetNameError;

    use super::parse_datasets;
    use crate::error::ClientError;

    #[test]
    fn dataset_names_are_trimmed_deduplicated_and_kept_in_first_seen_order() -> anyhow::Result<()> {
        let expected: Vec<DatasetName> = vec!["logs".parse()?, "other".parse()?];
        assert_eq!(
            parse_datasets(Some(" logs, other,logs, ,other "))?,
            expected
        );
        assert_eq!(parse_datasets(None)?, Vec::<DatasetName>::new());
        Ok(())
    }

    #[test]
    fn invalid_dataset_names_are_rejected() {
        let result = parse_datasets(Some("logs,invalid-name"));
        assert!(
            matches!(
                &result,
                Err(ClientError::InvalidDatasetName(
                    ParseDatasetNameError::InvalidCharacters(name)
                )) if name == "invalid-name"
            ),
            "unexpected result: {result:?}"
        );
    }
}

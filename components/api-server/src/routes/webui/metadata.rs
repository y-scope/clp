//! Routes that serve read-only metadata about the CLP package's datasets, archives, and jobs.

use std::collections::HashSet;

use axum::Json;
use axum::extract::Path;
use axum::extract::Query;
use axum::extract::State;
use clp_rust_utils::dataset::DatasetName;
use clp_rust_utils::job_config::QueryJobId;
use serde::Deserialize;
use serde::Deserializer;
use serde::de::Error as _;
use utoipa::IntoParams;
use utoipa_axum::router::OpenApiRouter;
use utoipa_axum::routes;

use crate::routes::AppState;
use crate::routes::HandlerError;
use crate::routes::webui::log_unexpected_error;
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

/// Query parameters for the time range endpoint.
#[derive(Deserialize, IntoParams)]
#[into_params(parameter_in = Query)]
#[serde(deny_unknown_fields)]
struct TimeRangeParams {
    /// Comma-separated list of dataset names. For the CLP storage engine, the names are validated
    /// but otherwise ignored.
    #[serde(default, deserialize_with = "deserialize_datasets")]
    #[param(value_type = Option<String>)]
    datasets: Vec<DatasetName>,
}

/// Query parameters for the query speed endpoint.
#[derive(Deserialize, IntoParams)]
#[into_params(parameter_in = Query)]
#[serde(deny_unknown_fields)]
struct QuerySpeedParams {
    /// Comma-separated list of dataset names. For the CLP storage engine, the names are validated
    /// but otherwise ignored.
    #[serde(default, deserialize_with = "deserialize_datasets")]
    #[param(value_type = Option<String>)]
    datasets: Vec<DatasetName>,
    /// The query job ID whose scan speed should be computed.
    #[param(value_type = u64)]
    query_job_id: QueryJobId,
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
    /// The name of the dataset whose timestamp column names to get.
    #[param(value_type = String)]
    dataset_name: DatasetName,
}

#[utoipa::path(
    get,
    path = "/metadata/datasets",
    description = "Gets the names of all datasets. Returns an empty list for the CLP storage \
        engine, which has no datasets.",
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
    description = "Gets the earliest and latest log entry timestamps across the given datasets. \
        For the CLP storage engine, the dataset names are validated but otherwise ignored. Returns \
        `null` when the datasets have no archives, including when no datasets are given for CLP-S. \
        Returns 404 when any of the datasets doesn't exist for CLP-S.",
    params(TimeRangeParams),
    responses(
        (
            status = OK,
            body = Option<TimeRange>,
            description = "The time range, or `null` when the datasets have no archives, including \
                when no datasets are given for CLP-S."
        ),
        (status = BAD_REQUEST, description = "Invalid dataset name, or unknown query parameter"),
        (status = NOT_FOUND, description = "Dataset not found (CLP-S only)"),
        (status = INTERNAL_SERVER_ERROR)
    )
)]
async fn time_range(
    State(client): State<WebuiClient>,
    Query(params): Query<TimeRangeParams>,
) -> Result<Json<Option<TimeRange>>, HandlerError> {
    let time_range = client
        .get_time_range(&params.datasets)
        .await
        .inspect_err(|err| log_unexpected_error("time_range", err))?;
    Ok(Json(time_range))
}

#[utoipa::path(
    get,
    path = "/metadata/space_savings",
    description = "Gets aggregated space-savings statistics (total uncompressed and compressed \
        sizes) across all datasets.",
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
    description = "Gets ingestion details (timestamp range, file count, message count) across all \
        datasets. Returns `null` when no data has been ingested yet.",
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
    description = "Gets the query speed (total uncompressed bytes scanned and job duration) for a \
        query job across the given datasets. For the CLP storage engine, the dataset names are \
        validated but otherwise ignored. Returns `null` when the job doesn't exist, the job has no \
        tasks over the datasets' archives, the job hasn't finished yet, or no datasets are given \
        for CLP-S. Returns 404 when any of the datasets doesn't exist for CLP-S.",
    params(QuerySpeedParams),
    responses(
        (
            status = OK,
            body = Option<QuerySpeed>,
            description = "The query speed, or `null` when the job doesn't exist, has no tasks \
                over the datasets' archives, or hasn't finished yet, or when no datasets are given \
                for CLP-S."
        ),
        (
            status = BAD_REQUEST,
            description = "Invalid dataset name, missing or malformed `query_job_id`, or unknown \
                query parameter"
        ),
        (status = NOT_FOUND, description = "Dataset not found (CLP-S only)"),
        (status = INTERNAL_SERVER_ERROR)
    )
)]
async fn query_speed(
    State(client): State<WebuiClient>,
    Query(params): Query<QuerySpeedParams>,
) -> Result<Json<Option<QuerySpeed>>, HandlerError> {
    let query_speed = client
        .get_query_speed(&params.datasets, params.query_job_id)
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
    description = "Gets the metadata of the 1000 most recent compression jobs (most recent first), \
        with the display fields of each job's config. Display fields stored as `null` are omitted. \
        Returns 500 if any of the jobs' stored configs can't be decoded, including when the \
        config, one of its sections, or one of its display fields has an unexpected type.",
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

/// Deserializes a comma-separated list of dataset names.
///
/// # Type Parameters
///
/// * `DeserializerType` - The type of the deserializer to read the list from.
///
/// # Returns
///
/// The trimmed, non-empty dataset names in first-seen order, without duplicates, on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`serde::de::Error::custom`] if any name isn't a valid dataset name.
/// * Forwards [`String::deserialize`]'s return values on failure.
fn deserialize_datasets<'de, DeserializerType: Deserializer<'de>>(
    deserializer: DeserializerType,
) -> Result<Vec<DatasetName>, DeserializerType::Error> {
    let datasets = String::deserialize(deserializer)?;
    let mut seen = HashSet::new();
    datasets
        .split(',')
        .map(str::trim)
        .filter(|name| !name.is_empty())
        .filter(|name| seen.insert(*name))
        .map(|name| name.parse().map_err(DeserializerType::Error::custom))
        .collect()
}

#[cfg(test)]
mod tests {
    use axum::extract::Query;
    use axum::extract::rejection::QueryRejection;
    use axum::http::Uri;
    use clp_rust_utils::dataset::DatasetName;
    use serde::de::IntoDeserializer;
    use serde::de::value::StrDeserializer;

    use super::TimeRangeParams;
    use super::deserialize_datasets;

    fn extract_datasets(uri: &'static str) -> Result<Vec<DatasetName>, QueryRejection> {
        let Query(params) = Query::<TimeRangeParams>::try_from_uri(&Uri::from_static(uri))?;
        Ok(params.datasets)
    }

    #[test]
    fn empty_or_comma_only_datasets_give_no_dataset_names() -> anyhow::Result<()> {
        for uri in [
            "/metadata/time_range?datasets=",
            "/metadata/time_range?datasets=,%20,,",
        ] {
            assert_eq!(
                extract_datasets(uri)?,
                Vec::<DatasetName>::new(),
                "uri: {uri}"
            );
        }
        Ok(())
    }

    #[test]
    fn dataset_names_are_trimmed_deduplicated_and_kept_in_first_seen_order() -> anyhow::Result<()> {
        let expected: Vec<DatasetName> = vec!["logs".parse()?, "other".parse()?];
        assert_eq!(
            extract_datasets("/metadata/time_range?datasets=%20logs,%20other,logs,%20,other%20")?,
            expected
        );
        Ok(())
    }

    #[test]
    fn invalid_dataset_names_are_rejected() {
        let parse_error = "invalid-name"
            .parse::<DatasetName>()
            .expect_err("name should be an invalid dataset name");
        let deserializer: StrDeserializer<'_, serde::de::value::Error> =
            "logs,invalid-name".into_deserializer();
        let error = deserialize_datasets(deserializer)
            .expect_err("an invalid dataset name should be rejected");
        assert_eq!(error.to_string(), parse_error.to_string());
    }
}

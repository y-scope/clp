//! Selects the archives and execution policies for query tasks.

use std::cmp::Reverse;
use std::collections::HashSet;
use std::num::NonZeroU32;
use std::num::NonZeroUsize;

use clp_rust_utils::clp_config::package::config::Database;
use clp_rust_utils::dataset::CLP_DEFAULT_DATASET_NAME;
use clp_rust_utils::job_config::SearchJobConfig;
use clp_rust_utils::types::ArchiveId;
use non_empty_string::NonEmptyString;
use spider_core::task::ExecutionPolicy;
use sqlx::MySqlPool;

use crate::Error;
use crate::query_job_submitter::ArchiveMetadata;

/// Options for selecting archives and setting their query-task execution policy.
pub struct ArchiveSelectionOptions {
    /// Archive retention in minutes, if configured.
    pub archive_retention_period: Option<NonZeroU32>,
    /// Maximum number of distinct datasets in an explicit query dataset list.
    pub max_datasets_per_query: Option<NonZeroUsize>,
    /// Execution policy copied to every selected archive's task.
    pub query_task_execution_policy: ExecutionPolicy,
}

/// Selects archives for a query job, ordered by descending archive end timestamp. The search is
/// bounded by the global archive retention period relative to the query job's creation timestamp.
///
/// # Returns
///
/// The selected archives and their query-task execution policies on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`resolve_datasets`]'s return values on failure.
/// * Forwards [`fetch_archives`]'s return values on failure.
pub async fn prepare_search_task_inputs(
    db_pool: &MySqlPool,
    db_config: &Database,
    search_job_config: &SearchJobConfig,
    archive_selection_options: &ArchiveSelectionOptions,
    job_creation_timestamp_millisecs: i64,
) -> Result<Vec<(ArchiveMetadata, ExecutionPolicy)>, Error> {
    let datasets = resolve_datasets(
        db_pool,
        db_config,
        search_job_config,
        archive_selection_options.max_datasets_per_query,
    )
    .await?;
    if datasets.is_empty() {
        return Ok(Vec::new());
    }
    let archive_end_timestamp_lower_bound = archive_selection_options
        .archive_retention_period
        .map(|period| retention_cutoff_millisecs(period, job_creation_timestamp_millisecs));

    let mut selected_archives = Vec::new();
    for dataset in &datasets {
        selected_archives.extend(
            fetch_archives(
                db_pool,
                db_config,
                search_job_config,
                dataset,
                archive_end_timestamp_lower_bound,
            )
            .await?,
        );
    }
    selected_archives.sort_by_key(|archive| Reverse(archive.end_timestamp));

    Ok(selected_archives
        .into_iter()
        .map(|archive| {
            (
                archive,
                archive_selection_options
                    .query_task_execution_policy
                    .clone(),
            )
        })
        .collect())
}

/// Resolves the datasets selected by a query job.
///
/// # Returns
///
/// The requested datasets on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`deduplicate_requested_datasets`]'s return values on failure.
/// * Forwards [`sqlx::query::QueryScalar::fetch_all`]'s return values on failure.
/// * Forwards [`validate_existing_datasets`]'s return values on failure.
async fn resolve_datasets(
    db_pool: &MySqlPool,
    db_config: &Database,
    search_job_config: &SearchJobConfig,
    max_datasets_per_query: Option<NonZeroUsize>,
) -> Result<Vec<NonEmptyString>, Error> {
    let Some(requested_datasets) = &search_job_config.datasets else {
        return Ok(Vec::new());
    };
    let requested_datasets =
        deduplicate_requested_datasets(requested_datasets, max_datasets_per_query)?;

    let datasets_table = db_config.datasets_table_name();
    let existing_datasets: HashSet<String> =
        sqlx::query_scalar(&format!("SELECT `name` FROM `{datasets_table}`"))
            .fetch_all(db_pool)
            .await?
            .into_iter()
            .collect();
    validate_existing_datasets(&requested_datasets, &existing_datasets)?;
    Ok(requested_datasets)
}

/// Selects archives from one dataset that overlap the query's time range and retention window.
///
/// # Returns
///
/// The selected archives on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`sqlx::query::QueryAs::fetch_all`]'s return values on failure.
async fn fetch_archives(
    db_pool: &MySqlPool,
    db_config: &Database,
    search_job_config: &SearchJobConfig,
    dataset: &NonEmptyString,
    archive_end_timestamp_lower_bound: Option<i64>,
) -> Result<Vec<ArchiveMetadata>, Error> {
    let archives_table = db_config.archives_table_name(Some(dataset.as_str()));
    let mut query_builder = sqlx::QueryBuilder::<sqlx::MySql>::new(format!(
        "SELECT `id`, `size`, `end_timestamp` FROM `{archives_table}` WHERE TRUE"
    ));
    if let Some(end_timestamp) = search_job_config.end_timestamp {
        query_builder
            .push(" AND `begin_timestamp` <= ")
            .push_bind(end_timestamp);
    }
    if let Some(begin_timestamp) = search_job_config.begin_timestamp {
        query_builder
            .push(" AND `end_timestamp` >= ")
            .push_bind(begin_timestamp);
    }
    if let Some(lower_bound) = archive_end_timestamp_lower_bound {
        query_builder
            .push(" AND (`end_timestamp` >= ")
            .push_bind(lower_bound)
            .push(" OR `end_timestamp` = 0)");
    }

    Ok(query_builder
        .build_query_as::<ArchiveRowProjection>()
        .fetch_all(db_pool)
        .await?
        .into_iter()
        .map(|row| ArchiveMetadata {
            id: row.id,
            dataset: Some(dataset.clone()),
            size: row.size,
            end_timestamp: row.end_timestamp,
        })
        .collect())
}

/// Columns projected from an archives table.
///
/// [`ArchiveMetadata`] can't be decoded directly from a row since the dataset is encoded in the
/// archives table's name rather than stored in a column.
#[derive(sqlx::FromRow)]
struct ArchiveRowProjection {
    id: ArchiveId,
    #[sqlx(try_from = "i64")]
    size: u64,
    end_timestamp: i64,
}

/// Validates and deduplicates an explicit dataset list in requested order.
///
/// # Returns
///
/// The distinct requested datasets on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`Error::InvalidQueryJobConfig`] if the list or a name is empty, or the distinct dataset count
///   exceeds the configured limit.
fn deduplicate_requested_datasets(
    requested_datasets: &[String],
    max_datasets_per_query: Option<NonZeroUsize>,
) -> Result<Vec<NonEmptyString>, Error> {
    if requested_datasets.is_empty() {
        return Err(Error::InvalidQueryJobConfig(
            "the datasets list must not be empty".to_owned(),
        ));
    }

    let mut datasets = Vec::new();
    for dataset in requested_datasets {
        let dataset = NonEmptyString::new(dataset.clone()).map_err(|_| {
            Error::InvalidQueryJobConfig("dataset names must not be empty".to_owned())
        })?;
        if !datasets.contains(&dataset) {
            datasets.push(dataset);
        }
    }
    if let Some(max_datasets_per_query) = max_datasets_per_query
        && datasets.len() > max_datasets_per_query.get()
    {
        return Err(Error::InvalidQueryJobConfig(format!(
            "the number of requested datasets ({}) exceeds `max_datasets_per_query` \
             ({max_datasets_per_query})",
            datasets.len()
        )));
    }

    Ok(datasets)
}

/// Checks that every requested dataset exists in the metadata database.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`Error::InvalidQueryJobConfig`] if a requested dataset is unknown.
fn validate_existing_datasets(
    datasets: &[NonEmptyString],
    existing_datasets: &HashSet<String>,
) -> Result<(), Error> {
    let missing_datasets: Vec<&str> = datasets
        .iter()
        .map(NonEmptyString::as_str)
        .filter(|dataset| {
            *dataset != CLP_DEFAULT_DATASET_NAME && !existing_datasets.contains(*dataset)
        })
        .collect();
    if !missing_datasets.is_empty() {
        return Err(Error::InvalidQueryJobConfig(format!(
            "datasets {missing_datasets:?} don't exist"
        )));
    }

    Ok(())
}

/// Calculates the earliest allowed archive end timestamp from the job's creation time.
///
/// # Returns
///
/// The retention cutoff in Unix epoch milliseconds.
fn retention_cutoff_millisecs(
    archive_retention_period: NonZeroU32,
    job_creation_timestamp_millisecs: i64,
) -> i64 {
    const MILLISECS_PER_MIN: i64 = 60 * 1000;
    job_creation_timestamp_millisecs - i64::from(archive_retention_period.get()) * MILLISECS_PER_MIN
}

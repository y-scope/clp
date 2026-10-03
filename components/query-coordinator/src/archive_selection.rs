//! Selects the archives and execution policies for query tasks.

use std::cmp::Reverse;
use std::collections::HashSet;

use clp_rust_utils::clp_config::package::config::Database;
use clp_rust_utils::job_config::SearchJobConfig;
use clp_rust_utils::types::ArchiveId;
use non_empty_string::NonEmptyString;
use spider_core::task::ExecutionPolicy;
use sqlx::MySqlPool;

use crate::Error;
use crate::query_job_submitter::ArchiveMetadata;

/// Selects archives for a query job, ordered by descending archive end timestamp. If
/// `archive_end_ts_lower_bound_millisecs` is set, archives that end before it are excluded.
///
/// # Returns
///
/// The selected archives, each paired with `query_task_execution_policy`, on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`validate_datasets_exist`]'s return values on failure.
/// * Forwards [`fetch_archives`]'s return values on failure.
pub(crate) async fn prepare_search_task_inputs(
    db_pool: &MySqlPool,
    db_config: &Database,
    search_job_config: &SearchJobConfig,
    datasets: &HashSet<NonEmptyString>,
    archive_end_ts_lower_bound_millisecs: Option<i64>,
    query_task_execution_policy: &ExecutionPolicy,
) -> Result<Vec<(ArchiveMetadata, ExecutionPolicy)>, Error> {
    validate_datasets_exist(db_pool, db_config, datasets).await?;

    let mut selected_archives = Vec::new();
    for dataset in datasets {
        selected_archives.extend(
            fetch_archives(
                db_pool,
                db_config,
                search_job_config,
                dataset,
                archive_end_ts_lower_bound_millisecs,
            )
            .await?,
        );
    }
    selected_archives.sort_by_key(|archive| Reverse(archive.end_timestamp));

    Ok(selected_archives
        .into_iter()
        .map(|archive| (archive, query_task_execution_policy.clone()))
        .collect())
}

/// Checks that every requested dataset exists in the metadata database.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`Error::InvalidQueryJobConfig`] if any requested dataset doesn't exist.
/// * Forwards [`sqlx::query::QueryScalar::fetch_one`]'s return values on failure.
async fn validate_datasets_exist(
    db_pool: &MySqlPool,
    db_config: &Database,
    datasets: &HashSet<NonEmptyString>,
) -> Result<(), Error> {
    let datasets_table = db_config.datasets_table_name();
    let mut query_builder = sqlx::QueryBuilder::<sqlx::MySql>::new(format!(
        "SELECT COUNT(*) FROM `{datasets_table}` WHERE `name` IN ("
    ));
    let mut separated_datasets = query_builder.separated(", ");
    for dataset in datasets {
        separated_datasets.push_bind(dataset.as_str());
    }
    query_builder.push(")");

    let num_existing_datasets: i64 = query_builder
        .build_query_scalar()
        .fetch_one(db_pool)
        .await?;
    if usize::try_from(num_existing_datasets).ok() != Some(datasets.len()) {
        return Err(Error::InvalidQueryJobConfig(
            "one or more requested datasets don't exist".to_owned(),
        ));
    }

    Ok(())
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
    archive_end_ts_lower_bound_millisecs: Option<i64>,
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
    if let Some(lower_bound) = archive_end_ts_lower_bound_millisecs {
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

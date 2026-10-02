//! Selects the archives and execution policies for query tasks.

use std::cmp::Reverse;
use std::collections::HashSet;

use clp_rust_utils::clp_config::package::config::Database;
use clp_rust_utils::job_config::SearchJobConfig;
use non_empty_string::NonEmptyString;
use spider_core::task::ExecutionPolicy;
use spider_core::task::TimeoutPolicy;
use sqlx::MySqlPool;

use crate::Error;
use crate::query_job_submitter::ArchiveMetadata;
use crate::query_job_submitter::DatasetArchivesToSearch;

/// Selects the archives to search for a query job.
///
/// If `archive_end_ts_lower_bound_millisecs` is set, archives that end before it are excluded.
///
/// # Returns
///
/// The selected archives grouped by dataset, each paired with the [`ExecutionPolicy`] for the query
/// task that searches it, on success. Each dataset's archives are ordered by descending end
/// timestamp, and datasets are ordered by their newest archive. Datasets without selected archives
/// are omitted.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`ensure_all_queried_datasets_exist`]'s return values on failure.
/// * Forwards [`fetch_archives`]'s return values on failure.
pub(crate) async fn prepare_search_task_inputs(
    db_pool: &MySqlPool,
    db_config: &Database,
    search_job_config: &SearchJobConfig,
    datasets: &HashSet<NonEmptyString>,
    archive_end_ts_lower_bound_millisecs: Option<i64>,
    query_task_max_retry: u32,
) -> Result<Vec<DatasetArchivesToSearch>, Error> {
    ensure_all_queried_datasets_exist(db_pool, db_config, datasets).await?;

    let mut archives_to_search = Vec::new();
    for dataset in datasets {
        let archives = fetch_archives(
            db_pool,
            db_config,
            search_job_config,
            dataset.as_str(),
            archive_end_ts_lower_bound_millisecs,
        )
        .await?;
        if archives.is_empty() {
            continue;
        }
        archives_to_search.push(DatasetArchivesToSearch {
            dataset: dataset.clone(),
            archives: archives
                .into_iter()
                .map(|archive| {
                    let execution_policy =
                        compute_query_task_execution_policy(archive.size, query_task_max_retry);
                    (archive, execution_policy)
                })
                .collect(),
        });
    }
    archives_to_search.sort_by_key(|dataset_archives| {
        Reverse(
            dataset_archives
                .archives
                .first()
                .map(|(archive, _)| archive.end_timestamp),
        )
    });

    Ok(archives_to_search)
}

/// Checks that every requested dataset exists in the metadata database.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`Error::InvalidQueryJobConfig`] if any requested dataset doesn't exist.
/// * Forwards [`sqlx::query::QueryScalar::fetch_one`]'s return values on failure.
async fn ensure_all_queried_datasets_exist(
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
/// The selected archives, ordered by descending end timestamp, on success.
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
    dataset: &str,
    archive_end_ts_lower_bound_millisecs: Option<i64>,
) -> Result<Vec<ArchiveMetadata>, Error> {
    let archives_table = db_config.archives_table_name(Some(dataset));
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
    query_builder.push(" ORDER BY `end_timestamp` DESC");

    Ok(query_builder
        .build_query_as::<ArchiveMetadata>()
        .fetch_all(db_pool)
        .await?)
}

/// Computes the execution policy for a query task that searches an archive of the given size.
///
/// # Returns
///
/// The [`ExecutionPolicy`], with timeouts scaled to `archive_size` and bounded to the range Spider
/// accepts.
fn compute_query_task_execution_policy(archive_size: u64, max_num_retry: u32) -> ExecutionPolicy {
    // NOTE: Keep these bounds in sync with the ones Spider enforces in `TimeoutPolicy::validate`.
    const SPIDER_MIN_TIMEOUT_MS: u64 = 100;
    const SPIDER_MAX_TIMEOUT_MS: u64 = 24 * 60 * 60 * 1000;

    const SOFT_TIMEOUT_MS_PER_MIB: u64 = 10;
    const HARD_TIMEOUT_MS_PER_MIB: u64 = 2000;
    const NUM_BYTES_PER_MIB: u64 = 1024 * 1024;

    let hard_timeout_ms = (archive_size.saturating_mul(HARD_TIMEOUT_MS_PER_MIB)
        / NUM_BYTES_PER_MIB)
        .clamp(SPIDER_MIN_TIMEOUT_MS + 1, SPIDER_MAX_TIMEOUT_MS);
    let soft_timeout_ms = (archive_size.saturating_mul(SOFT_TIMEOUT_MS_PER_MIB)
        / NUM_BYTES_PER_MIB)
        .clamp(SPIDER_MIN_TIMEOUT_MS, hard_timeout_ms - 1);
    ExecutionPolicy {
        max_num_retry,
        timeout_policy: TimeoutPolicy {
            soft_timeout_ms,
            hard_timeout_ms,
        },
        ..ExecutionPolicy::default()
    }
}

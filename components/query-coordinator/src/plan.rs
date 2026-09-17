//! Selects the archives and execution policies for query tasks.

use std::cmp::Reverse;
use std::collections::HashSet;
use std::num::NonZeroU32;
use std::num::NonZeroUsize;

use clp_rust_utils::clp_config::package::config::Database;
use clp_rust_utils::dataset::CLP_DEFAULT_DATASET_NAME;
use clp_rust_utils::job_config::ArchiveId;
use clp_rust_utils::job_config::QUERY_JOBS_TABLE_NAME;
use clp_rust_utils::job_config::QueryJobId;
use clp_rust_utils::job_config::SearchJobConfig;
use const_format::formatcp;
use non_empty_string::NonEmptyString;
use spider_core::task::ExecutionPolicy;
use sqlx::MySqlPool;

use crate::Error;
use crate::query_job_submitter::ArchiveMetadata;

/// Options for selecting archives and setting their query-task execution policy.
pub struct PlanningOption {
    /// Archive retention in minutes, if configured.
    pub archive_retention_period: Option<NonZeroU32>,
    /// Maximum number of distinct datasets in an explicit query dataset list.
    pub max_datasets_per_query: Option<NonZeroUsize>,
    /// Execution policy copied to every selected archive's task.
    pub query_task_execution_policy: ExecutionPolicy,
}

impl PlanningOption {
    /// Selects archives for a query job, ordered by descending archive end timestamp.
    ///
    /// # Errors
    ///
    /// Returns [`Error::InvalidQueryJobConfig`] for invalid timestamps or datasets, and forwards
    /// database errors from dataset validation, retention lookup, and archive selection.
    pub async fn prepare_task_inputs(
        &self,
        db_pool: &MySqlPool,
        db_config: &Database,
        query_job_id: QueryJobId,
        search_job_config: &SearchJobConfig,
    ) -> Result<Vec<(ArchiveMetadata, ExecutionPolicy)>, Error> {
        validate_timestamp_range(search_job_config)?;

        let datasets = self
            .resolve_datasets(db_pool, db_config, search_job_config)
            .await?;
        let archive_end_timestamp_lower_bound = self
            .fetch_archive_end_timestamp_lower_bound(db_pool, query_job_id)
            .await?;

        let mut selected_archives = Vec::new();
        for dataset in &datasets {
            selected_archives.extend(
                Self::fetch_archives(
                    db_pool,
                    db_config,
                    search_job_config,
                    dataset,
                    archive_end_timestamp_lower_bound,
                )
                .await?,
            );
        }
        sort_selected_archives(&mut selected_archives);

        Ok(selected_archives
            .into_iter()
            .map(|archive| (archive.metadata, self.query_task_execution_policy.clone()))
            .collect())
    }

    async fn resolve_datasets(
        &self,
        db_pool: &MySqlPool,
        db_config: &Database,
        search_job_config: &SearchJobConfig,
    ) -> Result<Vec<NonEmptyString>, Error> {
        let datasets = match &search_job_config.datasets {
            Some(requested_datasets) => {
                deduplicate_requested_datasets(requested_datasets, self.max_datasets_per_query)?
            }
            None => vec![
                NonEmptyString::new(CLP_DEFAULT_DATASET_NAME.to_owned())
                    .expect("the default dataset name is nonempty"),
            ],
        };

        let datasets_table = quote_identifier(&db_config.datasets_table_name());
        let existing_datasets: HashSet<String> =
            sqlx::query_scalar(&format!("SELECT `name` FROM {datasets_table}"))
                .fetch_all(db_pool)
                .await?
                .into_iter()
                .collect();
        validate_existing_datasets(&datasets, &existing_datasets)?;
        Ok(datasets)
    }

    async fn fetch_archive_end_timestamp_lower_bound(
        &self,
        db_pool: &MySqlPool,
        query_job_id: QueryJobId,
    ) -> Result<Option<i64>, Error> {
        let Some(archive_retention_period) = self.archive_retention_period else {
            return Ok(None);
        };
        let creation_time_millisecs: i64 = sqlx::query_scalar(formatcp!(
            "SELECT CAST(UNIX_TIMESTAMP(`creation_time`) * 1000 AS SIGNED) FROM \
             `{QUERY_JOBS_TABLE_NAME}` WHERE `id` = ?"
        ))
        .bind(query_job_id)
        .fetch_one(db_pool)
        .await?;
        Ok(Some(retention_cutoff_millisecs(
            creation_time_millisecs,
            archive_retention_period,
        )))
    }

    async fn fetch_archives(
        db_pool: &MySqlPool,
        db_config: &Database,
        search_job_config: &SearchJobConfig,
        dataset: &NonEmptyString,
        archive_end_timestamp_lower_bound: Option<i64>,
    ) -> Result<Vec<SelectedArchive>, Error> {
        let archives_table =
            quote_identifier(&db_config.archives_table_name(Some(dataset.as_str())));
        let mut query_builder = sqlx::QueryBuilder::<sqlx::MySql>::new(format!(
            "SELECT `id`, `size`, `end_timestamp` FROM {archives_table} WHERE TRUE"
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
            .map(|row| SelectedArchive {
                metadata: ArchiveMetadata {
                    id: row.id,
                    dataset: dataset.clone(),
                    size: row.size,
                },
                end_timestamp: row.end_timestamp,
            })
            .collect())
    }
}

/// An archive and the timestamp used to order it among all selected datasets.
struct SelectedArchive {
    metadata: ArchiveMetadata,
    end_timestamp: i64,
}

#[derive(sqlx::FromRow)]
struct ArchiveRowProjection {
    #[sqlx(try_from = "String")]
    id: ArchiveId,
    #[sqlx(try_from = "i64")]
    size: u64,
    end_timestamp: i64,
}

fn validate_timestamp_range(search_job_config: &SearchJobConfig) -> Result<(), Error> {
    if let (Some(begin_timestamp), Some(end_timestamp)) = (
        search_job_config.begin_timestamp,
        search_job_config.end_timestamp,
    ) && begin_timestamp > end_timestamp
    {
        return Err(Error::InvalidQueryJobConfig(format!(
            "begin timestamp {begin_timestamp} is greater than end timestamp {end_timestamp}"
        )));
    }
    Ok(())
}

fn sort_selected_archives(archives: &mut [SelectedArchive]) {
    archives.sort_unstable_by_key(|archive| Reverse(archive.end_timestamp));
}

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

fn validate_existing_datasets(
    datasets: &[NonEmptyString],
    existing_datasets: &HashSet<String>,
) -> Result<(), Error> {
    let missing_datasets: Vec<&str> = datasets
        .iter()
        .map(NonEmptyString::as_str)
        .filter(|dataset| !existing_datasets.contains(*dataset))
        .collect();
    if !missing_datasets.is_empty() {
        return Err(Error::InvalidQueryJobConfig(format!(
            "datasets {missing_datasets:?} don't exist"
        )));
    }

    Ok(())
}

fn retention_cutoff_millisecs(
    creation_time_millisecs: i64,
    archive_retention_period: NonZeroU32,
) -> i64 {
    const MILLISECS_PER_MIN: i64 = 60 * 1000;
    creation_time_millisecs - i64::from(archive_retention_period.get()) * MILLISECS_PER_MIN
}

fn quote_identifier(identifier: &str) -> String {
    format!("`{}`", identifier.replace('`', "``"))
}

//! Archive selection for search queries.

use std::cmp::Reverse;

use clp_rust_utils::clp_config::package::config::Database;
use clp_rust_utils::dataset::DatasetName;
use clp_rust_utils::dataset::NonEmptyDatasetNames;
use clp_rust_utils::job_config::SearchQuery;
use clp_rust_utils::job_config::TimeRange;
use clp_rust_utils::types::ArchiveUuid;
use spider_core::task::ExecutionPolicy;
use sqlx::MySql;
use sqlx::MySqlPool;
use sqlx::QueryBuilder;

use crate::Error;
use crate::job_handle::ArchiveMetadata;
use crate::job_handle::ArchiveSelectionOptions;
use crate::job_handle::archive::NonEmptyDatasetNameSet;
use crate::job_handle::archive::SEARCH_TIMEOUT_RATES;
use crate::job_handle::archive::ensure_datasets_exist;
use crate::job_handle::archive::retention_cutoff_millisecs;
use crate::job_handle::archive::task_execution_policy;

/// Selects the archives a query job should search, and derives each one's task execution policy.
///
/// # Type Parameters
///
/// * `'context_lifetime` - The lifetime of the database resources and selection options.
pub(super) struct ArchiveSelector<'context_lifetime> {
    db_pool: &'context_lifetime MySqlPool,
    db_config: &'context_lifetime Database,
    options: &'context_lifetime ArchiveSelectionOptions,
}

impl<'context_lifetime> ArchiveSelector<'context_lifetime> {
    /// Creates an archive selector with the given metadata database and selection options.
    ///
    /// # Returns
    ///
    /// A newly created [`ArchiveSelector`].
    pub(super) const fn new(
        db_pool: &'context_lifetime MySqlPool,
        db_config: &'context_lifetime Database,
        options: &'context_lifetime ArchiveSelectionOptions,
    ) -> Self {
        Self {
            db_pool,
            db_config,
            options,
        }
    }

    /// Prepares the task inputs for the query job.
    ///
    /// This method validates and deduplicates the job's requested datasets, then retrieves from the
    /// CLP database the metadata of the archives matching the query, ordered by descending archive
    /// end timestamp, and attaches an execution policy derived from each archive's uncompressed
    /// size to its task. The retention filter excludes archives that end before the cutoff, except
    /// archives with an end timestamp of zero.
    ///
    /// # Returns
    ///
    /// A vector of tuples on success, where each tuple contains:
    ///
    /// * The [`ArchiveMetadata`] identifying the archive searched by a single query task.
    /// * The [`ExecutionPolicy`] for that task.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Self::deduplicate_and_validate_requested_datasets`]'s return values on failure.
    /// * Forwards [`ensure_datasets_exist`]'s return values on failure.
    /// * Forwards [`Self::fetch_archives`]'s return values on failure.
    pub(super) async fn select(
        &self,
        query: &SearchQuery,
        job_creation_timestamp_millisecs: i64,
        max_num_retry: u32,
    ) -> Result<Vec<(ArchiveMetadata, ExecutionPolicy)>, Error> {
        let datasets = self.deduplicate_and_validate_requested_datasets(&query.datasets)?;
        ensure_datasets_exist(self.db_pool, self.db_config, &datasets).await?;

        let retention_cutoff_millisecs = self
            .options
            .archive_retention_period_millisecs
            .map(|period| retention_cutoff_millisecs(job_creation_timestamp_millisecs, period));
        let mut selected_archives = Vec::new();
        for dataset in datasets.iter() {
            selected_archives.extend(
                self.fetch_archives(dataset, &query.time_range, retention_cutoff_millisecs)
                    .await?,
            );
        }
        selected_archives.sort_by_key(|archive| Reverse(archive.end_timestamp));

        Ok(selected_archives
            .into_iter()
            .map(|archive| {
                let execution_policy = task_execution_policy(
                    archive.uncompressed_size,
                    &SEARCH_TIMEOUT_RATES,
                    max_num_retry,
                );
                (archive, execution_policy)
            })
            .collect())
    }

    /// Validates and deduplicates the datasets requested by a query job.
    ///
    /// # Type Parameters
    ///
    /// * `'dataset_lifetime` - The lifetime of the requested dataset names.
    ///
    /// # Returns
    ///
    /// The distinct requested datasets on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`Error::InvalidQueryJobConfig`] if the number of distinct datasets exceeds
    ///   [`ArchiveSelectionOptions::max_datasets_per_query`].
    fn deduplicate_and_validate_requested_datasets<'dataset_lifetime>(
        &self,
        requested_datasets: &'dataset_lifetime NonEmptyDatasetNames,
    ) -> Result<NonEmptyDatasetNameSet<'dataset_lifetime>, Error> {
        let datasets = NonEmptyDatasetNameSet::from(requested_datasets);

        if let Some(max_datasets_per_query) = self.options.max_datasets_per_query
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
        &self,
        dataset: &DatasetName,
        time_range: &TimeRange,
        retention_cutoff_millisecs: Option<i64>,
    ) -> Result<Vec<ArchiveMetadata>, Error> {
        let archives_table = self.db_config.archives_table_name(dataset);
        let mut query_builder =
            build_archive_query(&archives_table, time_range, retention_cutoff_millisecs);
        Ok(query_builder
            .build_query_as::<ArchiveRowProjection>()
            .fetch_all(self.db_pool)
            .await?
            .into_iter()
            .map(|row| ArchiveMetadata {
                uuid: row.uuid,
                dataset: dataset.clone(),
                uncompressed_size: row.uncompressed_size,
                end_timestamp: row.end_timestamp,
            })
            .collect())
    }
}

/// Columns projected from an archives table.
///
/// [`ArchiveMetadata`] can't be decoded directly from a row since the dataset is encoded in the
/// archives table's name rather than stored in a column.
#[derive(sqlx::FromRow)]
struct ArchiveRowProjection {
    #[sqlx(rename = "id")]
    uuid: ArchiveUuid,
    #[sqlx(try_from = "i64")]
    uncompressed_size: u64,
    end_timestamp: i64,
}

/// Builds an archive metadata query for the supplied time bounds.
///
/// # Returns
///
/// The query with the overlap and retention filters applied.
fn build_archive_query(
    archives_table: &str,
    time_range: &TimeRange,
    retention_cutoff_millisecs: Option<i64>,
) -> QueryBuilder<'static, MySql> {
    let mut query_builder = QueryBuilder::<MySql>::new(format!(
        "SELECT `id`, `uncompressed_size`, `end_timestamp` FROM `{archives_table}` WHERE TRUE"
    ));
    if let Some(end_timestamp) = time_range.end_timestamp_millisecs() {
        query_builder
            .push(" AND `begin_timestamp` <= ")
            .push_bind(end_timestamp);
    }
    if let Some(begin_timestamp) = time_range.begin_timestamp_millisecs() {
        query_builder
            .push(" AND `end_timestamp` >= ")
            .push_bind(begin_timestamp);
    }
    if let Some(retention_cutoff) = retention_cutoff_millisecs {
        query_builder
            .push(" AND (`end_timestamp` >= ")
            .push_bind(retention_cutoff)
            .push(" OR `end_timestamp` = 0)");
    }

    query_builder
}

#[cfg(test)]
mod tests {
    use std::num::NonZeroUsize;

    use clp_rust_utils::clp_config::package::config::Database;
    use clp_rust_utils::dataset::DatasetName;
    use clp_rust_utils::dataset::NonEmptyDatasetNames;
    use clp_rust_utils::job_config::TimeRange;
    use sqlx::mysql::MySqlPoolOptions;

    use super::ArchiveSelector;
    use super::build_archive_query;
    use crate::Error;
    use crate::job_handle::ArchiveSelectionOptions;

    #[tokio::test]
    async fn duplicate_requested_datasets_count_once_toward_max_datasets_per_query() {
        let db_pool = MySqlPoolOptions::new()
            .connect_lazy("mysql://localhost/clp-db")
            .expect("the database URL should be valid");
        let db_config = Database::default();
        let options = ArchiveSelectionOptions {
            archive_retention_period_millisecs: None,
            max_datasets_per_query: NonZeroUsize::new(1),
        };
        let selector = ArchiveSelector::new(&db_pool, &db_config, &options);
        let default_dataset = DatasetName::default();
        let duplicates =
            NonEmptyDatasetNames::try_from(vec![default_dataset.clone(), default_dataset.clone()])
                .expect("dataset names should be non-empty");
        assert_eq!(
            selector
                .deduplicate_and_validate_requested_datasets(&duplicates)
                .expect("duplicate datasets should count once")
                .iter()
                .collect::<Vec<_>>(),
            [&default_dataset]
        );
        let other_dataset: DatasetName = "other"
            .parse()
            .expect("name should be a valid dataset name");
        let distinct = NonEmptyDatasetNames::try_from(vec![default_dataset, other_dataset])
            .expect("dataset names should be non-empty");
        let error = selector
            .deduplicate_and_validate_requested_datasets(&distinct)
            .expect_err("two distinct datasets should exceed the limit");
        assert!(
            matches!(
                &error,
                Error::InvalidQueryJobConfig(message)
                    if message == "the number of requested datasets (2) exceeds \
                                   `max_datasets_per_query` (1)"
            ),
            "unexpected error: {error:?}"
        );
    }

    #[test]
    fn archive_query_uses_inclusive_retention_cutoff_and_only_exempts_zero_from_retention() {
        let time_range = TimeRange::new(Some(100), Some(300)).expect("time range should be valid");
        let query = build_archive_query("archives", &time_range, Some(200));
        assert_eq!(
            query.sql(),
            "SELECT `id`, `uncompressed_size`, `end_timestamp` FROM `archives` WHERE TRUE AND \
             `begin_timestamp` <= ? AND `end_timestamp` >= ? AND (`end_timestamp` >= ? OR \
             `end_timestamp` = 0)"
        );
    }
}

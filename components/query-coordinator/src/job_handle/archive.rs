//! Shared helpers for archive planning.

use std::collections::HashSet;
use std::num::NonZeroU64;

use clp_rust_utils::clp_config::package::config::Database;
use clp_rust_utils::dataset::DatasetName;
use clp_rust_utils::dataset::NonEmptyDatasetNames;
use spider_core::task::ExecutionPolicy;
use spider_core::task::TimeoutPolicy;
use sqlx::MySqlPool;

use crate::Error;

pub(super) const SEARCH_TIMEOUT_RATES: ArchiveTaskTimeoutRates = ArchiveTaskTimeoutRates {
    soft_millisecs_per_mib: 10,
    hard_millisecs_per_mib: 2000,
};

/// Timeout rates in milliseconds per MiB of uncompressed archive data.
pub(super) struct ArchiveTaskTimeoutRates {
    soft_millisecs_per_mib: u64,
    hard_millisecs_per_mib: u64,
}

/// A non-empty set of distinct dataset names.
///
/// # Type Parameters
///
/// * `'dataset_lifetime` - The lifetime of the borrowed dataset names.
#[derive(Debug)]
pub(super) struct NonEmptyDatasetNameSet<'dataset_lifetime> {
    names: HashSet<&'dataset_lifetime DatasetName>,
}

impl<'dataset_lifetime> NonEmptyDatasetNameSet<'dataset_lifetime> {
    /// # Returns
    ///
    /// An iterator over the distinct dataset names.
    pub(super) fn iter(&self) -> impl Iterator<Item = &'dataset_lifetime DatasetName> + '_ {
        self.names.iter().copied()
    }

    /// # Returns
    ///
    /// The number of distinct dataset names.
    pub(super) fn len(&self) -> usize {
        self.names.len()
    }
}

impl<'dataset_lifetime> From<&'dataset_lifetime NonEmptyDatasetNames>
    for NonEmptyDatasetNameSet<'dataset_lifetime>
{
    fn from(names: &'dataset_lifetime NonEmptyDatasetNames) -> Self {
        Self {
            names: names.as_slice().iter().collect(),
        }
    }
}

/// Checks that every requested dataset exists in the metadata database.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`Error::InvalidQueryJobConfig`] if any requested dataset doesn't exist.
/// * Forwards [`sqlx::query::QueryScalar::fetch_one`]'s return values on failure.
pub(super) async fn ensure_datasets_exist(
    db_pool: &MySqlPool,
    db_config: &Database,
    datasets: &NonEmptyDatasetNameSet<'_>,
) -> Result<(), Error> {
    let datasets_table = db_config.datasets_table_name();
    let mut query_builder = sqlx::QueryBuilder::<sqlx::MySql>::new(format!(
        "SELECT COUNT(*) FROM `{datasets_table}` WHERE CAST(`name` AS BINARY) IN ("
    ));
    let mut separated_datasets = query_builder.separated(", ");
    for dataset in datasets.iter() {
        separated_datasets.push_bind(dataset);
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

/// Computes the retention cutoff relative to the job's creation time.
///
/// # Returns
///
/// The inclusive lower bound on archive end timestamps in Unix epoch milliseconds.
pub(super) const fn retention_cutoff_millisecs(
    job_creation_timestamp_millisecs: i64,
    archive_retention_period_millisecs: NonZeroU64,
) -> i64 {
    job_creation_timestamp_millisecs
        .saturating_sub_unsigned(archive_retention_period_millisecs.get())
}

/// Derives a task's execution policy from the size of the archive it processes.
///
/// The soft timeout is an aggressive estimate of the task's execution time, whereas the hard
/// timeout is a conservative deadline that assumes the entire archive is extracted.
///
/// # Returns
///
/// The task's execution policy.
pub(super) fn task_execution_policy(
    archive_uncompressed_size: u64,
    rates: &ArchiveTaskTimeoutRates,
    max_num_retry: u32,
) -> ExecutionPolicy {
    const BYTES_PER_MIB: u64 = 1024 * 1024;
    const MIN_SOFT_TIMEOUT_MILLISECS: u64 = 1000;
    const MIN_HARD_TIMEOUT_MILLISECS: u64 = 10_000;

    /// The maximum timeout accepted by Spider, in milliseconds (24 hours).
    const MAX_TIMEOUT_MILLISECS: u64 = 1000 * 60 * 60 * 24;

    const _: () = assert!(MIN_SOFT_TIMEOUT_MILLISECS < MIN_HARD_TIMEOUT_MILLISECS);
    const _: () = assert!(MIN_HARD_TIMEOUT_MILLISECS <= MAX_TIMEOUT_MILLISECS);

    let hard_timeout_ms = (archive_uncompressed_size.saturating_mul(rates.hard_millisecs_per_mib)
        / BYTES_PER_MIB)
        .clamp(MIN_HARD_TIMEOUT_MILLISECS, MAX_TIMEOUT_MILLISECS);
    let soft_timeout_ms = (archive_uncompressed_size.saturating_mul(rates.soft_millisecs_per_mib)
        / BYTES_PER_MIB)
        .clamp(MIN_SOFT_TIMEOUT_MILLISECS, hard_timeout_ms - 1);

    ExecutionPolicy {
        max_num_retry,
        timeout_policy: TimeoutPolicy {
            soft_timeout_ms,
            hard_timeout_ms,
        },
        ..ExecutionPolicy::default()
    }
}

#[cfg(test)]
mod tests {
    use std::num::NonZeroU64;

    use super::SEARCH_TIMEOUT_RATES;
    use super::retention_cutoff_millisecs;
    use super::task_execution_policy;

    #[test]
    fn retention_cutoff_is_relative_to_job_creation() {
        assert_eq!(
            retention_cutoff_millisecs(
                1500,
                NonZeroU64::new(1000).expect("1000 is a positive retention period")
            ),
            500
        );
        assert_eq!(
            retention_cutoff_millisecs(i64::MIN, NonZeroU64::MIN),
            i64::MIN
        );
    }

    #[test]
    fn search_task_timeouts_scale_with_archive_size_and_respect_limits() {
        const MIB: u64 = 1024 * 1024;
        const MAX_NUM_RETRY: u32 = 3;

        for (archive_size, expected_soft_timeout_ms, expected_hard_timeout_ms) in [
            (1, 1_000, 10_000),
            (1024 * MIB + MIB / 2, 10_245, 2_049_000),
            (u64::MAX, 86_399_999, 86_400_000),
        ] {
            let policy = task_execution_policy(archive_size, &SEARCH_TIMEOUT_RATES, MAX_NUM_RETRY);
            assert_eq!(
                policy.timeout_policy.soft_timeout_ms, expected_soft_timeout_ms,
                "soft timeout for {archive_size} bytes"
            );
            assert_eq!(
                policy.timeout_policy.hard_timeout_ms, expected_hard_timeout_ms,
                "hard timeout for {archive_size} bytes"
            );
            assert_eq!(policy.max_num_retry, MAX_NUM_RETRY);
        }
    }
}

//! The commit worker that publishes a compression job's archives to CLP's metadata store.

use anyhow::Context;
use clp_rust_utils::clp_config::package::credentials;
use clp_rust_utils::database::mysql::create_clp_db_mysql_pool;
use clp_rust_utils::dataset::DatasetName;
use clp_rust_utils::job_config::COMPRESSION_JOBS_TABLE_NAME;
use clp_rust_utils::job_config::CompressionJobId;
use clp_rust_utils::job_config::CompressionJobStatus;
use clp_rust_utils::task_io::compression::ArchiveMetadata;
use clp_rust_utils::task_io::compression::CompressionTaskOutput;
use const_format::formatcp;
use secrecy::SecretString;
use spider_core::types::id::JobId;

use crate::common::clp_home;
use crate::common::spider_task_executor_config;

/// Publishes a compression job's archives and marks it succeeded.
///
/// Registers the dataset and archives together with the job's final sizes, completed task count,
/// and duration in one transaction. A no-op if the job is already
/// [`CompressionJobStatus::Succeeded`].
///
/// # Errors
///
/// Returns an error if:
///
/// * [`anyhow::Error`] if no CLP compression job exists for `spider_job_id`, or its status is
///   neither [`CompressionJobStatus::Running`] nor [`CompressionJobStatus::Succeeded`].
/// * Forwards [`db_credentials_from_env`]'s return values on failure.
/// * Forwards [`create_clp_db_mysql_pool`]'s return values on failure.
/// * Forwards [`sqlx::Pool::begin`]'s return values on failure.
/// * Forwards [`sqlx::query::QueryAs::fetch_optional`]'s return values on failure.
/// * Forwards [`register_dataset`]'s return values on failure.
/// * Forwards [`insert_archives`]'s return values on failure.
/// * Forwards [`mark_job_succeeded`]'s return values on failure.
/// * Forwards [`sqlx::Transaction::commit`]'s return values on failure.
///
/// # Panics
///
/// Panics if the executor configuration or CLP home has not been initialized, or an archive-size
/// total overflows with overflow checks enabled.
pub(super) async fn commit(
    spider_job_id: JobId,
    dataset: &DatasetName,
    archives: Vec<ArchiveMetadata>,
) -> anyhow::Result<()> {
    const QUERY: &str = formatcp!(
        "SELECT id, status FROM {table} WHERE spider_id = ? FOR UPDATE",
        table = COMPRESSION_JOBS_TABLE_NAME,
    );

    tracing::info!(
        spider_job_id = spider_job_id.get(),
        "CLP compression commit task started."
    );
    let total_uncompressed_size: i64 = archives.iter().map(|a| a.uncompressed_size).sum();
    let total_compressed_size: i64 = archives.iter().map(|a| a.size).sum();

    let config = spider_task_executor_config();
    // The dataset is interpolated into the table name (it cannot be bound). This is safe from SQL
    // injection because a `DatasetName` only contains underscores, letters, and numbers.
    let archives_table = config.database.archives_table_name(dataset);

    let pool = create_clp_db_mysql_pool(&config.database, &db_credentials_from_env()?, 1)
        .await
        .context("failed to create the CLP DB connection pool")?;
    let mut tx = pool
        .begin()
        .await
        .context("failed to begin the commit transaction")?;

    let job: Option<(CompressionJobId, CompressionJobStatus)> = sqlx::query_as(QUERY)
        .bind(spider_job_id.get())
        .fetch_optional(&mut *tx)
        .await
        .context("failed to look up the CLP compression job")?;
    let Some((id, status)) = job else {
        anyhow::bail!(
            "no CLP compression job found for spider job {}",
            spider_job_id.get()
        );
    };
    if status == CompressionJobStatus::Succeeded {
        tracing::info!(
            job_id = id,
            spider_job_id = spider_job_id.get(),
            "CLP compression job already committed; nothing to do."
        );
        return Ok(());
    }
    if status != CompressionJobStatus::Running {
        anyhow::bail!("CLP compression job {id} is no longer running; refusing to commit");
    }

    register_dataset(&mut tx, config, dataset).await?;
    insert_archives(&mut tx, &archives_table, &archives).await?;
    mark_job_succeeded(&mut tx, id, total_uncompressed_size, total_compressed_size).await?;

    tx.commit()
        .await
        .context("failed to commit the transaction")?;
    tracing::info!(
        job_id = id,
        num_archives = archives.len(),
        "CLP compression commit task completed successfully."
    );
    Ok(())
}

/// Combines compression outputs belonging to one dataset.
///
/// # Returns
///
/// The combined output on success.
///
/// # Errors
///
/// Returns [`anyhow::Error`] if outputs are missing or belong to different datasets.
pub(super) fn collect_outputs(
    outputs: Vec<CompressionTaskOutput>,
) -> anyhow::Result<CompressionTaskOutput> {
    let mut outputs = outputs.into_iter();
    let mut combined = outputs
        .next()
        .ok_or_else(|| anyhow::anyhow!("commit received no compression task outputs"))?;
    for output in outputs {
        if output.dataset != combined.dataset {
            anyhow::bail!("compression task outputs belong to more than one dataset");
        }
        combined.archives.extend(output.archives);
    }
    Ok(combined)
}

/// Idempotently registers `dataset` in the `datasets` table, recording its archive storage
/// directory.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
///
/// # Panics
///
/// Panics if CLP home has not been initialized.
async fn register_dataset(
    tx: &mut sqlx::Transaction<'_, sqlx::MySql>,
    config: &clp_rust_utils::clp_config::package::config::SpiderTaskExecutorConfig,
    dataset: &DatasetName,
) -> anyhow::Result<()> {
    let datasets_table = config.database.datasets_table_name();
    let archive_storage_directory = config.dataset_archive_storage_directory(clp_home(), dataset);
    sqlx::query(&format!(
        "INSERT INTO `{datasets_table}` (name, archive_storage_directory) VALUES (?, ?) ON \
         DUPLICATE KEY UPDATE archive_storage_directory = VALUES(archive_storage_directory)"
    ))
    .bind(dataset)
    .bind(&archive_storage_directory)
    .execute(&mut **tx)
    .await
    .with_context(|| format!("failed to register dataset in `{datasets_table}`"))?;
    Ok(())
}

/// Inserts every archive's metadata into `archives_table`.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
async fn insert_archives(
    tx: &mut sqlx::Transaction<'_, sqlx::MySql>,
    archives_table: &str,
    archives: &[ArchiveMetadata],
) -> anyhow::Result<()> {
    for archives in archives.chunks(1000) {
        let mut builder = sqlx::QueryBuilder::<sqlx::MySql>::new(format!(
            "INSERT INTO `{archives_table}` (id, begin_timestamp, end_timestamp, \
             uncompressed_size, size, creator_id, creation_ix) "
        ));
        builder.push_values(archives, |mut row, archive| {
            // NOTE: clp-s does not set `creator_id` or `creation_ix`.
            row.push_bind(archive.id)
                .push_bind(archive.begin_timestamp)
                .push_bind(archive.end_timestamp)
                .push_bind(archive.uncompressed_size)
                .push_bind(archive.size)
                .push_bind("")
                .push_bind(0_i32);
        });
        builder
            .build()
            .execute(&mut **tx)
            .await
            .with_context(|| format!("failed to insert archives into `{archives_table}`"))?;
    }
    Ok(())
}

/// Marks a running compression job succeeded with its final sizes, completed task count, and
/// duration.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`anyhow::Error`] if the update does not affect exactly one running job.
/// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
async fn mark_job_succeeded(
    tx: &mut sqlx::Transaction<'_, sqlx::MySql>,
    id: CompressionJobId,
    total_uncompressed_size: i64,
    total_compressed_size: i64,
) -> anyhow::Result<()> {
    // NOTE: `duration` (seconds) is derived from the DB clock so it stays consistent with the DB's
    // time.
    const QUERY: &str = formatcp!(
        "UPDATE {table} SET status = ?, uncompressed_size = ?, compressed_size = ?, \
         num_tasks_completed = num_tasks, duration = TIMESTAMPDIFF(MICROSECOND, start_time, \
         CURRENT_TIMESTAMP(3)) / 1000000 WHERE id = ? AND status = ?",
        table = COMPRESSION_JOBS_TABLE_NAME,
    );
    let query_result = sqlx::query(QUERY)
        .bind(CompressionJobStatus::Succeeded)
        .bind(total_uncompressed_size)
        .bind(total_compressed_size)
        .bind(id)
        .bind(CompressionJobStatus::Running)
        .execute(&mut **tx)
        .await
        .context("failed to mark the CLP compression job succeeded")?;

    if query_result.rows_affected() != 1 {
        anyhow::bail!(
            "failed to mark CLP compression job {} succeeded; expected 1 row to be updated, got {}",
            id,
            query_result.rows_affected()
        );
    }

    Ok(())
}

/// Reads the CLP DB credentials from `CLP_DB_USER` and `CLP_DB_PASS` environment variables.
///
/// # Returns
///
/// The CLP DB credentials.
///
/// # Errors
///
/// Returns an error if:
///
/// * Either env var is unset or not valid Unicode.
fn db_credentials_from_env() -> anyhow::Result<credentials::Database> {
    const CLP_DB_USER_ENV_VAR: &str = "CLP_DB_USER";
    const CLP_DB_PASS_ENV_VAR: &str = "CLP_DB_PASS";
    let user = std::env::var(CLP_DB_USER_ENV_VAR).with_context(|| {
        format!("failed to read the `{CLP_DB_USER_ENV_VAR}` environment variable")
    })?;
    let password = std::env::var(CLP_DB_PASS_ENV_VAR).with_context(|| {
        format!("failed to read the `{CLP_DB_PASS_ENV_VAR}` environment variable")
    })?;
    Ok(credentials::Database {
        user,
        password: SecretString::from(password),
    })
}

#[cfg(test)]
mod tests {
    //! Validation of the compression task outputs accepted for commit.

    use clp_rust_utils::dataset::DatasetName;
    use clp_rust_utils::task_io::compression::CompressionTaskOutput;

    use super::collect_outputs;

    #[test]
    fn commit_requires_outputs_but_accepts_empty_archives() {
        let error = collect_outputs(vec![]).expect_err("missing task outputs must be rejected");
        assert!(error.to_string().contains("no compression task outputs"));
        let dataset = DatasetName::try_from("empty_dataset".to_owned())
            .expect("test dataset name should be valid");
        let output = collect_outputs(vec![CompressionTaskOutput {
            dataset: dataset.clone(),
            archives: vec![],
        }])
        .expect("a valid task can produce no archives");
        assert_eq!(output.dataset, dataset);
        assert_eq!(output.archives, vec![]);
    }

    #[test]
    fn commit_rejects_mixed_datasets() {
        let error = collect_outputs(vec![
            CompressionTaskOutput {
                dataset: DatasetName::default(),
                archives: vec![],
            },
            CompressionTaskOutput {
                dataset: DatasetName::try_from("other".to_owned())
                    .expect("test dataset name should be valid"),
                archives: vec![],
            },
        ])
        .expect_err("task outputs from different datasets must not be committed together");
        assert!(error.to_string().contains("more than one dataset"));
    }
}

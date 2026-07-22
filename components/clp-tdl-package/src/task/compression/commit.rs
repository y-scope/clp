//! The commit worker that publishes a compression job's archives to CLP's metadata store.

use anyhow::Context;
use clp_rust_utils::{
    clp_config::package::credentials,
    database::mysql::create_clp_db_mysql_pool,
    dataset::{VALID_DATASET_NAME_REGEX, resolve_dataset_name},
    job_config::{CompressionJobId, CompressionJobStatus},
    task_io::compression::ArchiveMetadata,
};
use secrecy::SecretString;
use spider_core::types::id::JobId;

use crate::common::spider_task_executor_config;

/// Publishes a compression job's archives and marks it succeeded.
///
/// In one DB transaction, idempotently registers the dataset in the `datasets` table, inserts all
/// `archives`, and CAS-transitions the CLP compression job (found by reverse-lookup on `spider_id`)
/// from [`CompressionJobStatus::Running`] to [`CompressionJobStatus::Succeeded`], recording the
/// job's total sizes and duration. A no-op if the job is already
/// [`CompressionJobStatus::Succeeded`].
///
/// # Errors
///
/// Returns an error if:
///
/// * `archives` is empty.
/// * `dataset` is not a valid dataset name.
/// * No CLP compression job exists for `spider_job_id`.
/// * The CLP compression job is in a state other than [`CompressionJobStatus::Running`] or
///   [`CompressionJobStatus::Succeeded`].
/// * Forwards [`db_credentials_from_env`]'s return values on failure.
/// * Forwards [`create_clp_db_mysql_pool`]'s return values on failure.
/// * Forwards [`sqlx::Pool::begin`]'s return values on failure.
/// * Forwards [`sqlx::query::Query::fetch_optional`]'s return values on failure.
/// * Forwards [`register_dataset`]'s return values on failure.
/// * Forwards [`insert_archives`]'s return values on failure.
/// * Forwards [`mark_job_succeeded`]'s return values on failure.
/// * Forwards [`sqlx::Transaction::commit`]'s return values on failure.
pub(super) async fn commit(
    spider_job_id: JobId,
    dataset: Option<String>,
    archives: Vec<ArchiveMetadata>,
) -> anyhow::Result<()> {
    tracing::info!(
        spider_job_id = spider_job_id.get(),
        "CLP compression commit task started."
    );
    if archives.is_empty() {
        anyhow::bail!("commit received no archives to publish");
    }
    let total_uncompressed_size: i64 = archives.iter().map(|a| a.uncompressed_size).sum();
    let total_compressed_size: i64 = archives.iter().map(|a| a.size).sum();

    let config = spider_task_executor_config();

    // The dataset is interpolated into the table name (it cannot be bound), so guard against SQL
    // injection by validating it against the allowed-name pattern.
    if let Some(dataset) = dataset.as_deref()
        && !VALID_DATASET_NAME_REGEX.is_match(dataset)
    {
        anyhow::bail!("invalid dataset name: {dataset}");
    }
    let archives_table = config.database.archives_table_name(dataset.as_deref());

    let pool = create_clp_db_mysql_pool(&config.database, &db_credentials_from_env()?, 1)
        .await
        .context("failed to create the CLP DB connection pool")?;
    let mut tx = pool
        .begin()
        .await
        .context("failed to begin the commit transaction")?;

    let job: Option<(CompressionJobId, CompressionJobStatus)> =
        sqlx::query_as("SELECT id, status FROM compression_jobs WHERE spider_id = ? FOR UPDATE")
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

    register_dataset(&mut tx, config, dataset.as_deref()).await?;
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

/// Idempotently registers `dataset` (defaulting a missing one to the `CLP_S` default) in the
/// `datasets` table, recording its archive storage directory.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
async fn register_dataset(
    tx: &mut sqlx::Transaction<'_, sqlx::MySql>,
    config: &clp_rust_utils::clp_config::package::config::SpiderTaskExecutorConfig,
    dataset: Option<&str>,
) -> anyhow::Result<()> {
    let datasets_table = config.database.datasets_table_name();
    let archive_storage_directory = config
        .archive_output
        .dataset_archive_storage_directory(dataset);
    sqlx::query(&format!(
        "INSERT INTO `{datasets_table}` (name, archive_storage_directory) VALUES (?, ?) ON \
         DUPLICATE KEY UPDATE archive_storage_directory = VALUES(archive_storage_directory)"
    ))
    .bind(resolve_dataset_name(dataset))
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
             uncompressed_size, size, creator_id, creation_ix);"
        ));
        builder.push_values(archives, |mut row, archive| {
            // NOTE: clp-s does not set `creator_id` or `creation_ix`.
            row.push_bind(&archive.id)
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

/// CAS-transitions the compression job `id` to [`CompressionJobStatus::Succeeded`], recording its
/// total sizes and the DB-clock-derived duration.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
async fn mark_job_succeeded(
    tx: &mut sqlx::Transaction<'_, sqlx::MySql>,
    id: CompressionJobId,
    total_uncompressed_size: i64,
    total_compressed_size: i64,
) -> anyhow::Result<()> {
    // NOTE: `duration` (seconds) is derived from the DB clock so it stays consistent with the DB's
    // time.
    sqlx::query(
        "UPDATE compression_jobs SET status = ?, uncompressed_size = ?, compressed_size = ?, \
         duration = TIMESTAMPDIFF(MICROSECOND, start_time, CURRENT_TIMESTAMP(3)) / 1000000, \
         update_time = CURRENT_TIMESTAMP() WHERE id = ?",
    )
    .bind(CompressionJobStatus::Succeeded)
    .bind(total_uncompressed_size)
    .bind(total_compressed_size)
    .bind(id)
    .execute(&mut **tx)
    .await
    .context("failed to mark the CLP compression job succeeded")?;
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

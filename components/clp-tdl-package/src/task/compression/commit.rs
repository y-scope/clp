//! The commit worker that publishes a compression job's archives to CLP's metadata store.

use anyhow::Context;
use clp_rust_utils::clp_config::package::credentials;
use clp_rust_utils::database::mysql::create_clp_db_mysql_pool;
use clp_rust_utils::dataset::DatasetName;
use clp_rust_utils::dataset::register_dataset as register_dataset_in_db;
use clp_rust_utils::job_config::COMPRESSION_JOBS_TABLE_NAME;
use clp_rust_utils::job_config::CompressionJobId;
use clp_rust_utils::job_config::CompressionJobStatus;
use clp_rust_utils::task_io::compression::ArchiveMetadata;
use const_format::formatcp;
use secrecy::SecretString;
use spider_core::types::id::JobId;

use crate::common::spider_task_executor_config;

/// Mirror of `clp_py_utils.clp_metadata_db_utils.LONG_SPAN_THRESHOLD_MILLIS`.
const LONG_SPAN_THRESHOLD_MILLIS: i64 = 24 * 60 * 60 * 1000;

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
    dataset: &DatasetName,
    archives: Vec<ArchiveMetadata>,
) -> anyhow::Result<()> {
    const QUERY: &str = formatcp!(
        "SELECT id, status FROM {COMPRESSION_JOBS_TABLE_NAME} WHERE spider_id = ? FOR UPDATE"
    );

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
    let archives_table = config.database.archives_table_name();
    let long_span_archives_table = config.database.long_span_archives_table_name();

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

    let dataset_id = register_dataset(&mut tx, config, dataset).await?;
    insert_archives(
        &mut tx,
        &archives_table,
        &long_span_archives_table,
        dataset_id,
        &archives,
    )
    .await?;
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

/// Idempotently registers `dataset` in the `datasets` table, recording its archive storage path.
///
/// # Returns
///
/// The dataset's ID.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`register_dataset_in_db`]'s return values on failure.
async fn register_dataset(
    tx: &mut sqlx::Transaction<'_, sqlx::MySql>,
    config: &clp_rust_utils::clp_config::package::config::SpiderTaskExecutorConfig,
    dataset: &DatasetName,
) -> anyhow::Result<u64> {
    let datasets_table = config.database.datasets_table_name();
    let archive_storage_path = config
        .archive_output
        .dataset_archive_storage_directory(dataset);
    register_dataset_in_db(tx, &datasets_table, &archive_storage_path, dataset)
        .await
        .with_context(|| format!("failed to register dataset in `{datasets_table}`"))
}

/// Inserts every archive's metadata into `archives_table`, and records those whose timestamp range
/// exceeds [`LONG_SPAN_THRESHOLD_MILLIS`] in `long_span_archives_table`.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
async fn insert_archives(
    tx: &mut sqlx::Transaction<'_, sqlx::MySql>,
    archives_table: &str,
    long_span_archives_table: &str,
    dataset_id: u64,
    archives: &[ArchiveMetadata],
) -> anyhow::Result<()> {
    for archives in archives.chunks(1000) {
        let mut builder = sqlx::QueryBuilder::<sqlx::MySql>::new(format!(
            "INSERT INTO `{archives_table}` (dataset_id, uuid, timestamp_range_begin_millis, \
             timestamp_range_end_millis, num_uncompressed_bytes, num_compressed_bytes, \
             creation_time_millis) "
        ));
        builder.push_values(archives, |mut row, archive| {
            row.push_bind(dataset_id)
                .push_bind(archive.id.as_bytes().as_slice())
                .push_bind(archive.begin_timestamp)
                .push_bind(archive.end_timestamp)
                .push_bind(archive.uncompressed_size)
                .push_bind(archive.size)
                .push("CAST(UNIX_TIMESTAMP(CURRENT_TIMESTAMP(3)) * 1000 AS SIGNED)");
        });
        builder
            .build()
            .execute(&mut **tx)
            .await
            .with_context(|| format!("failed to insert archives into `{archives_table}`"))?;

        let long_span_archives: Vec<&ArchiveMetadata> = archives
            .iter()
            .filter(|archive| {
                LONG_SPAN_THRESHOLD_MILLIS < archive.end_timestamp - archive.begin_timestamp
            })
            .collect();
        if long_span_archives.is_empty() {
            continue;
        }

        let placeholders = vec!["?"; long_span_archives.len()].join(", ");
        let statement = format!(
            "INSERT INTO `{long_span_archives_table}` (dataset_id, timestamp_range_begin_millis, \
             timestamp_range_end_millis, archive_id) SELECT dataset_id, \
             timestamp_range_begin_millis, timestamp_range_end_millis, id FROM `{archives_table}` \
             WHERE `dataset_id` = ? AND `uuid` IN ({placeholders})"
        );
        let mut query = sqlx::query(&statement).bind(dataset_id);
        for archive in long_span_archives {
            query = query.bind(archive.id.as_bytes().as_slice());
        }
        query.execute(&mut **tx).await.with_context(|| {
            format!("failed to insert archives into `{long_span_archives_table}`")
        })?;
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
    const QUERY: &str = formatcp!(
        "UPDATE {COMPRESSION_JOBS_TABLE_NAME} SET status = ?, uncompressed_size = ?, \
         compressed_size = ?, duration = TIMESTAMPDIFF(MICROSECOND, start_time, \
         CURRENT_TIMESTAMP(3)) / 1000000 WHERE id = ? AND status = ?"
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

//! Publication of completed timeline histograms.

use std::time::Instant;

use anyhow::Context;
use clp_rust_utils::clp_config::package::config::SpiderTaskExecutorConfig;
use clp_rust_utils::database::mysql::create_clp_db_mysql_pool;
use clp_rust_utils::job_config::QueryJobId;
use clp_rust_utils::timeline::Bucket;
use clp_rust_utils::timeline::COMMIT_MARKER_ID;
use clp_rust_utils::timeline::aggregate_source_contributions;
use futures::TryStreamExt;
use mongodb::Client;
use mongodb::Collection;
use mongodb::bson::Document;
use mongodb::bson::doc;
use mongodb::options::ReadConcern;
use mongodb::options::ReadPreference;
use mongodb::options::ReplaceOneModel;
use mongodb::options::SelectionCriteria;
use spider_core::types::id::JobId;

use crate::task::query::results_cache::parse_results_cache_uri;
use crate::task::utils::db_credentials_from_env;

/// Reduces successful archive contributions and publishes the completed histogram.
///
/// Reads use the primary with local read concern to include acknowledged archive contributions.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`anyhow::Error`] if no query job is associated with `spider_job_id` or the results-cache URI
///   has no database.
/// * Forwards [`db_credentials_from_env`]'s return values on failure.
/// * Forwards [`create_clp_db_mysql_pool`]'s return values on failure.
/// * Forwards [`sqlx::query::QueryScalar::fetch_optional`]'s return values on failure.
/// * Forwards [`parse_results_cache_uri`]'s return values on failure.
/// * Forwards [`Client::with_options`]'s return values on failure.
/// * Forwards [`publish_timeline_buckets`]'s return values on failure.
pub(super) async fn commit_timeline_results(
    spider_job_id: JobId,
    config: &SpiderTaskExecutorConfig,
) -> anyhow::Result<()> {
    let pool = create_clp_db_mysql_pool(&config.database, &db_credentials_from_env()?, 1)
        .await
        .context("failed to create the CLP DB connection pool")?;
    let query_job_id: QueryJobId =
        sqlx::query_scalar("SELECT id FROM query_jobs WHERE spider_id = ?")
            .bind(spider_job_id.get())
            .fetch_optional(&pool)
            .await
            .context("failed to look up the CLP query job")?
            .with_context(|| {
                format!(
                    "no CLP query job found for spider job {}",
                    spider_job_id.get()
                )
            })?;
    let collection_name = format!("{query_job_id}-timeline");
    let started = Instant::now();
    tracing::info!(query_job_id, "Started timeline commit.");
    let mut options = parse_results_cache_uri(config.results_cache.uri().as_str()).await?;
    options.selection_criteria = Some(SelectionCriteria::ReadPreference(ReadPreference::Primary));
    // A majority read can omit acknowledged w=1 contributions even on the primary.
    options.read_concern = Some(ReadConcern::local());
    options.app_name = Some("clp-timeline-commit".to_owned());
    let database = options
        .default_database
        .clone()
        .context("results-cache URI must name a database")?;
    let client = Client::with_options(options)?;
    let collection = client
        .database(&database)
        .collection(collection_name.as_str());
    publish_timeline_buckets(&collection).await?;
    tracing::info!(
        query_job_id,
        elapsed_seconds = started.elapsed().as_secs_f64(),
        "Completed timeline commit."
    );
    Ok(())
}

/// Writes validated buckets in bounded batches, then publishes the completion marker.
///
/// Source contributions are retained. Numeric bucket IDs cannot collide with their object IDs,
/// and replacement upserts make publication safe to repeat after acknowledgement loss. The
/// query and its archive inputs must remain immutable throughout retries.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`aggregate_source_contributions`]'s return values on failure.
/// * Forwards [`TryStreamExt::try_next`]'s return values on failure.
/// * Forwards [`Bucket::try_from_aggregation_document`]'s return values on failure.
/// * Forwards [`Client::bulk_write`]'s return values on failure.
/// * Forwards [`Collection::replace_one`]'s return values on failure.
async fn publish_timeline_buckets(collection: &Collection<Document>) -> anyhow::Result<()> {
    const BATCH_SIZE: usize = 1_000;
    let client = collection.client();
    let mut cursor = aggregate_source_contributions(collection).await?;
    let mut batch = Vec::with_capacity(BATCH_SIZE);
    while let Some(document) = cursor.try_next().await? {
        let bucket = Bucket::try_from_aggregation_document(&document)?;
        batch.push(
            ReplaceOneModel::builder()
                .namespace(collection.namespace())
                .filter(doc! { "_id": bucket.timestamp() })
                .replacement(doc! { "_id": bucket.timestamp(), "count": bucket.count() })
                .upsert(true)
                .build(),
        );
        if batch.len() == BATCH_SIZE {
            client
                .bulk_write(std::mem::take(&mut batch))
                .write_concern(client.write_concern().cloned().unwrap_or_default())
                .await?;
        }
    }
    if !batch.is_empty() {
        client
            .bulk_write(batch)
            .write_concern(client.write_concern().cloned().unwrap_or_default())
            .await?;
    }
    collection
        .replace_one(
            doc! { "_id": COMMIT_MARKER_ID },
            doc! { "_id": COMMIT_MARKER_ID },
        )
        .upsert(true)
        .await?;
    Ok(())
}

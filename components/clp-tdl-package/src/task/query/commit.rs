//! Publication of completed timeline histograms.

use std::time::Instant;

use anyhow::Context;
use clp_rust_utils::task_io::query::OutputHandle;
use clp_rust_utils::task_io::query::QueryTaskOutput;
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

use crate::task::query::results_cache::parse_results_cache_uri;

/// Reduces successful archive contributions and publishes the completed histogram.
///
/// Reads use the primary with local read concern to include acknowledged archive contributions.
///
/// # Errors
///
/// * [`anyhow::Error`] if outputs are absent, have inconsistent destinations, lack the expected
///   timeline collection, specify an unsupported output handler, or lack a URI database.
/// * Forwards [`parse_results_cache_uri`]'s return values on failure.
/// * Forwards [`Client::with_options`]'s return values on failure.
/// * Forwards [`publish_timeline_buckets`]'s return values on failure.
pub(super) async fn commit_timeline_results(outputs: &[QueryTaskOutput]) -> anyhow::Result<()> {
    let output = outputs
        .first()
        .context("timeline commit received no task outputs")?;
    if outputs.iter().any(|candidate| candidate != output) {
        anyhow::bail!("timeline task outputs have inconsistent result destinations");
    }
    let collection_name = output
        .timeline_collection
        .as_ref()
        .context("timeline commit requires a timeline collection")?;
    if collection_name.as_str() != format!("{}-timeline", output.query_job_id) {
        anyhow::bail!("timeline collection does not match its query job ID");
    }
    let OutputHandle::ResultsCache { uri } = &output.output_handle else {
        anyhow::bail!("timeline commit requires results-cache output");
    };
    let started = Instant::now();
    tracing::info!(
        query_job_id = output.query_job_id,
        "Started timeline commit."
    );
    let mut options = parse_results_cache_uri(uri.as_str()).await?;
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
        query_job_id = output.query_job_id,
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

#[cfg(test)]
mod tests {
    use clp_rust_utils::task_io::query::OutputHandle;
    use clp_rust_utils::task_io::query::QueryTaskOutput;
    use clp_rust_utils::types::non_empty_string::ExpectedNonEmpty;
    use non_empty_string::NonEmptyString;

    use super::commit_timeline_results;

    /// # Returns
    ///
    /// An archive output identifying the timeline destination for query job 42.
    fn timeline_output() -> QueryTaskOutput {
        QueryTaskOutput {
            query_job_id: 42,
            output_handle: OutputHandle::ResultsCache {
                uri: NonEmptyString::from_static_str("mongodb://localhost/results"),
            },
            timeline_collection: Some(NonEmptyString::from_static_str("42-timeline")),
        }
    }

    #[tokio::test]
    async fn rejects_empty_archive_outputs() -> anyhow::Result<()> {
        let error = commit_timeline_results(&[])
            .await
            .expect_err("commit requires archive outputs");
        assert_eq!(
            error.to_string(),
            "timeline commit received no task outputs"
        );
        Ok(())
    }

    #[tokio::test]
    async fn rejects_inconsistent_archive_destinations() -> anyhow::Result<()> {
        let output = timeline_output();
        for different_output in [
            QueryTaskOutput {
                query_job_id: 43,
                ..output.clone()
            },
            QueryTaskOutput {
                output_handle: OutputHandle::ResultsCache {
                    uri: NonEmptyString::from_static_str("mongodb://localhost/other"),
                },
                ..output.clone()
            },
            QueryTaskOutput {
                timeline_collection: None,
                ..output.clone()
            },
            QueryTaskOutput {
                timeline_collection: Some(NonEmptyString::from_static_str("43-timeline")),
                ..output.clone()
            },
        ] {
            let error = commit_timeline_results(&[output.clone(), different_output])
                .await
                .expect_err("conflicting destinations must be rejected before publication");
            assert_eq!(
                error.to_string(),
                "timeline task outputs have inconsistent result destinations"
            );
        }
        Ok(())
    }

    #[tokio::test]
    async fn rejects_missing_or_incorrect_timeline_collection() -> anyhow::Result<()> {
        for collection in [None, Some("42"), Some("43-timeline")] {
            let output = QueryTaskOutput {
                timeline_collection: collection.map(NonEmptyString::from_static_str),
                ..timeline_output()
            };
            let error = commit_timeline_results(&[output])
                .await
                .expect_err("commit requires the query job's timeline collection");
            let expected = if collection.is_none() {
                "timeline commit requires a timeline collection"
            } else {
                "timeline collection does not match its query job ID"
            };
            assert_eq!(error.to_string(), expected);
        }
        Ok(())
    }

    #[tokio::test]
    async fn rejects_file_output() -> anyhow::Result<()> {
        let output = QueryTaskOutput {
            output_handle: OutputHandle::File,
            ..timeline_output()
        };
        let error = commit_timeline_results(&[output])
            .await
            .expect_err("commit requires results-cache output");
        assert_eq!(
            error.to_string(),
            "timeline commit requires results-cache output"
        );
        Ok(())
    }
}

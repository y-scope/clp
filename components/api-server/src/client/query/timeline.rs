//! Retrieval of the timelines of completed query jobs.

use async_stream::try_stream;
use clp_rust_utils::job_config::QueryJobId;
use clp_rust_utils::timeline;
use clp_rust_utils::timeline::Bucket;
use futures::Stream;
use futures::TryStreamExt;
use mongodb::bson::Document;
use mongodb::bson::doc;
use mongodb::options::DatabaseOptions;
use mongodb::options::Hint;
use mongodb::options::ReadPreference;
use mongodb::options::SelectionCriteria;

use crate::client::Client;
use crate::error::ClientError;

impl Client {
    /// Asynchronously fetches the committed timeline buckets of a completed query job.
    ///
    /// # Returns
    ///
    /// A stream of [`Bucket`] values in ascending timestamp order on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::InvalidInput`] if the query job has no timeline configuration.
    /// * [`ClientError::TimelineNotCommitted`] if the timeline's commit marker is absent.
    /// * Forwards [`Self::get_job_config`]'s return values on failure.
    /// * Forwards [`Self::wait_for_query_success`]'s return values on failure.
    /// * Forwards [`mongodb::Collection::find_one`]'s return values on failure.
    ///
    /// When consumed, the stream yields [`ClientError::Mongo`] if querying or reading buckets
    /// fails, and [`ClientError::Timeline`] if [`Bucket::try_from_committed_document`] fails.
    ///
    /// # Panics
    ///
    /// Panics if an existing query job's ID does not fit in [`QueryJobId`].
    pub async fn fetch_timeline(
        &self,
        query_job_id: u64,
    ) -> Result<impl Stream<Item = Result<Bucket, ClientError>> + use<>, ClientError> {
        let output = self.get_job_config(query_job_id).await?.output;
        let timeline_query_job_id: QueryJobId = query_job_id
            .try_into()
            .expect("an existing query job's ID fits in `QueryJobId`");
        if !output.has_timeline() {
            return Err(ClientError::InvalidInput(
                "query job has no timeline configuration".to_owned(),
            ));
        }
        self.wait_for_query_success(query_job_id).await?;
        let started = std::time::Instant::now();
        let database = self.mongodb_client.database_with_options(
            &self.config.results_cache.db_name,
            DatabaseOptions::builder()
                .selection_criteria(SelectionCriteria::ReadPreference(ReadPreference::Primary))
                .build(),
        );
        let timeline_name = timeline::collection_name(timeline_query_job_id);
        let collection = database.collection::<Document>(&timeline_name);
        collection
            .find_one(doc! { "_id": timeline::COMMIT_MARKER_ID })
            .await?
            .ok_or(ClientError::TimelineNotCommitted(query_job_id))?;
        let buckets = try_stream! {
            let mut num_buckets = 0_u64;
            let mut cursor = collection
                .find(doc! { "_id": { "$type": "number" } })
                .sort(doc! { "_id": 1 })
                .hint(Hint::Name("_id_".to_owned()))
                .await?;
            while let Some(document) = cursor.try_next().await? {
                let bucket = Bucket::try_from_committed_document(&document)
                    .map_err(ClientError::from)?;
                yield bucket;
                num_buckets += 1;
            }
            tracing::debug!(
                query_job_id,
                num_buckets,
                elapsed_micros = started.elapsed().as_micros(),
                "Finished reading timeline results."
            );
        };
        Ok(buckets.inspect_err(move |error: &ClientError| {
            tracing::error!(
                query_job_id,
                error = % error,
                "An error occurred when streaming timeline results."
            );
        }))
    }
}

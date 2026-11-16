//! Retrieval of committed timeline buckets.

use async_stream::try_stream;
use clp_rust_utils::job_config::Aggregation;
use clp_rust_utils::job_config::QueryOutput;
use clp_rust_utils::job_config::ResultsCacheOutput;
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
use serde::Serialize;
use utoipa::ToSchema;

use super::Client;
use super::ClientError;

#[derive(Debug, Eq, PartialEq, Serialize, ToSchema)]
pub struct TimelineBucket {
    #[schema(minimum = -9_007_199_254_740_991_i64, maximum = 9_007_199_254_740_991_i64)]
    pub timestamp: i64,
    #[schema(minimum = 0, maximum = 9_007_199_254_740_991_i64)]
    pub count: i64,
}

impl TryFrom<Bucket> for TimelineBucket {
    type Error = ClientError;

    fn try_from(bucket: Bucket) -> Result<Self, Self::Error> {
        const MAX_SAFE_INTEGER: i64 = (1_i64 << 53) - 1;
        let timestamp = bucket.timestamp();
        let count = bucket.count();
        if !(-MAX_SAFE_INTEGER..=MAX_SAFE_INTEGER).contains(&timestamp) || count > MAX_SAFE_INTEGER
        {
            return Err(ClientError::MalformedData);
        }
        Ok(Self { timestamp, count })
    }
}

impl Client {
    /// Asynchronously fetches committed timeline buckets for a query job.
    ///
    /// # Returns
    ///
    /// A stream of [`TimelineBucket`] values in ascending timestamp order on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::InvalidInput`] if the query has no timeline configuration.
    /// * [`ClientError::MalformedData`] if the query ID is out of range or the commit marker is
    ///   absent.
    /// * [`ClientError::Mongo`] if reading the commit marker fails.
    /// * Forwards [`Self::get_job_config`]'s return values on failure.
    /// * Forwards [`Self::wait_for_query_success`]'s return values on failure.
    ///
    /// When consumed, the stream yields [`ClientError::Mongo`] if querying or reading buckets
    /// fails, [`ClientError::MalformedData`] if [`Bucket::try_from_committed_document`] fails,
    /// and forwards [`TimelineBucket::try_from`]'s return values on failure.
    pub async fn fetch_timeline(
        &self,
        query_job_id: u64,
    ) -> Result<impl Stream<Item = Result<TimelineBucket, ClientError>> + use<>, ClientError> {
        let output = self.get_job_config(query_job_id).await?.output;
        if !matches!(
            output,
            QueryOutput::ResultsCache(ResultsCacheOutput {
                aggregation: Some(Aggregation::CountByTime { .. }),
                ..
            })
        ) {
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
        let timeline_name = timeline::collection_name(
            query_job_id
                .try_into()
                .map_err(|_| ClientError::MalformedData)?,
        );
        let collection = database.collection::<Document>(&timeline_name);
        collection
            .find_one(doc! { "_id": timeline::COMMIT_MARKER_ID })
            .await?
            .ok_or(ClientError::MalformedData)?;
        Ok(try_stream! {
            let mut num_buckets = 0_u64;
            let mut cursor = collection
                .find(doc! { "_id": { "$type": "number" } })
                .sort(doc! { "_id": 1 })
                .hint(Hint::Name("_id_".to_owned()))
                .await?;
            while let Some(document) = cursor.try_next().await? {
                let bucket = Bucket::try_from_committed_document(&document)
                    .map_err(ClientError::from)?;
                yield TimelineBucket::try_from(bucket)?;
                num_buckets += 1;
            }
            tracing::debug!(
                query_job_id,
                num_buckets,
                elapsed_micros = started.elapsed().as_micros(),
                "Finished reading timeline results."
            );
        })
    }
}

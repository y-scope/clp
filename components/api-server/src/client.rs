use std::num::NonZeroU32;
use std::pin::Pin;
use std::sync::Arc;
use std::time::Duration;

use async_stream::stream;
use async_stream::try_stream;
use axum::body::Bytes;
use chrono::DateTime;
use chrono::TimeZone;
use chrono::Utc;
use clp_rust_utils::aws::AWS_DEFAULT_REGION;
use clp_rust_utils::clp_config::package::config::Config;
use clp_rust_utils::clp_config::package::config::QueryJobPollingConfig;
use clp_rust_utils::clp_config::package::config::StreamOutputStorage;
use clp_rust_utils::database::mysql::query_job;
use clp_rust_utils::job_config::Aggregation;
use clp_rust_utils::job_config::Cancellability;
pub use clp_rust_utils::job_config::CompressionJobStatus;
use clp_rust_utils::job_config::QUERY_JOBS_TABLE_NAME;
use clp_rust_utils::job_config::QueryJobConfig;
use clp_rust_utils::job_config::QueryJobId;
use clp_rust_utils::job_config::QueryJobStatus;
use clp_rust_utils::job_config::QueryOutput;
use clp_rust_utils::job_config::ResultsCacheOutput;
use clp_rust_utils::job_config::SearchQuery;
use clp_rust_utils::timeline;
use clp_rust_utils::timeline::Bucket;
use clp_rust_utils::types::ArchiveId;
use futures::Stream;
use futures::StreamExt;
use futures::TryStreamExt;
use mongodb::bson::Document;
use mongodb::bson::doc;
use mongodb::options::DatabaseOptions;
use mongodb::options::Hint;
use mongodb::options::ReadPreference;
use mongodb::options::SelectionCriteria;
use pin_project_lite::pin_project;
use query_result_listener::listener::ResultListener;
use query_result_listener::session::SessionConfig;
use query_result_listener::session::SessionEvent;
use query_result_listener::session::SessionStats;
use serde::Deserialize;
use serde::Serialize;
use sqlx::Row;
use utoipa::IntoParams;
use utoipa::ToSchema;

pub use crate::error::ClientError;
use crate::sse::SseEncoding;
use crate::sse::encode_event;

/// Default job statuses to include when the caller does not specify `job_status`.
/// Covers all terminal states that consumed compute resources (succeeded, failed).
pub const DEFAULT_JOB_STATUSES: &[CompressionJobStatus] = &[
    CompressionJobStatus::Succeeded,
    CompressionJobStatus::Failed,
];

/// Query parameters for the compression usage endpoint.
#[derive(Deserialize, IntoParams)]
#[into_params(parameter_in = Query)]
pub struct CompressionUsageParams {
    /// Start of usage window (epoch milliseconds, inclusive).
    pub time_range_begin_millisecs: i64,
    /// End of usage window (epoch milliseconds, exclusive).
    pub time_range_end_millisecs: i64,
    /// Job statuses to include as a comma-separated list (e.g.
    /// `job_status=succeeded,failed`). Recognized values (case-insensitive):
    /// `PENDING`, `RUNNING`, `SUCCEEDED`, `FAILED`.
    /// Defaults to `SUCCEEDED,FAILED` (all terminal states).
    #[serde(default)]
    pub job_status: Option<String>,
    /// Maximum number of jobs to return. Must be > 0; defaults to 1000.
    #[serde(default = "default_limit")]
    pub limit: i64,
}

const fn default_limit() -> i64 {
    1000
}

/// Validated parameters produced by [`TryFrom<CompressionUsageParams>`].
pub struct ValidatedCompressionUsageParams {
    pub time_range_begin: DateTime<Utc>,
    pub time_range_end: DateTime<Utc>,
    pub job_statuses: Vec<CompressionJobStatus>,
    pub limit: i64,
}

impl TryFrom<CompressionUsageParams> for ValidatedCompressionUsageParams {
    type Error = ClientError;

    /// Validates the parameters and resolves the requested job statuses into
    /// [`CompressionJobStatus`] variants.
    ///
    /// # Errors
    ///
    /// Returns [`ClientError::InvalidInput`] if:
    /// - `time_range_begin_millisecs > time_range_end_millisecs`
    /// - `time_range_begin_millisecs` or `time_range_end_millisecs` is outside the representable
    ///   range
    /// - `limit <= 0`
    /// - `job_status` contains unrecognized, empty, or duplicate values
    fn try_from(value: CompressionUsageParams) -> Result<Self, Self::Error> {
        if value.time_range_begin_millisecs > value.time_range_end_millisecs {
            return Err(ClientError::InvalidInput(
                "time_range_begin_millisecs must be <= time_range_end_millisecs".to_owned(),
            ));
        }
        if value.limit <= 0 {
            return Err(ClientError::InvalidInput("limit must be > 0".to_owned()));
        }
        let time_range_begin = Utc
            .timestamp_millis_opt(value.time_range_begin_millisecs)
            .earliest();
        let time_range_end = Utc
            .timestamp_millis_opt(value.time_range_end_millisecs)
            .earliest();
        let (Some(time_range_begin), Some(time_range_end)) = (time_range_begin, time_range_end)
        else {
            return Err(ClientError::InvalidInput(
                "timestamp out of representable range".to_owned(),
            ));
        };
        let job_statuses = match &value.job_status {
            Some(s) => {
                let parsed: Vec<CompressionJobStatus> = s
                    .split(',')
                    .map(str::trim)
                    .filter(|t| !t.is_empty())
                    .map(|token| {
                        token.parse().map_err(|_| {
                            ClientError::InvalidInput(format!("Unknown job_status: {token}"))
                        })
                    })
                    .collect::<Result<Vec<_>, _>>()?;
                let mut seen = Vec::new();
                for &status in &parsed {
                    if seen.contains(&status) {
                        return Err(ClientError::InvalidInput(format!(
                            "Duplicate job_status: {status:?}"
                        )));
                    }
                    seen.push(status);
                }
                parsed
            }
            None => DEFAULT_JOB_STATUSES.to_vec(),
        };
        if job_statuses.is_empty() {
            return Err(ClientError::InvalidInput(
                "job_status must contain at least one valid status".to_owned(),
            ));
        }
        Ok(Self {
            time_range_begin,
            time_range_end,
            job_statuses,
            limit: value.limit,
        })
    }
}

/// Resource usage statistics for the compression job with the specified ID.
#[derive(Serialize, sqlx::FromRow, ToSchema)]
pub struct CompressionUsage {
    /// Compression job ID.
    pub id: i32,
    /// Current status of the job.
    #[sqlx(rename = "status", try_from = "i32")]
    pub job_status: CompressionJobStatus,
    /// Time the job was created (epoch milliseconds).
    #[serde(with = "chrono::serde::ts_milliseconds")]
    #[schema(value_type = i64)]
    #[sqlx(rename = "creation_time")]
    pub time_creation_millisecs: DateTime<Utc>,
    /// Time the job started executing (epoch milliseconds).
    #[serde(with = "chrono::serde::ts_milliseconds")]
    #[schema(value_type = i64)]
    #[sqlx(rename = "start_time")]
    pub time_begin_millisecs: DateTime<Utc>,
    /// Wall-clock duration the job ran, in seconds. Absent if the job did not complete.
    #[sqlx(rename = "duration")]
    pub duration_secs: Option<f64>,
    /// Total uncompressed size of input files, in bytes.
    #[sqlx(rename = "uncompressed_size")]
    pub uncompressed_size_bytes: i64,
    /// Total compressed archive size, in bytes.
    #[sqlx(rename = "compressed_size")]
    pub compressed_size_bytes: i64,
    /// Number of tasks the job was split into.
    pub num_tasks: i32,
    /// Sum of all task durations, in seconds. Absent if no tasks reported a duration.
    #[sqlx(rename = "tasks_duration")]
    pub tasks_duration_secs: Option<f64>,
}

/// Defines the request configuration for submitting a query job.
#[derive(Clone, Debug, Serialize, Deserialize, ToSchema)]
#[serde(deny_unknown_fields)]
pub struct QueryConfig {
    /// The query to run.
    pub query: SearchQuery,

    /// Where to store the query results.
    pub output: QueryOutputConfig,
}

impl From<QueryConfig> for QueryJobConfig {
    fn from(config: QueryConfig) -> Self {
        Self {
            query: config.query,
            output: config.output.into(),
        }
    }
}

/// Defines the request configuration for submitting a query job whose results are streamed instead
/// of buffered.
#[derive(Debug, Deserialize, ToSchema)]
#[serde(deny_unknown_fields)]
pub struct StreamingQueryConfig {
    /// The query to run.
    pub query: SearchQuery,
}

/// Defines where a submitted query job's results are buffered.
#[derive(Clone, Debug, Serialize, Deserialize, ToSchema)]
#[serde(deny_unknown_fields, tag = "type", rename_all = "snake_case")]
pub enum QueryOutputConfig {
    /// Buffers the results in the results cache (backed by `MongoDB`).
    ResultsCache {
        /// The maximum number of results to keep. Omit for no limit.
        #[serde(default)]
        #[schema(value_type = Option<u32>, minimum = 1)]
        max_num_results: Option<NonZeroU32>,

        /// The aggregation to compute over all matching results, regardless of `max_num_results`.
        #[serde(default)]
        aggregation: Option<Aggregation>,
    },

    /// Buffers the results in files stored in the stream output storage.
    File {},
}

impl From<QueryOutputConfig> for QueryOutput {
    fn from(output: QueryOutputConfig) -> Self {
        match output {
            QueryOutputConfig::ResultsCache {
                max_num_results,
                aggregation,
            } => Self::ResultsCache(ResultsCacheOutput {
                max_num_results,
                aggregation,
            }),
            QueryOutputConfig::File {} => Self::File {},
        }
    }
}

#[derive(Clone)]
pub struct Client {
    #[allow(clippy::struct_field_names)]
    mongodb_client: mongodb::Client,
    sql_pool: sqlx::Pool<sqlx::MySql>,
    config: Config,
    query_result_listener: Arc<ResultListener<SseEncoding>>,
}

impl Client {
    /// Factory function.
    ///
    /// Creates a client that shares the given `MongoDB` client, `MySQL` connection pool and query
    /// result listener.
    ///
    /// # Returns
    ///
    /// A newly created [`Client`].
    #[must_use]
    pub const fn new(
        config: Config,
        mongodb_client: mongodb::Client,
        sql_pool: sqlx::Pool<sqlx::MySql>,
        query_result_listener: Arc<ResultListener<SseEncoding>>,
    ) -> Self {
        Self {
            mongodb_client,
            sql_pool,
            config,
            query_result_listener,
        }
    }

    /// Submits a search or aggregation query as a job.
    ///
    /// # Returns
    ///
    /// The unique ID of the newly created query job on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`query_job::submit`]'s return values on failure.
    pub async fn submit_query(
        &self,
        query_job_config: &QueryJobConfig,
    ) -> Result<u64, ClientError> {
        Ok(query_job::submit(&self.sql_pool, query_job_config).await?)
    }

    /// Submits a query job that runs `query` and streams its results to a new session of the
    /// client's query result listener.
    ///
    /// If the query fails, or the returned stream is dropped before the job terminates, the job's
    /// cancellation is requested.
    ///
    /// # Returns
    ///
    /// A stream of the job's events, encoded as Server-Sent Events, on success:
    ///
    /// * `job`, the first event, whose [`StreamingQueryJobEvent`] payload identifies the query job.
    /// * One unnamed event per result, whose [`StreamingQueryResultEvent`] payload carries the
    ///   result. Results from different archives can arrive in any order.
    /// * `end`, which closes the stream and reports the job's terminal status in a
    ///   [`StreamingQueryEndEvent`] payload.
    /// * `error`, which closes the stream in place of `end` if an error occurs after the job is
    ///   submitted, with a [`StreamingQueryErrorEvent`] payload.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`ResultListener::open_session`]'s return values on failure.
    /// * Forwards [`Client::submit_query`]'s return values on failure.
    pub async fn submit_streaming_query(
        &self,
        query: SearchQuery,
    ) -> Result<impl Stream<Item = Result<Bytes, serde_json::Error>> + Send + use<>, ClientError>
    {
        let open_session = self
            .query_result_listener
            .open_session(SessionConfig::default())?;
        let query_job_config = QueryJobConfig {
            query,
            output: QueryOutput::Network(open_session.network_output().clone()),
        };
        let query_job_id = self.submit_query(&query_job_config).await?;
        tracing::info!(query_job_id, "Submitted a streaming query.");

        let wait_for_termination = {
            let client = self.clone();
            async move { client.wait_for_query_job_termination(query_job_id).await }
        };
        let request_cancellation = {
            let client = self.clone();
            async move {
                match client.cancel_query_job(query_job_id).await {
                    Ok(()) => tracing::info!(
                        query_job_id,
                        "Requested the cancellation of a streaming query's job."
                    ),
                    Err(ClientError::QueryJobAlreadyTerminated(_)) => tracing::info!(
                        query_job_id,
                        "Skipped the cancellation of a streaming query's job, which had already \
                         terminated."
                    ),
                    Err(error) => tracing::warn!(
                        query_job_id,
                        error = % error,
                        "Failed to request the cancellation of a streaming query's job."
                    ),
                }
            }
        };
        Ok(encode_streaming_query_events(
            query_job_id,
            open_session.into_events(wait_for_termination, request_cancellation),
        ))
    }

    /// Asynchronously fetches the results of a completed query job.
    ///
    /// # Returns
    ///
    /// A stream of the job's results on success. The stream variant indicates where the results
    /// are stored:
    ///
    /// * [`SearchResultStream::File`] if the job's results are stored in files.
    /// * [`SearchResultStream::Mongo`] if the job's results are stored in `MongoDB`.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::QueryResultsNotStored`] if the job is configured to stream its results to a
    ///   network listener, which means they're not stored.
    /// * Forwards [`Client::wait_for_query_success`]'s return values on failure.
    /// * Forwards [`Client::get_job_config`]'s return values on failure.
    /// * Forwards [`Client::fetch_results_from_mongo`]'s return values on failure.
    /// * Forwards [`Client::fetch_results_from_s3`]'s return values on failure.
    pub async fn fetch_results(
        &self,
        query_job_id: u64,
        raw_docs: bool,
        sorted: bool,
    ) -> Result<
        SearchResultStream<
            impl Stream<Item = Result<String, ClientError>> + use<>,
            impl Stream<Item = Result<String, ClientError>> + use<>,
            impl Stream<Item = Result<String, ClientError>> + use<>,
        >,
        ClientError,
    > {
        let output = self.get_job_config(query_job_id).await?.output;
        match output {
            QueryOutput::ResultsCache(ResultsCacheOutput {
                max_num_results, ..
            }) => {
                self.wait_for_query_success(query_job_id).await?;
                self.fetch_results_from_mongo(query_job_id, max_num_results, raw_docs, sorted)
                    .await
                    .map(|inner| SearchResultStream::Mongo { inner })
            }
            QueryOutput::File {} => {
                self.wait_for_query_success(query_job_id).await?;
                Ok(match &self.config.stream_output.storage {
                    StreamOutputStorage::Fs { .. } => SearchResultStream::File {
                        inner: self.fetch_results_from_file(query_job_id),
                    },
                    StreamOutputStorage::S3 { .. } => SearchResultStream::S3 {
                        inner: self.fetch_results_from_s3(query_job_id).await?,
                    },
                })
            }
            QueryOutput::Network(_) => Err(ClientError::QueryResultsNotStored(query_job_id)),
        }
    }

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

    /// Submits a cancellation request for a query job.
    ///
    /// This method is idempotent: if a cancellation has already been requested for the job, it's a
    /// no-op that returns `Ok(())`.
    ///
    /// # Returns
    ///
    /// `Ok(())` on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::QueryJobNotFound`] if the job doesn't exist.
    /// * [`ClientError::QueryJobAlreadyTerminated`] if the job has already succeeded, failed, or
    ///   been killed.
    /// * Forwards [`query_job::request_cancellation`]'s return values on failure.
    pub async fn cancel_query_job(&self, query_job_id: u64) -> Result<(), ClientError> {
        match query_job::request_cancellation(&self.sql_pool, query_job_id).await? {
            None => Err(ClientError::QueryJobNotFound(query_job_id)),
            Some(Cancellability::Cancellable | Cancellability::CancellationRequested) => Ok(()),
            Some(Cancellability::NotCancellable) => {
                Err(ClientError::QueryJobAlreadyTerminated(query_job_id))
            }
        }
    }

    /// Retrieves the status of a previously submitted query job.
    ///
    /// # Returns
    ///
    /// The current status of the job on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::QueryJobNotFound`] if the job doesn't exist.
    /// * Forwards [`query_job::get_status`]'s return values on failure.
    pub async fn get_status(&self, query_job_id: u64) -> Result<QueryJobStatus, ClientError> {
        query_job::get_status(&self.sql_pool, query_job_id)
            .await?
            .ok_or(ClientError::QueryJobNotFound(query_job_id))
    }

    /// Waits until a query job terminates, reading its status with the backoff of the API server's
    /// query job polling configuration.
    ///
    /// # Returns
    ///
    /// The terminal status of the job on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Client::get_status`]'s return values on failure.
    async fn wait_for_query_job_termination(
        &self,
        query_job_id: u64,
    ) -> Result<QueryJobStatus, ClientError> {
        let mut backoff =
            QueryJobPollingBackoff::new(&self.get_api_server_config().query_job_polling);
        loop {
            let status = self.get_status(query_job_id).await?;
            if status.is_terminal() {
                return Ok(status);
            }
            tokio::time::sleep(backoff.next_delay()).await;
        }
    }

    /// Waits until a query succeeds, fails, or begins cancellation.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::QueryNotSucceeded`] if the query fails or cancellation has begun.
    /// * Forwards [`Self::get_status`]'s return values on failure.
    async fn wait_for_query_success(&self, query_job_id: u64) -> Result<(), ClientError> {
        let mut backoff =
            QueryJobPollingBackoff::new(&self.get_api_server_config().query_job_polling);
        loop {
            match self.get_status(query_job_id).await? {
                QueryJobStatus::Succeeded => {
                    break;
                }
                QueryJobStatus::Failed
                | QueryJobStatus::CancelRequested
                | QueryJobStatus::Cancelling
                | QueryJobStatus::Cancelled
                | QueryJobStatus::Killed => {
                    return Err(ClientError::QueryNotSucceeded);
                }
                QueryJobStatus::Running | QueryJobStatus::Pending => {
                    tokio::time::sleep(backoff.next_delay()).await;
                }
            }
        }

        Ok(())
    }

    async fn get_job_config(&self, query_job_id: u64) -> Result<QueryJobConfig, ClientError> {
        let row = sqlx::query(&format!(
            "SELECT job_config FROM `{QUERY_JOBS_TABLE_NAME}` WHERE id = ?"
        ))
        .bind(query_job_id)
        .fetch_optional(&self.sql_pool)
        .await?
        .ok_or(ClientError::QueryJobNotFound(query_job_id))?;
        let msgpack: &[u8] = row.try_get("job_config")?;
        Ok(rmp_serde::from_slice(msgpack)?)
    }

    /// Asynchronously fetches results of a completed query job from files.
    ///
    /// # Returns
    ///
    /// A stream of the job's results. Each item in the stream is a [`Result`] that:
    ///
    /// ## Returns
    ///
    /// The log message in string representation on success.
    ///
    /// ## Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`std::fs::File::open`]'s return values on failure.
    /// * Forwards [`tokio::fs::read_dir`]'s return values on failure.
    /// * Forwards [`tokio::fs::ReadDir::next_entry`]'s return values on failure.
    ///
    /// # Panics
    ///
    /// Panics if the stream output storage is not file system.
    fn fetch_results_from_file(
        &self,
        query_job_id: u64,
    ) -> impl Stream<Item = Result<String, ClientError>> + use<> {
        let StreamOutputStorage::Fs { directory } = &self.config.stream_output.storage else {
            unreachable!();
        };
        let query_job_output_dir = std::path::Path::new("/")
            .join(directory)
            .join(query_job_id.to_string());
        stream! {
            let mut entries = tokio::fs::read_dir(query_job_output_dir).await?;
            while let Some(entry) = entries.next_entry().await? {
                let search_result_path = entry.path();
                let reader = std::fs::File::open(search_result_path)?;
                let mut deserializer = rmp_serde::Deserializer::new(reader);
                while let Ok(event) = Deserialize::deserialize(&mut deserializer) {
                    let event: (i64, String, String, String, i64) = event;
                    yield Ok(event.1);
                }
            }
        }
    }

    /// Asynchronously fetches results of a completed query job from S3.
    ///
    /// # Returns
    ///
    /// A stream of the job's results on success. Each item in the stream is a [`Result`] that:
    ///
    /// ## Returns
    ///
    /// The log message in string representation on success.
    ///
    /// ## Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards the pagination stream's error returned from S3 list-object-v2 operation's
    ///   paginator (i.e., from
    ///   [`aws_smithy_async::future::pagination_stream::PaginationStream::next`]).
    /// * Forwards S3 get-object operation's return values on failure (i.e., from
    ///   [`aws_sdk_s3::operation::get_object::builders::GetObjectFluentBuilder::send`]).
    /// * Forwards [`aws_smithy_types::byte_stream::ByteStream::collect`]'s return values on
    ///   failure.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::Aws`] if a region code is not provided when using the default AWS S3
    ///   endpoint.
    ///
    /// # Panics
    ///
    /// Panics if the stream output storage is not S3.
    async fn fetch_results_from_s3(
        &self,
        query_job_id: u64,
    ) -> Result<impl Stream<Item = Result<String, ClientError>> + use<>, ClientError> {
        tracing::info!("Streaming results from S3");
        let StreamOutputStorage::S3 { s3_config, .. } = &self.config.stream_output.storage else {
            unreachable!();
        };

        let s3_config = s3_config.clone();
        if s3_config.region_code.is_none() && s3_config.endpoint_url.is_none() {
            return Err(ClientError::Aws {
                description: "a region code must be given when using the default AWS S3 endpoint"
                    .to_owned(),
            });
        }

        let region_str = s3_config
            .region_code
            .as_ref()
            .map_or(AWS_DEFAULT_REGION, non_empty_string::NonEmptyString::as_str);
        let s3_client = clp_rust_utils::s3::create_new_client(
            region_str,
            s3_config.endpoint_url.as_ref(),
            &s3_config.aws_authentication,
        )
        .await;

        let key_prefix = format!("{}{}/", s3_config.key_prefix, query_job_id);
        tracing::info!("Streaming results from S3 prefix: {}", key_prefix);
        let mut object_pages = s3_client
            .list_objects_v2()
            .bucket(s3_config.bucket.as_str())
            .prefix(key_prefix)
            .into_paginator()
            .send();

        Ok(stream! {
            while let Some(object_page) = object_pages.next().await {
                tracing::debug!("Received S3 object page: {:?}", object_page);
                for object in object_page?.contents() {
                    let Some(key) = object.key() else {
                        tracing::info!("S3 object {:?} doesn't have a key", object);
                        continue;
                    };
                    if key.ends_with('/') {
                        tracing::info!("Skipping S3 object with key {} as it is a directory", key);
                        continue;
                    }
                    tracing::info!("Streaming results from S3 object with key: {}", key);
                    let obj = s3_client
                        .get_object()
                        .bucket(s3_config.bucket.as_str())
                        .key(key)
                        .send()
                        .await?;
                    let bytes = obj.body.collect().await?.into_bytes();
                    let mut deserializer = rmp_serde::Deserializer::from_read_ref(bytes.as_ref());
                    while let Ok(event) = Deserialize::deserialize(&mut deserializer) {
                        let event: (i64, String, String, String, i64) = event;
                        yield Ok(event.1);
                    }
                }
            }
        })
    }

    /// Asynchronously fetches results of a completed query job from `MongoDB`.
    ///
    /// When `max_num_results` is set, the query is limited to at most that many documents.
    ///
    /// When `raw_docs` is `true`, each search document is serialized to JSON whole; otherwise
    /// the document's "message" field is extracted. Timeline records are excluded.
    ///
    /// When `sorted` is `true`, documents are returned sorted by `timestamp` descending (then
    /// `_id` ascending); otherwise, they're returned in insertion order.
    ///
    /// # Returns
    ///
    /// A stream of the job's results on success. Each item in the stream is a [`Result`] that:
    ///
    /// ## Returns
    ///
    /// A parsed JSON value representing a search result on success.
    ///
    /// ## Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::MalformedData`] if a retrieved document does not contain a "message" field,
    ///   or if the "message" field is not a BSON string.
    /// * Forwards [`mongodb::error::Error`] produced by the `MongoDB` cursor item access.
    /// * Forwards [`serde_json::to_string`]'s return values on failure.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`mongodb::Collection::find`]'s return values on failure.
    async fn fetch_results_from_mongo(
        &self,
        query_job_id: u64,
        max_num_results: Option<NonZeroU32>,
        raw_docs: bool,
        sorted: bool,
    ) -> Result<impl Stream<Item = Result<String, ClientError>> + use<>, ClientError> {
        let database = self
            .mongodb_client
            .database(&self.config.results_cache.db_name);
        let collection: mongodb::Collection<mongodb::bson::Document> =
            database.collection(&query_job_id.to_string());
        let mut find_options = mongodb::options::FindOptions::default();
        if let Some(limit) = max_num_results {
            find_options.limit = Some(i64::from(limit.get()));
        }
        if sorted {
            find_options.sort = Some(mongodb::bson::doc! {
                "timestamp": -1,
                "_id": 1,
            });
        }
        let cursor = collection
            .find(mongodb::bson::doc! {
                "_id": { "$type": "object" },
                "_id.log_event_idx": { "$type": ["int", "long"] },
                "_id.timestamp": { "$exists": false },
                "count": { "$exists": false },
            })
            .with_options(find_options)
            .await?;

        let mapped = cursor.map(move |res| {
            let doc = res?;
            if raw_docs {
                return Ok(serde_json::to_string(&doc)?);
            }
            let Some(msg) = doc.get("message") else {
                return Err(ClientError::MalformedData);
            };
            let mongodb::bson::Bson::String(message) = msg else {
                return Err(ClientError::MalformedData);
            };
            Ok(message.clone())
        });
        Ok(mapped)
    }

    /// Returns compression usage for each job within a time range, optionally
    /// filtered by job outcome.
    ///
    /// # Returns
    ///
    /// A vector of [`CompressionUsage`] (one per job) on success, ordered by
    /// `start_time` descending.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::query::Query::fetch_all`]'s return values on failure.
    /// * Forwards [`sqlx::Row::try_get`]'s return values on failure.
    pub async fn get_compression_usage(
        &self,
        params: &ValidatedCompressionUsageParams,
    ) -> Result<Vec<CompressionUsage>, ClientError> {
        // Build the job-status IN clause dynamically so the DB sees a
        // static `IN (?, ?, ...)` predicate and can use an index on `j.status`.
        let placeholders = params
            .job_statuses
            .iter()
            .map(|_| "?")
            .collect::<Vec<_>>()
            .join(", ");
        let job_status_clause = format!(" AND j.status IN ({placeholders})");

        #[rustfmt::skip]
        let sql = format!(
            // MAX() is used on non-aggregated columns because they are all
            // functionally dependent on j.id (the primary key). Since each group
            // contains exactly one row, MAX() simply returns the column's value.
            // This is used instead of MySQL-only ANY_VALUE() for MariaDB compat.
            //
            // sqlx binds DateTime<Utc> as a UTC timestamp, so we compare
            // directly against the DATETIME columns in the database.
            //
            // Task durations are summed regardless of individual task status so
            // that failed jobs still report the compute resources they consumed.
            //
            // INNER JOIN filters out jobs with zero tasks (e.g. failed before
            // task creation) since they consumed no compute resources.
            "SELECT \
              j.id, \
              MAX(j.status) AS status, \
              MAX(j.creation_time) AS creation_time, \
              MAX(j.start_time) AS start_time, \
              MAX(j.duration) AS duration, \
              MAX(j.uncompressed_size) AS uncompressed_size, \
              MAX(j.compressed_size) AS compressed_size, \
              MAX(j.num_tasks) AS num_tasks, \
              SUM(t.duration) AS tasks_duration \
            FROM compression_jobs j \
            JOIN compression_tasks t ON t.job_id = j.id \
            WHERE j.start_time >= ? \
              AND j.start_time < ? \
              {job_status_clause} \
            GROUP BY j.id \
            ORDER BY MAX(j.start_time) DESC \
            LIMIT ?"
        );

        let mut query = sqlx::query_as::<_, CompressionUsage>(&sql)
            .bind(params.time_range_begin)
            .bind(params.time_range_end);
        for &status in &params.job_statuses {
            query = query.bind(i32::from(status));
        }
        query = query.bind(params.limit);
        query.fetch_all(&self.sql_pool).await.map_err(Into::into)
    }

    /// # Returns
    ///
    /// A reference to the API server configuration.
    ///
    /// # Panics
    ///
    /// Panics if `self.config.api_server` is `None`.
    const fn get_api_server_config(
        &self,
    ) -> &clp_rust_utils::clp_config::package::config::ApiServer {
        self.config
            .api_server
            .as_ref()
            .expect("api_server configuration is missing")
    }
}

pin_project! {
    /// Enum for search result streams from different storage backends.
    ///
    /// # Type Parameters
    ///
    /// * `FileStream`: Streaming from file system storage.
    /// * `MongoStream`: Streaming from MongoDB storage.
    /// * `S3Stream`: Streaming from S3 storage.
    #[project = SearchResultStreamProj]
    pub enum SearchResultStream<FileStream, MongoStream, S3Stream>
    where
        FileStream: Stream<Item = Result<String, ClientError>>,
        MongoStream: Stream<Item = Result<String, ClientError>>,
        S3Stream: Stream<Item = Result<String, ClientError>>
    {
        File{#[pin] inner: FileStream},
        Mongo{#[pin] inner: MongoStream},
        S3{#[pin] inner: S3Stream},
    }
}

impl<FileStream, MongoStream, S3Stream> Stream
    for SearchResultStream<FileStream, MongoStream, S3Stream>
where
    FileStream: Stream<Item = Result<String, ClientError>>,
    MongoStream: Stream<Item = Result<String, ClientError>>,
    S3Stream: Stream<Item = Result<String, ClientError>>,
{
    type Item = Result<String, ClientError>;

    fn poll_next(
        self: Pin<&mut Self>,
        cx: &mut std::task::Context<'_>,
    ) -> std::task::Poll<Option<Self::Item>> {
        let poll = match self.project() {
            SearchResultStreamProj::File { inner } => inner.poll_next(cx),
            SearchResultStreamProj::Mongo { inner } => inner.poll_next(cx),
            SearchResultStreamProj::S3 { inner } => inner.poll_next(cx),
        };
        if let std::task::Poll::Ready(Some(Err(err))) = &poll {
            tracing::error!("An error occurred when streaming results: {}", err);
        }
        poll
    }
}

/// The data of a streaming query's event.
#[expect(dead_code, reason = "only documents an `OpenAPI` schema")]
#[derive(ToSchema)]
#[serde(untagged)]
pub(crate) enum StreamingQueryEventData {
    Job(StreamingQueryJobEvent),
    Result(StreamingQueryResultEvent),
    End(StreamingQueryEndEvent),
    Error(StreamingQueryErrorEvent),
}

/// The data of a streaming query's `job` event.
#[derive(Serialize, ToSchema)]
pub(crate) struct StreamingQueryJobEvent {
    /// The ID of the query's job.
    query_job_id: u64,
}

/// The data of a streaming query's result event.
#[expect(dead_code, reason = "only documents an `OpenAPI` schema")]
#[derive(ToSchema)]
pub(crate) struct StreamingQueryResultEvent {
    /// The ID of the archive that contains the result.
    #[schema(value_type = String)]
    archive_id: ArchiveId,

    /// The result's timestamp, in milliseconds since the Unix epoch.
    timestamp_millisecs: i64,

    /// The result's message with its trailing newline removed.
    message: String,
}

/// The data of a streaming query's `end` event.
#[derive(Serialize, ToSchema)]
pub(crate) struct StreamingQueryEndEvent {
    /// The terminal status of the query's job.
    status: QueryJobStatus,

    /// The number of results streamed.
    num_results_emitted: u64,
}

/// The data of a streaming query's `error` event.
#[derive(Serialize, ToSchema)]
pub(crate) struct StreamingQueryErrorEvent {
    /// A generic description of the error.
    message: String,
}

/// The backoff between the reads of a query job's status: the first delay is the configured initial
/// backoff, and each next delay doubles the previous one, up to the configured maximum backoff.
struct QueryJobPollingBackoff {
    delay: Duration,
    max_delay: Duration,
}

impl QueryJobPollingBackoff {
    /// Factory function.
    ///
    /// # Returns
    ///
    /// A newly created backoff that `config` configures, starting at its first delay.
    const fn new(config: &QueryJobPollingConfig) -> Self {
        Self {
            delay: Duration::from_millis(config.initial_backoff_ms),
            max_delay: Duration::from_millis(config.max_backoff_ms),
        }
    }

    /// # Returns
    ///
    /// The delay before the next read of the status. The delay after it is doubled, up to the
    /// maximum.
    fn next_delay(&mut self) -> Duration {
        let delay = self.delay;
        self.delay = std::cmp::min(self.delay.saturating_mul(2), self.max_delay);
        delay
    }
}

/// Encodes `event_stream`, a stream of the events of a streaming query's job, as Server-Sent
/// Events.
///
/// # Returns
///
/// A stream that yields a `job` event whose data is a [`StreamingQueryJobEvent`], then the events
/// that [`encode_session_event`] encodes.
fn encode_streaming_query_events(
    query_job_id: u64,
    event_stream: impl Stream<Item = SessionEvent<Bytes>>,
) -> impl Stream<Item = Result<Bytes, serde_json::Error>> {
    let job_event = encode_event("job", &StreamingQueryJobEvent { query_job_id });
    futures::stream::once(futures::future::ready(job_event))
        .chain(event_stream.map(move |event| encode_session_event(query_job_id, event)))
}

/// Encodes `event`, an event of the streaming query whose job is `query_job_id`, as a Server-Sent
/// Event.
///
/// A [`SessionEvent::Ended`] is logged with `query_job_id` and the session's statistics, of which
/// only the number of results streamed is sent to the client. A [`SessionEvent::Failed`] is logged
/// with `query_job_id`, and its error isn't sent to the client.
///
/// # Returns
///
/// On success:
///
/// * The batch of [`SessionEvent::Results`] as it is.
/// * An `end` event whose data is a [`StreamingQueryEndEvent`] for [`SessionEvent::Ended`].
/// * An `error` event whose data is a [`StreamingQueryErrorEvent`] for [`SessionEvent::Failed`].
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`encode_event`]'s return values on failure.
fn encode_session_event(
    query_job_id: u64,
    event: SessionEvent<Bytes>,
) -> Result<Bytes, serde_json::Error> {
    const STREAMING_QUERY_ERROR_MESSAGE: &str = "Internal server error";

    match event {
        SessionEvent::Results(batch) => Ok(batch),
        SessionEvent::Ended { status, stats } => {
            let SessionStats {
                num_results_emitted,
                num_duplicates_dropped,
            } = stats;
            tracing::info!(
                query_job_id,
                status = ? status,
                num_results_emitted,
                num_duplicates_dropped,
                "A streaming query ended."
            );
            encode_event(
                "end",
                &StreamingQueryEndEvent {
                    status,
                    num_results_emitted,
                },
            )
        }
        SessionEvent::Failed(error) => {
            tracing::error!(query_job_id, error = % error, "A streaming query failed.");
            encode_event(
                "error",
                &StreamingQueryErrorEvent {
                    message: STREAMING_QUERY_ERROR_MESSAGE.to_owned(),
                },
            )
        }
    }
}

#[cfg(test)]
mod tests {
    use std::str::FromStr;

    use super::*;

    /// The ID of the query job whose events a test encodes.
    const QUERY_JOB_ID: u64 = 7;

    #[test]
    fn parse_job_status_pending_not_in_defaults() {
        assert_eq!(
            CompressionJobStatus::from_str("PENDING"),
            Ok(CompressionJobStatus::Pending)
        );
        assert!(!DEFAULT_JOB_STATUSES.contains(&CompressionJobStatus::Pending));
    }

    #[test]
    fn reject_duplicate_job_status() {
        let params = CompressionUsageParams {
            time_range_begin_millisecs: 0,
            time_range_end_millisecs: 1,
            job_status: Some("succeeded,succeeded".to_owned()),
            limit: 100,
        };
        let result = ValidatedCompressionUsageParams::try_from(params);
        assert!(result.is_err());
    }

    #[test]
    fn query_output_config_converts_to_the_matching_query_output() {
        use clp_rust_utils::types::PositiveI64;

        let max_num_results = NonZeroU32::new(7);
        let aggregation = Some(Aggregation::CountByTime {
            bucket_size_millisecs: PositiveI64::new(60_000).expect("60,000 is positive"),
        });
        for (output, expected) in [
            (
                QueryOutputConfig::ResultsCache {
                    max_num_results,
                    aggregation,
                },
                QueryOutput::ResultsCache(ResultsCacheOutput {
                    max_num_results,
                    aggregation,
                }),
            ),
            (QueryOutputConfig::File {}, QueryOutput::File {}),
        ] {
            assert_eq!(
                QueryOutput::from(output.clone()),
                expected,
                "output: {output:?}"
            );
        }
    }

    #[test]
    fn query_job_polling_backoff_doubles_its_delay_up_to_the_maximum() {
        let mut backoff = QueryJobPollingBackoff::new(&QueryJobPollingConfig {
            initial_backoff_ms: 100,
            max_backoff_ms: 500,
        });

        for expected_delay_ms in [100, 200, 400, 500, 500] {
            assert_eq!(
                backoff.next_delay(),
                Duration::from_millis(expected_delay_ms)
            );
        }
    }

    #[tokio::test]
    async fn streaming_query_events_are_the_job_event_the_batches_as_they_are_and_the_end()
    -> anyhow::Result<()> {
        use futures::stream;

        const FIRST_BATCH: &str = "data: a\n\n";
        const SECOND_BATCH: &str = "data: b\n\ndata: c\n\n";

        let event_stream = stream::iter([
            SessionEvent::Results(Bytes::from_static(FIRST_BATCH.as_bytes())),
            SessionEvent::Results(Bytes::from_static(SECOND_BATCH.as_bytes())),
            SessionEvent::Ended {
                status: QueryJobStatus::Cancelled,
                stats: SessionStats {
                    num_results_emitted: 3,
                    num_duplicates_dropped: 1,
                },
            },
        ]);

        let encoded_events = encode_streaming_query_events(QUERY_JOB_ID, event_stream)
            .try_collect::<Vec<_>>()
            .await?
            .concat();

        assert_eq!(
            String::from_utf8(encoded_events)?,
            [
                &format!("event: job\ndata: {{\"query_job_id\":{QUERY_JOB_ID}}}\n\n"),
                FIRST_BATCH,
                SECOND_BATCH,
                "event: end\ndata: ",
                r#"{"status":"Cancelled","num_results_emitted":3}"#,
                "\n\n",
            ]
            .concat()
        );
        Ok(())
    }

    #[tokio::test]
    async fn streaming_query_events_end_with_an_error_event_without_the_errors_details()
    -> anyhow::Result<()> {
        use futures::stream;
        use query_result_listener::session::SessionEventError;

        const BATCH: &str = "data: a\n\n";

        let event_stream = stream::iter([
            SessionEvent::Results(Bytes::from_static(BATCH.as_bytes())),
            SessionEvent::Failed(SessionEventError::Encoding("an internal detail".to_owned())),
        ]);

        let encoded_events = encode_streaming_query_events(QUERY_JOB_ID, event_stream)
            .try_collect::<Vec<_>>()
            .await?
            .concat();

        assert_eq!(
            String::from_utf8(encoded_events)?,
            [
                &format!("event: job\ndata: {{\"query_job_id\":{QUERY_JOB_ID}}}\n\n"),
                BATCH,
                "event: error\ndata: ",
                r#"{"message":"Internal server error"}"#,
                "\n\n",
            ]
            .concat()
        );
        Ok(())
    }

    #[tokio::test]
    async fn dropping_the_streaming_query_events_drops_the_jobs_events() -> anyhow::Result<()> {
        use futures::channel::mpsc;
        use query_result_listener::test_utils::TIMEOUT;

        let (event_sender, event_stream) = mpsc::unbounded::<SessionEvent<Bytes>>();
        let mut encoded_event_stream =
            Box::pin(encode_streaming_query_events(QUERY_JOB_ID, event_stream));
        tokio::time::timeout(TIMEOUT, encoded_event_stream.next())
            .await?
            .expect("the events should start with the job event")?;
        assert!(!event_sender.is_closed());

        drop(encoded_event_stream);

        assert!(event_sender.is_closed());
        Ok(())
    }
}

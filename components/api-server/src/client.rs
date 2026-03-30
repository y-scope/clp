use std::pin::Pin;
use std::sync::Arc;

use async_stream::stream;
use clp_rust_utils::{
    archive_metadata::{self, DEFAULT_S3_FETCH_CONCURRENCY},
    aws::AWS_DEFAULT_REGION,
    clp_config::package::{
        config::{Config, StorageEngine, StreamOutputStorage},
        credentials::Credentials,
    },
    database::mysql::create_clp_db_mysql_pool,
    job_config::{QUERY_JOBS_TABLE_NAME, QueryJobStatus, QueryJobType, SearchJobConfig},
};
use futures::{Stream, StreamExt, stream as futures_stream};
use pin_project_lite::pin_project;
use serde::{Deserialize, Serialize};
use sqlx::Row;
use utoipa::ToSchema;

pub use crate::error::ClientError;
use crate::query_dedup::QueryDedupCache;

/// Defines the request configuration for submitting a search query.
#[derive(Clone, Debug, Serialize, Deserialize, ToSchema)]
#[serde(deny_unknown_fields)]
pub struct QueryConfig {
    /// The search query as a KQL string.
    pub query_string: String,

    /// The datasets to search within. If not provided, only `default` dataset will be searched.
    #[serde(default)]
    pub datasets: Option<Vec<String>>,

    /// The maximum number of results to return. Set to `0` for no limit.
    #[serde(default)]
    pub max_num_results: u32,

    /// The beginning timestamp (in epoch milliseconds) for the search range (inclusive).
    #[serde(default)]
    pub time_range_begin_millisecs: Option<i64>,

    /// The ending timestamp (in epoch milliseconds) for the search range (inclusive).
    #[serde(default)]
    pub time_range_end_millisecs: Option<i64>,

    /// Whether the string match should be case-insensitive.
    #[serde(default)]
    pub ignore_case: bool,

    /// Whether to buffer search results in `MongoDB`.
    /// By default, search results are buffered in temporary files. When set to `true`, results
    /// will be stored in `MongoDB` instead.
    #[serde(default)]
    pub buffer_results_in_mongodb: bool,
}

impl From<QueryConfig> for SearchJobConfig {
    fn from(value: QueryConfig) -> Self {
        Self {
            datasets: value.datasets,
            query_string: value.query_string,
            max_num_results: value.max_num_results,
            begin_timestamp: value.time_range_begin_millisecs,
            end_timestamp: value.time_range_end_millisecs,
            ignore_case: value.ignore_case,
            write_to_file: !value.buffer_results_in_mongodb,
            ..Default::default()
        }
    }
}

#[derive(Clone)]
pub struct Client {
    #[allow(clippy::struct_field_names)]
    mongodb_client: mongodb::Client,
    sql_pool: sqlx::Pool<sqlx::MySql>,
    config: Config,
    /// Reusable S3 client, created once at connect time when stream output is S3.
    s3_client: Option<aws_sdk_s3::Client>,
    /// Query deduplication cache to avoid redundant jobs for identical queries.
    dedup_cache: Arc<QueryDedupCache>,
}

impl Client {
    /// Factory method to create a new client with active connections to both `MySQL` and `MongoDB`
    /// databases. If the stream output storage is S3, an S3 client is also created and reused
    /// across all subsequent requests.
    ///
    /// # Returns
    ///
    /// A newly created [`Client`] instance with active connections to both databases.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::ConfigIsNone`] if `config.api_server` is `None`.
    /// * Forwards [`create_clp_db_mysql_pool`]'s errors on failure.
    /// * Forwards [`mongodb::Client::with_uri_str`]'s errors on failure.
    pub async fn connect(config: &Config, credentials: &Credentials) -> Result<Self, ClientError> {
        if config.api_server.is_none() {
            return Err(ClientError::ConfigIsNone);
        }

        let sql_pool =
            create_clp_db_mysql_pool(&config.database, &credentials.database, 10).await?;

        let mongo_uri = format!(
            "mongodb://{}:{}/{}?directConnection=true",
            config.results_cache.host, config.results_cache.port, config.results_cache.db_name,
        );
        let mongo_client = mongodb::Client::with_uri_str(mongo_uri).await?;

        // Create a reusable S3 client if stream output is S3-backed.
        let s3_client = match &config.stream_output.storage {
            StreamOutputStorage::S3 { s3_config, .. } => {
                let region_str = s3_config
                    .region_code
                    .as_ref()
                    .map_or(AWS_DEFAULT_REGION, non_empty_string::NonEmptyString::as_str);
                Some(
                    clp_rust_utils::s3::create_new_client(
                        region_str,
                        s3_config.endpoint_url.as_ref(),
                        &s3_config.aws_authentication,
                    )
                    .await,
                )
            }
            StreamOutputStorage::Fs { .. } => None,
        };

        // Ensure the archive_time_metadata table exists.
        if let Err(e) = archive_metadata::create_archive_metadata_table(&sql_pool).await {
            tracing::warn!("Failed to create archive_time_metadata table: {e}");
        }

        Ok(Self {
            config: config.clone(),
            mongodb_client: mongo_client,
            sql_pool,
            s3_client,
            dedup_cache: Arc::new(QueryDedupCache::new()),
        })
    }

    /// Submits a search or aggregation query as a job.
    ///
    /// When time range filters are provided, the archive metadata index is consulted to prune
    /// archives whose time ranges do not overlap with the query window. The pruned set of
    /// archive IDs is attached to the job config so that workers only scan relevant archives.
    ///
    /// # Returns
    ///
    /// The unique ID of the newly created query job on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`rmp_serde::to_vec_named`]'s return values on failure.
    /// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
    pub async fn submit_query(&self, query_config: QueryConfig) -> Result<u64, ClientError> {
        // Check the dedup cache first: if an identical query was recently submitted,
        // return the existing job ID to avoid redundant archive scans.
        let fingerprint = QueryDedupCache::fingerprint(&query_config);
        if let Some(cached_job_id) = self.dedup_cache.get(&fingerprint) {
            // Verify the cached job hasn't failed/been cancelled.
            match self.get_status(cached_job_id).await {
                Ok(
                    QueryJobStatus::Pending
                    | QueryJobStatus::Running
                    | QueryJobStatus::Succeeded,
                ) => {
                    tracing::info!(
                        "Dedup cache hit: reusing job {} for identical query",
                        cached_job_id
                    );
                    return Ok(cached_job_id);
                }
                _ => {
                    // Job failed/cancelled/not found — fall through to create a new one.
                }
            }
        }

        let mut search_job_config: SearchJobConfig = query_config.clone().into();
        if search_job_config.datasets.is_none() {
            search_job_config.datasets = match self.config.package.storage_engine {
                StorageEngine::Clp => None,
                StorageEngine::ClpS => Some(vec!["default".to_owned()]),
            }
        }
        if search_job_config.max_num_results == 0 {
            search_job_config.max_num_results =
                self.get_api_server_config().default_max_num_query_results;
        }

        // Time-range pruning: query the archive metadata index to find only the archives
        // whose time ranges overlap with the search window.
        if query_config.time_range_begin_millisecs.is_some()
            || query_config.time_range_end_millisecs.is_some()
        {
            let datasets = search_job_config
                .datasets
                .clone()
                .unwrap_or_else(|| vec!["default".to_owned()]);

            match archive_metadata::query_overlapping_archives(
                &self.sql_pool,
                &datasets,
                query_config.time_range_begin_millisecs,
                query_config.time_range_end_millisecs,
            )
            .await
            {
                Ok(archive_ids) => {
                    if archive_ids.is_empty() {
                        tracing::info!(
                            "No archives overlap with the requested time range; \
                             submitting job anyway for backward compatibility"
                        );
                    } else {
                        tracing::info!(
                            "Time-range pruning: {} archives match the query window \
                             (out of potentially many more)",
                            archive_ids.len()
                        );
                        search_job_config.archive_ids_filter = Some(archive_ids);
                    }
                }
                Err(e) => {
                    // Non-fatal: if the metadata table doesn't exist or the query fails,
                    // fall back to scanning all archives.
                    tracing::warn!(
                        "Archive metadata pruning failed, falling back to full scan: {e}"
                    );
                }
            }
        }

        let query_job_type_i32: i32 = QueryJobType::SearchOrAggregation.into();
        let query_result = sqlx::query(&format!(
            "INSERT INTO `{QUERY_JOBS_TABLE_NAME}` (`job_config`, `type`) VALUES (?, ?)"
        ))
        .bind(rmp_serde::to_vec_named(&search_job_config)?)
        .bind(query_job_type_i32)
        .execute(&self.sql_pool)
        .await?;

        let search_job_id = query_result.last_insert_id();

        // Cache the query fingerprint → job ID mapping for deduplication.
        self.dedup_cache.insert(fingerprint, search_job_id);

        // Eagerly create MongoDB indexes on the result collection so that workers insert into
        // an indexed collection from the start, and the scheduler's found_max_num_latest_results
        // check can use the timestamp index instead of doing a collection scan.
        if !search_job_config.write_to_file {
            let collection_name = search_job_id.to_string();
            let db = self
                .mongodb_client
                .database(&self.config.results_cache.db_name);
            let collection = db.collection::<mongodb::bson::Document>(&collection_name);
            let ts_asc = mongodb::IndexModel::builder()
                .keys(mongodb::bson::doc! { "timestamp": 1, "_id": 1 })
                .options(
                    mongodb::options::IndexOptions::builder()
                        .name("timestamp-ascending".to_owned())
                        .build(),
                )
                .build();
            let ts_desc = mongodb::IndexModel::builder()
                .keys(mongodb::bson::doc! { "timestamp": -1, "_id": -1 })
                .options(
                    mongodb::options::IndexOptions::builder()
                        .name("timestamp-descending".to_owned())
                        .build(),
                )
                .build();
            if let Err(e) = collection.create_indexes(vec![ts_asc, ts_desc]).await {
                tracing::warn!(
                    "Failed to eagerly create MongoDB indexes for job {}: {}",
                    search_job_id,
                    e
                );
            } else {
                tracing::info!(
                    "Created MongoDB timestamp indexes for job {}",
                    search_job_id
                );
            }
        }

        Ok(search_job_id)
    }

    /// Asynchronously fetches the results of a completed search job.
    ///
    /// # Returns
    ///
    /// A stream of the job's results on success. The stream variant indicates where the results
    /// are stored:
    ///
    /// * [`SearchResultStream::File`] if the job's results are stored in files.
    /// * [`SearchResultStream::Mongo`] if the job's results are stored in `MongoDB`.
    /// * [`SearchResultStream::S3`] if the job's results are stored in S3.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Client::get_status`]'s return values on failure.
    /// * Forwards [`Client::get_job_config`]'s return values on failure.
    /// * Forwards [`Client::fetch_results_from_mongo`]'s return values on failure.
    /// * Forwards [`Client::fetch_results_from_s3`]'s return values on failure.
    pub async fn fetch_results(
        &self,
        search_job_id: u64,
    ) -> Result<
        SearchResultStream<
            impl Stream<Item = Result<String, ClientError>> + use<>,
            impl Stream<Item = Result<String, ClientError>> + use<>,
            impl Stream<Item = Result<String, ClientError>> + use<>,
        >,
        ClientError,
    > {
        let api_server_config = self.get_api_server_config();
        let mut delay_ms = api_server_config.query_job_polling.initial_backoff_ms;
        let max_delay_ms = api_server_config.query_job_polling.max_backoff_ms;
        loop {
            match self.get_status(search_job_id).await? {
                QueryJobStatus::Succeeded => {
                    break;
                }
                QueryJobStatus::Failed
                | QueryJobStatus::Cancelled
                | QueryJobStatus::Killed
                | QueryJobStatus::Cancelling => {
                    return Err(ClientError::QueryNotSucceeded);
                }
                QueryJobStatus::Running | QueryJobStatus::Pending => {
                    tokio::time::sleep(tokio::time::Duration::from_millis(delay_ms)).await;
                    delay_ms = std::cmp::min(delay_ms.saturating_mul(2), max_delay_ms);
                }
            }
        }

        let job_config = self.get_job_config(search_job_id).await?;

        if job_config.write_to_file {
            let stream = match &self.config.stream_output.storage {
                StreamOutputStorage::Fs { .. } => SearchResultStream::File {
                    inner: self.fetch_results_from_file(search_job_id),
                },
                StreamOutputStorage::S3 { .. } => SearchResultStream::S3 {
                    inner: self.fetch_results_from_s3(search_job_id).await?,
                },
            };
            return Ok(stream);
        }

        self.fetch_results_from_mongo(search_job_id)
            .await
            .map(|s| SearchResultStream::Mongo { inner: s })
    }

    /// Submits a cancellation request for a search job.
    ///
    /// # Returns
    ///
    /// `Ok(())` on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::SearchJobNotFound`] if no matching job was found (e.g., the job doesn't
    ///   exist or is not in a cancellable state).
    /// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
    pub async fn cancel_search_job(&self, search_job_id: u64) -> Result<(), ClientError> {
        let result = sqlx::query(&format!(
            "UPDATE `{QUERY_JOBS_TABLE_NAME}` SET status = ? WHERE id = ? AND status IN (?, ?)"
        ))
        .bind::<i32>(QueryJobStatus::Cancelling.into())
        .bind(search_job_id)
        .bind::<i32>(QueryJobStatus::Pending.into())
        .bind::<i32>(QueryJobStatus::Running.into())
        .execute(&self.sql_pool)
        .await?;

        if result.rows_affected() == 0 {
            return Err(ClientError::SearchJobNotFound(search_job_id));
        }

        Ok(())
    }

    /// Retrieves the status of a previously submitted search job.
    ///
    /// # Returns
    ///
    /// The current status of the job on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::query::Query::fetch_one`]'s return values on failure.
    /// * Forwards [`sqlx::Row::try_get`]'s return values on failure.
    /// * Forwards [`QueryJobStatus::try_from`]'s return values on failure.
    pub async fn get_status(&self, search_job_id: u64) -> Result<QueryJobStatus, ClientError> {
        let row = sqlx::query(&format!(
            "SELECT status FROM `{QUERY_JOBS_TABLE_NAME}` WHERE id = ?"
        ))
        .bind(search_job_id)
        .fetch_one(&self.sql_pool)
        .await?;
        let status: i32 = row.try_get("status")?;
        Ok(QueryJobStatus::try_from(status)?)
    }

    async fn get_job_config(&self, search_job_id: u64) -> Result<SearchJobConfig, ClientError> {
        let row = sqlx::query(&format!(
            "SELECT job_config FROM `{QUERY_JOBS_TABLE_NAME}` WHERE id = ?"
        ))
        .bind(search_job_id)
        .fetch_one(&self.sql_pool)
        .await?;
        let msgpack: &[u8] = row.try_get("job_config")?;
        Ok(rmp_serde::from_slice(msgpack)?)
    }

    /// Asynchronously fetches results of a completed search job from files.
    fn fetch_results_from_file(
        &self,
        search_job_id: u64,
    ) -> impl Stream<Item = Result<String, ClientError>> + use<> {
        let StreamOutputStorage::Fs { directory } = &self.config.stream_output.storage else {
            unreachable!();
        };
        let search_job_output_dir = std::path::Path::new("/")
            .join(directory)
            .join(search_job_id.to_string());
        let stream = stream! {
            let mut entries = tokio::fs::read_dir(search_job_output_dir).await?;
            while let Some(entry) = entries.next_entry().await? {
                let search_result_path = entry.path();
                let reader = std::fs::File::open(search_result_path)?;
                let mut deserializer = rmp_serde::Deserializer::new(reader);
                while let Ok(event) = Deserialize::deserialize(&mut deserializer) {
                    let event: (i64, String, String, String, i64) = event;
                    yield Ok(event.1);
                }
            }
        };
        stream
    }

    /// Asynchronously fetches results of a completed search job from S3 using the reusable
    /// S3 client and concurrent object fetches via `buffer_unordered`.
    async fn fetch_results_from_s3(
        &self,
        search_job_id: u64,
    ) -> Result<impl Stream<Item = Result<String, ClientError>> + use<>, ClientError> {
        tracing::info!("Streaming results from S3 (concurrent fetch)");
        let StreamOutputStorage::S3 { s3_config, .. } = &self.config.stream_output.storage else {
            unreachable!();
        };

        if s3_config.region_code.is_none() && s3_config.endpoint_url.is_none() {
            return Err(ClientError::Aws {
                description: "a region code must be given when using the default AWS S3 endpoint"
                    .to_owned(),
            });
        }

        // Use the reusable S3 client created at connect time.
        let s3_client = self.s3_client.clone().expect(
            "S3 client must be initialized when stream output storage is S3",
        );

        let bucket = s3_config.bucket.to_string();
        let key_prefix = format!("{}{}/", s3_config.key_prefix, search_job_id);
        tracing::info!("Streaming results from S3 prefix: {}", key_prefix);

        // Phase 1: Collect all object keys from the S3 listing.
        let mut all_keys: Vec<String> = Vec::new();
        let mut object_pages = s3_client
            .list_objects_v2()
            .bucket(&bucket)
            .prefix(&key_prefix)
            .into_paginator()
            .send();

        while let Some(object_page) = object_pages.next().await {
            for object in object_page?.contents() {
                if let Some(key) = object.key() {
                    if !key.ends_with('/') {
                        all_keys.push(key.to_owned());
                    }
                }
            }
        }

        tracing::info!(
            "Found {} result objects for job {}; fetching concurrently",
            all_keys.len(),
            search_job_id
        );

        // Phase 2: Fetch objects concurrently using buffer_unordered.
        let bucket_clone = bucket.clone();
        let result_stream = futures_stream::iter(all_keys)
            .map(move |key| {
                let client = s3_client.clone();
                let b = bucket_clone.clone();
                async move {
                    let obj = client
                        .get_object()
                        .bucket(&b)
                        .key(&key)
                        .send()
                        .await
                        .map_err(|e| ClientError::Aws {
                            description: e.to_string(),
                        })?;
                    let bytes = obj.body.collect().await.map_err(|e| ClientError::Aws {
                        description: e.to_string(),
                    })?;
                    Ok::<_, ClientError>(bytes.into_bytes())
                }
            })
            .buffer_unordered(DEFAULT_S3_FETCH_CONCURRENCY);

        // Phase 3: Deserialize each fetched object's bytes into log events.
        let event_stream = result_stream.flat_map(|result| {
            futures_stream::iter(match result {
                Err(e) => vec![Err(e)],
                Ok(bytes) => {
                    let mut events = Vec::new();
                    let mut de = rmp_serde::Deserializer::from_read_ref(bytes.as_ref());
                    while let Ok(event) = Deserialize::deserialize(&mut de) {
                        let event: (i64, String, String, String, i64) = event;
                        events.push(Ok(event.1));
                    }
                    events
                }
            })
        });

        Ok(event_stream)
    }

    /// Asynchronously fetches results of a completed search job from `MongoDB`.
    async fn fetch_results_from_mongo(
        &self,
        search_job_id: u64,
    ) -> Result<impl Stream<Item = Result<String, ClientError>> + use<>, ClientError> {
        let database = self
            .mongodb_client
            .database(&self.config.results_cache.db_name);
        let collection: mongodb::Collection<mongodb::bson::Document> =
            database.collection(&search_job_id.to_string());
        let cursor = collection.find(mongodb::bson::doc! {}).await?;

        let mapped = cursor.map(|res| {
            let doc = res?;
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

    /// Retrieves timestamp column names for a given dataset.
    pub async fn get_timestamp_column_names(
        &self,
        dataset_name: &str,
    ) -> Result<Vec<String>, ClientError> {
        const TIMESTAMP_NODE_TYPE: i8 = 14;
        const MYSQL_TABLE_NOT_FOUND: u16 = 1146;

        if !clp_rust_utils::dataset::VALID_DATASET_NAME_REGEX.is_match(dataset_name) {
            return Err(ClientError::InvalidDatasetName);
        }
        let table_name = format!("clp_{dataset_name}_column_metadata");
        let names: Vec<String> =
            sqlx::query_scalar(&format!("SELECT name FROM `{table_name}` WHERE type = ?"))
                .bind(TIMESTAMP_NODE_TYPE)
                .fetch_all(&self.sql_pool)
                .await
                .map_err(|err| {
                    if let sqlx::Error::Database(db_err) = &err
                        && let Some(mysql_err) =
                            db_err.try_downcast_ref::<sqlx::mysql::MySqlDatabaseError>()
                        && mysql_err.number() == MYSQL_TABLE_NOT_FOUND
                    {
                        return ClientError::DatasetNotFound(dataset_name.to_owned());
                    }
                    err.into()
                })?;

        Ok(names)
    }

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

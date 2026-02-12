use std::pin::Pin;

use async_stream::stream;
use clp_rust_utils::{
    aws::AWS_DEFAULT_REGION,
    clp_config::{
        AwsAuthentication,
        package::{
            config::{Config, StorageEngine, StreamOutputStorage},
            credentials::Credentials,
        },
    },
    database::mysql::create_clp_db_mysql_pool,
    job_config::{QUERY_JOBS_TABLE_NAME, QueryJobStatus, QueryJobType, SearchJobConfig},
};
use futures::{Stream, StreamExt};
use pin_project_lite::pin_project;
use serde::{Deserialize, Serialize};
use sqlx::Row;
use utoipa::ToSchema;

pub use crate::error::ClientError;

/// Defines the request configuration for submitting a search query.
#[derive(Clone, Debug, Serialize, Deserialize, ToSchema)]
#[serde(deny_unknown_fields)]
pub struct QueryConfig {
    /// The search query as a KQL string.
    pub query_string: String,

    /// The dataset to search within. If not provided, only `default` dataset will be searched.
    #[serde(default)]
    pub dataset: Option<String>,

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
            dataset: value.dataset,
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
}

impl Client {
    /// Factory method to create a new client with active connections to both `MySQL` and `MongoDB`
    /// databases.
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

        Ok(Self {
            config: config.clone(),
            mongodb_client: mongo_client,
            sql_pool,
        })
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
    /// * Forwards [`rmp_serde::to_vec_named`]'s return values on failure.
    /// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
    pub async fn submit_query(&self, query_config: QueryConfig) -> Result<u64, ClientError> {
        let mut search_job_config: SearchJobConfig = query_config.into();
        if search_job_config.dataset.is_none() {
            search_job_config.dataset = match self.config.package.storage_engine {
                StorageEngine::Clp => None,
                StorageEngine::ClpS => Some("default".to_owned()),
            }
        }
        if search_job_config.max_num_results == 0 {
            search_job_config.max_num_results =
                self.get_api_server_config().default_max_num_query_results;
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
    /// * Forwards [`std::fs::File::open`]'s return values on failure.
    /// * Forwards [`tokio::fs::read_dir`]'s return values on failure.
    /// * Forwards [`tokio::fs::ReadDir::next_entry`]'s return values on failure.
    ///
    /// # Panics
    ///
    /// Panics if the stream output storage is not file system.
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

    /// Asynchronously fetches results of a completed search job from S3.
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
    /// Return an error if:
    ///
    /// * [`ClientError::Aws`] if a region code is not provided when using the default AWS S3
    ///   endpoint.
    ///
    /// # Panics
    ///
    /// Panics if the stream output storage is not S3.
    async fn fetch_results_from_s3(
        &self,
        search_job_id: u64,
    ) -> Result<impl Stream<Item = Result<String, ClientError>> + use<>, ClientError> {
        tracing::info!("Streaming results from S3");
        let StreamOutputStorage::S3 { s3_config, .. } = &self.config.stream_output.storage else {
            unreachable!();
        };

        let AwsAuthentication::Credentials { credentials } = &s3_config.aws_authentication;

        let s3_config = s3_config.clone();
        if s3_config.region_code.is_none() && s3_config.endpoint_url.is_none() {
            return Err(ClientError::Aws {
                description: "a region code must be given when using the default AWS S3 endpoint"
                    .to_owned(),
            });
        }

        let credentials = credentials.clone();
        let s3_client = clp_rust_utils::s3::create_new_client(
            credentials.access_key_id.as_str(),
            credentials.secret_access_key.as_str(),
            s3_config
                .region_code
                .as_ref()
                .map_or(AWS_DEFAULT_REGION, non_empty_string::NonEmptyString::as_str),
            s3_config.endpoint_url.as_ref(),
        )
        .await;

        let key_prefix = format!("{}{}/", s3_config.key_prefix, search_job_id);
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

    /// Asynchronously fetches results of a completed search job from `MongoDB`.
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
    /// * Forwards [`serde_json::from_str`]'s return values on failure.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`mongodb::Collection::find`]'s return values on failure.
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

//! Retrieval of the results of completed query jobs.

use std::num::NonZeroU32;
use std::pin::Pin;

use async_stream::stream;
use clp_rust_utils::aws::AWS_DEFAULT_REGION;
use clp_rust_utils::clp_config::package::config::StreamOutputStorage;
use clp_rust_utils::job_config::QueryOutput;
use clp_rust_utils::job_config::ResultsCacheOutput;
use futures::Stream;
use futures::StreamExt;
use pin_project_lite::pin_project;
use serde::Deserialize;

use crate::client::Client;
use crate::error::ClientError;

impl Client {
    /// Asynchronously fetches the results of a completed query job.
    ///
    /// # Returns
    ///
    /// A stream of the job's results on success. The stream variant indicates where the results are
    /// stored:
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
    /// When `raw_docs` is `true`, each search document is serialized to JSON whole; otherwise the
    /// document's "message" field is extracted. Timeline records are excluded.
    ///
    /// When `sorted` is `true`, documents are returned sorted by `timestamp` descending (then `_id`
    /// ascending); otherwise, they're returned in insertion order.
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

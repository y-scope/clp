//! Extraction of stream files for the `WebUI`'s log viewer.

use std::time::Duration;

use aws_sdk_s3::presigning::PresigningConfig;
use clp_rust_utils::aws::AWS_DEFAULT_REGION;
use clp_rust_utils::clp_config::package::config::ApiServer;
use clp_rust_utils::clp_config::package::config::StorageEngine;
use clp_rust_utils::clp_config::package::config::StreamOutputStorage;
use clp_rust_utils::dataset::DatasetName;
use clp_rust_utils::job_config::ExtractJsonJobConfig;
use clp_rust_utils::job_config::QUERY_JOBS_TABLE_NAME;
use clp_rust_utils::job_config::QueryJobStatus;
use clp_rust_utils::job_config::QueryJobType;
use clp_rust_utils::types::ArchiveId;
use mongodb::bson::doc;
use non_empty_string::NonEmptyString;
use serde::Deserialize;
use serde::Serialize;
use utoipa::ToSchema;

use crate::error::ClientError;
use crate::webui_client::WebuiClient;

/// Request body for the stream-file extraction endpoint.
#[derive(Debug, Deserialize, ToSchema)]
#[serde(deny_unknown_fields)]
pub struct StreamFileExtraction {
    /// The dataset of the archive to extract the stream from. Defaults to the default dataset when
    /// absent or `null`.
    #[schema(value_type = Option<String>)]
    pub dataset: DatasetName,
    /// Index of the log event that the extracted stream file must contain.
    pub log_event_idx: u64,
    /// ID of the archive to extract the JSON stream from.
    #[schema(value_type = String, format = "uuid")]
    pub stream_id: ArchiveId,
}

/// The metadata of an extracted stream file.
#[derive(Debug, Deserialize, Serialize, ToSchema)]
pub struct StreamFileMetadata {
    /// Index of the first log event in the stream file (inclusive).
    pub begin_msg_ix: i64,
    /// Index after the last log event in the stream file (exclusive).
    pub end_msg_ix: i64,
    /// Whether this is the stream's last chunk.
    pub is_last_chunk: bool,
    /// Where the `WebUI` fetches the stream file from: a pre-signed URL that's valid for one hour
    /// when stream output is stored on S3, or a path under the `WebUI`'s `/streams` static mount
    /// otherwise.
    pub path: String,
    /// ID of the stream the file was extracted from.
    pub stream_id: String,
}

impl WebuiClient {
    /// Extracts a JSON stream file that contains the log event at `log_event_idx` in the archive
    /// with ID `stream_id`.
    ///
    /// If the results cache already has such a stream file, its metadata is returned directly.
    /// Otherwise, an extraction job is submitted, and the stream file is looked up again once the
    /// job finishes. The wait is bounded by `api_server.stream_file_extraction_timeout_secs`; the
    /// job keeps running when the timeout is reached.
    ///
    /// # Returns
    ///
    /// The stream file's metadata on success, with its `path` resolved to where the `WebUI` fetches
    /// the stream file from (see [`StreamFileMetadata::path`]).
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::InvalidInput`] if:
    ///   * the package's storage engine isn't clp-s.
    ///   * `log_event_idx` exceeds `i64::MAX`.
    /// * [`ClientError::StreamExtractionTimeout`] if the extraction job doesn't finish within the
    ///   configured timeout.
    /// * [`ClientError::LogEventNotFound`] if the extraction job succeeds without extracting a
    ///   stream file that contains the log event.
    /// * Forwards [`mongodb::Collection::find_one`]'s return values on failure.
    /// * Forwards [`Self::submit_and_wait_for_extraction_job`]'s return values on failure.
    /// * Forwards [`StreamFileStorage::resolve_path`]'s return values on failure.
    ///
    /// # Panics
    ///
    /// Panics if the results cache has no such stream file and `self.config.api_server` is `None`.
    pub async fn extract_stream_file(
        &self,
        extraction: StreamFileExtraction,
    ) -> Result<StreamFileMetadata, ClientError> {
        if self.config.package.storage_engine != StorageEngine::ClpS {
            return Err(ClientError::InvalidInput(
                "stream extraction requires the clp-s storage engine".to_owned(),
            ));
        }
        let log_event_idx = i64::try_from(extraction.log_event_idx).map_err(|_| {
            ClientError::InvalidInput("log_event_idx must not exceed i64::MAX".to_owned())
        })?;
        let stream_files = self
            .mongodb_client
            .database(&self.config.results_cache.db_name)
            .collection::<StreamFileMetadata>(&self.config.results_cache.stream_collection_name);
        let filter = doc! {
            "stream_id": extraction.stream_id.to_string(),
            "begin_msg_ix": {"$lte": log_event_idx},
            "end_msg_ix": {"$gt": log_event_idx},
        };

        let mut metadata = if let Some(metadata) = stream_files.find_one(filter.clone()).await? {
            metadata
        } else {
            let timeout_secs = self
                .get_api_server_config()
                .stream_file_extraction_timeout_secs
                .get();
            let job_config = ExtractJsonJobConfig {
                dataset: extraction.dataset,
                archive_id: extraction.stream_id,
                target_chunk_size: Some(self.config.stream_output.target_uncompressed_size.get()),
            };
            tokio::time::timeout(
                Duration::from_secs(timeout_secs),
                self.submit_and_wait_for_extraction_job(&job_config),
            )
            .await
            .map_err(|_| ClientError::StreamExtractionTimeout(timeout_secs))??;
            stream_files
                .find_one(filter)
                .await?
                .ok_or(ClientError::LogEventNotFound {
                    archive_id: extraction.stream_id,
                    log_event_idx: extraction.log_event_idx,
                })?
        };

        metadata.path = self
            .stream_file_storage
            .resolve_path(&metadata.path)
            .await?;
        Ok(metadata)
    }

    /// Submits a JSON extraction job to the query jobs table, and polls its status until it
    /// finishes.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::ExtractionJobNotFound`] if the job disappears from the query jobs table
    ///   before it finishes.
    /// * [`ClientError::ExtractionJobFailed`] if the job fails, is killed, or is cancelled.
    /// * Forwards [`rmp_serde::to_vec_named`]'s return values on failure.
    /// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
    /// * Forwards [`sqlx::query::QueryScalar::fetch_optional`]'s return values on failure.
    ///
    /// # Panics
    ///
    /// Panics if `self.config.api_server` is `None`.
    async fn submit_and_wait_for_extraction_job(
        &self,
        job_config: &ExtractJsonJobConfig,
    ) -> Result<(), ClientError> {
        let job_id = sqlx::query(&format!(
            "INSERT INTO `{QUERY_JOBS_TABLE_NAME}` (`job_config`, `type`) VALUES (?, ?)"
        ))
        .bind(rmp_serde::to_vec_named(job_config)?)
        .bind(QueryJobType::ExtractJson)
        .execute(&self.sql_pool)
        .await?
        .last_insert_id();

        let polling = &self.get_api_server_config().query_job_polling;
        let mut delay_ms = polling.initial_backoff_ms.max(1);
        let max_delay_ms = polling.max_backoff_ms.max(delay_ms);
        loop {
            let status = sqlx::query_scalar::<_, QueryJobStatus>(&format!(
                "SELECT `status` FROM `{QUERY_JOBS_TABLE_NAME}` WHERE `id` = ?"
            ))
            .bind(job_id)
            .fetch_optional(&self.sql_pool)
            .await?
            .ok_or(ClientError::ExtractionJobNotFound(job_id))?;
            match status {
                QueryJobStatus::Succeeded => return Ok(()),
                QueryJobStatus::Pending
                | QueryJobStatus::Running
                | QueryJobStatus::CancelRequested
                | QueryJobStatus::Cancelling => {
                    tokio::time::sleep(Duration::from_millis(delay_ms)).await;
                    delay_ms = std::cmp::min(delay_ms.saturating_mul(2), max_delay_ms);
                }
                QueryJobStatus::Failed | QueryJobStatus::Killed | QueryJobStatus::Cancelled => {
                    return Err(ClientError::ExtractionJobFailed { job_id, status });
                }
            }
        }
    }

    /// # Returns
    ///
    /// A reference to the API server configuration.
    ///
    /// # Panics
    ///
    /// Panics if `self.config.api_server` is `None`.
    const fn get_api_server_config(&self) -> &ApiServer {
        self.config
            .api_server
            .as_ref()
            .expect("api_server configuration is missing")
    }
}

/// Where extracted stream files are stored.
#[derive(Clone)]
pub(super) enum StreamFileStorage {
    Fs,
    S3 {
        client: aws_sdk_s3::Client,
        bucket: NonEmptyString,
        key_prefix: NonEmptyString,
    },
}

impl StreamFileStorage {
    /// Factory function.
    ///
    /// Creates the storage of the given stream output. For S3 storage, also creates the S3 client
    /// that pre-signs the stream files' URLs.
    ///
    /// # Returns
    ///
    /// The newly created storage.
    pub(super) async fn from_config(stream_output_storage: &StreamOutputStorage) -> Self {
        match stream_output_storage {
            StreamOutputStorage::Fs { .. } => Self::Fs,
            StreamOutputStorage::S3 { s3_config, .. } => Self::S3 {
                client: clp_rust_utils::s3::create_new_client(
                    s3_config
                        .region_code
                        .as_ref()
                        .map_or(AWS_DEFAULT_REGION, NonEmptyString::as_str),
                    s3_config.endpoint_url.as_ref(),
                    &s3_config.aws_authentication,
                )
                .await,
                bucket: s3_config.bucket.clone(),
                key_prefix: s3_config.key_prefix.clone(),
            },
        }
    }

    /// Resolves a stream file's path in the results cache to where the `WebUI` fetches the stream
    /// file from.
    ///
    /// # Returns
    ///
    /// On success:
    ///
    /// * For S3 storage, a pre-signed GET URL of the object at `path` under the key prefix, valid
    ///   for one hour.
    /// * For filesystem storage, `path` under the `WebUI`'s `/streams` static mount.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`GetObjectFluentBuilder::presigned`]'s return values on failure.
    ///
    /// [`GetObjectFluentBuilder::presigned`]:
    ///   aws_sdk_s3::operation::get_object::builders::GetObjectFluentBuilder::presigned
    async fn resolve_path(&self, path: &str) -> Result<String, ClientError> {
        const PRESIGNED_URL_EXPIRY: Duration = Duration::from_secs(3600);

        match self {
            Self::Fs => Ok(format!("/streams/{path}")),
            Self::S3 {
                client,
                bucket,
                key_prefix,
            } => {
                let presigning_config = PresigningConfig::expires_in(PRESIGNED_URL_EXPIRY)
                    .expect("one hour should be a valid pre-signed URL expiry");
                let request = client
                    .get_object()
                    .bucket(bucket.as_str())
                    .key(format!("{key_prefix}{path}"))
                    .presigned(presigning_config)
                    .await?;
                Ok(request.uri().to_owned())
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use clp_rust_utils::aws::AWS_DEFAULT_REGION;
    use clp_rust_utils::clp_config::AwsAuthentication;
    use clp_rust_utils::clp_config::AwsCredentials;
    use clp_rust_utils::clp_config::S3Config;
    use clp_rust_utils::clp_config::package::config::Config;
    use clp_rust_utils::clp_config::package::config::Package;
    use clp_rust_utils::clp_config::package::config::StorageEngine;
    use clp_rust_utils::clp_config::package::config::StreamOutput;
    use clp_rust_utils::clp_config::package::config::StreamOutputStorage;
    use clp_rust_utils::dataset::DatasetName;
    use clp_rust_utils::types::non_empty_string::ExpectedNonEmpty;
    use non_empty_string::NonEmptyString;

    use super::StreamFileExtraction;
    use super::StreamFileStorage;
    use crate::webui_client::WebuiClient;

    #[tokio::test]
    async fn extraction_is_rejected_under_the_clp_storage_engine() -> anyhow::Result<()> {
        let config = Config {
            package: Package {
                storage_engine: StorageEngine::Clp,
            },
            ..Config::default()
        };
        let sql_pool =
            sqlx::mysql::MySqlPoolOptions::new().connect_lazy("mysql://127.0.0.1:1/clp-db")?;
        let mongodb_client =
            mongodb::Client::with_uri_str("mongodb://127.0.0.1:1/?serverSelectionTimeoutMS=100")
                .await?;
        let client = WebuiClient::new(config, sql_pool, mongodb_client).await;

        let error = client
            .extract_stream_file(StreamFileExtraction {
                dataset: DatasetName::default(),
                log_event_idx: 0,
                stream_id: "3f1c1a5e-5b6e-4a43-9d6b-0f2b2f5a8c11".parse()?,
            })
            .await
            .expect_err("the extraction should be rejected");
        assert_eq!(
            error.to_string(),
            "Invalid input: stream extraction requires the clp-s storage engine"
        );
        Ok(())
    }

    #[tokio::test]
    async fn fs_stream_files_resolve_under_the_streams_mount() -> anyhow::Result<()> {
        let storage = StreamFileStorage::from_config(&StreamOutputStorage::Fs {
            directory: "/var/data/streams".to_owned(),
        })
        .await;
        assert_eq!(
            storage.resolve_path("archive/stream.jsonl").await?,
            "/streams/archive/stream.jsonl"
        );
        Ok(())
    }

    #[tokio::test]
    async fn s3_stream_files_resolve_to_a_presigned_url_under_the_key_prefix() -> anyhow::Result<()>
    {
        let config = Config {
            stream_output: StreamOutput {
                storage: StreamOutputStorage::S3 {
                    staging_directory: "/var/data/staged-streams".to_owned(),
                    s3_config: S3Config {
                        bucket: NonEmptyString::from_static_str("stream-bucket"),
                        region_code: None,
                        key_prefix: NonEmptyString::from_static_str("streams/"),
                        endpoint_url: Some(NonEmptyString::from_static_str(
                            "http://127.0.0.1:9000",
                        )),
                        aws_authentication: AwsAuthentication::Credentials {
                            credentials: AwsCredentials {
                                access_key_id: "access-key-id".to_owned(),
                                secret_access_key: "secret-access-key".to_owned(),
                                session_token: None,
                            },
                        },
                    },
                },
                ..StreamOutput::default()
            },
            ..Config::default()
        };
        let sql_pool =
            sqlx::mysql::MySqlPoolOptions::new().connect_lazy("mysql://localhost/clp-db")?;
        let mongodb_client = mongodb::Client::with_uri_str("mongodb://localhost:27017").await?;
        let client = WebuiClient::new(config, sql_pool, mongodb_client).await;

        let url = client
            .stream_file_storage
            .resolve_path("archive/stream.jsonl")
            .await?;
        assert!(
            url.starts_with("http://127.0.0.1:9000/stream-bucket/streams/archive/stream.jsonl?"),
            "unexpected URL: {url}"
        );
        assert!(url.contains("X-Amz-Expires=3600"), "unexpected URL: {url}");
        assert!(
            url.contains("X-Amz-Credential=access-key-id%2F"),
            "unexpected URL: {url}"
        );
        assert!(
            url.contains(&format!("%2F{AWS_DEFAULT_REGION}%2Fs3%2Faws4_request")),
            "unexpected URL: {url}"
        );
        Ok(())
    }
}

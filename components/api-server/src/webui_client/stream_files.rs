//! Extraction of stream files for the log viewer.

use std::num::NonZeroU64;
use std::time::Duration;

use aws_sdk_s3::presigning::PresigningConfig;
use clp_rust_utils::aws::AWS_DEFAULT_REGION;
use clp_rust_utils::clp_config::package::config::ApiServer;
use clp_rust_utils::clp_config::package::config::QueryJobPollingConfig;
use clp_rust_utils::clp_config::package::config::StreamOutputStorage;
use clp_rust_utils::dataset::DatasetName;
use clp_rust_utils::job_config::ExtractIrJobConfig;
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

use super::WebuiClient;
use crate::error::ClientError;

/// Request body for the stream-files extract endpoint.
#[derive(Clone, Debug, Deserialize, ToSchema)]
#[serde(deny_unknown_fields)]
pub struct StreamFileExtraction {
    /// Dataset the stream belongs to (CLP-S only; `null` for the CLP storage engine). Defaults to
    /// the default dataset.
    #[serde(default)]
    pub dataset: Option<String>,
    /// The type of extraction job to submit.
    #[schema(schema_with = extract_job_type_schema)]
    pub extract_job_type: QueryJobType,
    /// Index of the log event the extracted stream file must contain.
    pub log_event_idx: i64,
    /// ID of the stream to extract.
    #[schema(min_length = 1)]
    pub stream_id: String,
}

impl StreamFileExtraction {
    /// Builds the config of the extraction job that extracts the requested stream.
    ///
    /// # Returns
    ///
    /// The job config on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::InvalidDatasetName`] if the dataset name is invalid.
    /// * [`ClientError::InvalidInput`] if:
    ///   * `stream_id` is empty.
    ///   * `extract_job_type` isn't an extraction job type.
    ///   * `log_event_idx` is negative for an IR extraction.
    ///   * `stream_id` isn't an archive ID for a JSON extraction.
    fn job_config(&self, target_uncompressed_size: u64) -> Result<ExtractJobConfig, ClientError> {
        let dataset = DatasetName::try_from(self.dataset.clone())?;
        let Ok(stream_id) = NonEmptyString::new(self.stream_id.clone()) else {
            return Err(ClientError::InvalidInput(
                "stream_id must not be empty".to_owned(),
            ));
        };
        match self.extract_job_type {
            QueryJobType::ExtractIr => {
                let msg_ix = u64::try_from(self.log_event_idx).map_err(|_| {
                    ClientError::InvalidInput("log_event_idx must not be negative".to_owned())
                })?;
                Ok(ExtractJobConfig::Ir(ExtractIrJobConfig {
                    orig_file_id: stream_id,
                    msg_ix,
                    file_split_id: None,
                    target_uncompressed_size: Some(target_uncompressed_size),
                }))
            }
            QueryJobType::ExtractJson => {
                let archive_id = stream_id
                    .as_str()
                    .parse::<ArchiveId>()
                    .map_err(|err| ClientError::InvalidInput(err.to_string()))?;
                Ok(ExtractJobConfig::Json(ExtractJsonJobConfig {
                    dataset,
                    archive_id,
                    target_chunk_size: Some(target_uncompressed_size),
                }))
            }
            QueryJobType::SearchOrAggregation => Err(ClientError::InvalidInput(
                "extract_job_type must be `ExtractIr` or `ExtractJson`".to_owned(),
            )),
        }
    }
}

/// Extracted stream-file metadata returned by the stream-files extract endpoint.
#[derive(Clone, Debug, Serialize, Deserialize, ToSchema)]
pub struct StreamFileMetadata {
    /// Index of the first log event in the stream file (inclusive).
    pub begin_msg_ix: i64,
    /// Index of the last log event in the stream file (exclusive).
    pub end_msg_ix: i64,
    /// Whether this is the stream's last chunk.
    pub is_last_chunk: bool,
    /// The resolved stream-file path: a pre-signed URL when stream-files S3 storage is
    /// configured, otherwise a path relative to the webui `/streams` static mount.
    pub path: String,
    /// ID of the stream the file was extracted from.
    pub stream_id: String,
}

/// The stream-output storage and the settings that stream-file extraction uses.
#[derive(Clone)]
pub struct StreamFileExtractionSettings {
    storage: StreamFileStorage,
    query_job_polling: QueryJobPollingConfig,
    timeout_secs: NonZeroU64,
}

impl StreamFileExtractionSettings {
    /// Factory function.
    ///
    /// Creates the settings from the API server's config and the stream-output storage. For S3
    /// storage, also creates the S3 client that pre-signs the stream files' URLs.
    ///
    /// # Returns
    ///
    /// The newly created settings.
    pub async fn from_config(
        api_server_config: &ApiServer,
        stream_output_storage: &StreamOutputStorage,
    ) -> Self {
        let storage = match stream_output_storage {
            StreamOutputStorage::Fs { .. } => StreamFileStorage::Fs,
            StreamOutputStorage::S3 { s3_config, .. } => StreamFileStorage::S3 {
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
        };
        Self {
            storage,
            query_job_polling: api_server_config.query_job_polling.clone(),
            timeout_secs: api_server_config.stream_file_extraction_timeout_secs,
        }
    }
}

impl WebuiClient {
    /// Extracts a stream file containing the log event at `log_event_idx` in the stream with
    /// the given `stream_id`. If the stream has already been extracted, returns its metadata
    /// directly; otherwise submits an extraction job and waits for it to complete.
    ///
    /// # Returns
    ///
    /// The extracted stream file's metadata on success. Its `path` is the resolved stream-file
    /// path: a pre-signed URL when stream-files S3 storage is configured, otherwise a path
    /// relative to the webui `/streams` static mount.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::StreamExtractionTimeout`] if the extraction job doesn't finish within the
    ///   configured timeout.
    /// * [`ClientError::InvalidInput`] if the extraction job finishes without producing a stream
    ///   file that contains the log event.
    /// * Forwards [`StreamFileExtraction::job_config`]'s return values on failure.
    /// * Forwards [`mongodb::Collection::find_one`]'s return values on failure.
    /// * Forwards [`Self::submit_and_wait_extract_job`]'s return values on failure.
    /// * Forwards [`StreamFileStorage::resolve_path`]'s return values on failure.
    pub async fn extract_stream_file(
        &self,
        extraction: StreamFileExtraction,
    ) -> Result<StreamFileMetadata, ClientError> {
        let job_config =
            extraction.job_config(self.config.stream_output.target_uncompressed_size)?;
        let stream_files_collection = self
            .mongodb_client
            .database(&self.config.results_cache.db_name)
            .collection::<StreamFileMetadata>(&self.config.results_cache.stream_collection_name);
        let filter = doc! {
            "stream_id": &extraction.stream_id,
            "begin_msg_ix": {"$lte": extraction.log_event_idx},
            "end_msg_ix": {"$gt": extraction.log_event_idx},
        };

        let mut metadata =
            if let Some(metadata) = stream_files_collection.find_one(filter.clone()).await? {
                metadata
            } else {
                let timeout_secs = self.stream_file_extraction.timeout_secs.get();
                tokio::time::timeout(
                    Duration::from_secs(timeout_secs),
                    self.submit_and_wait_extract_job(&job_config),
                )
                .await
                .map_err(|_| ClientError::StreamExtractionTimeout(timeout_secs))??;
                stream_files_collection
                    .find_one(filter)
                    .await?
                    .ok_or_else(|| {
                        ClientError::InvalidInput(format!(
                            "Unable to extract stream with streamId={} at logEventIdx={}",
                            extraction.stream_id, extraction.log_event_idx
                        ))
                    })?
            };

        metadata.path = self
            .stream_file_extraction
            .storage
            .resolve_path(&metadata.path)
            .await?;
        Ok(metadata)
    }

    /// Submits an extraction job to the query jobs table and polls its status until it finishes.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::QueryJobNotFound`] if the job disappears from the database.
    /// * [`ClientError::InvalidInput`] if the job fails, is killed, or is cancelled.
    /// * Forwards [`ExtractJobConfig::encode`]'s return values on failure.
    /// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
    /// * Forwards [`sqlx::query::QueryScalar::fetch_optional`]'s return values on failure.
    async fn submit_and_wait_extract_job(
        &self,
        job_config: &ExtractJobConfig,
    ) -> Result<(), ClientError> {
        let job_id = sqlx::query(&format!(
            "INSERT INTO `{QUERY_JOBS_TABLE_NAME}` (`job_config`, `type`) VALUES (?, ?)"
        ))
        .bind(job_config.encode()?)
        .bind(job_config.job_type())
        .execute(&self.sql_pool)
        .await?
        .last_insert_id();

        let polling = &self.stream_file_extraction.query_job_polling;
        let mut delay_ms = polling.initial_backoff_ms.max(1);
        let max_delay_ms = polling.max_backoff_ms.max(delay_ms);
        loop {
            let status = sqlx::query_scalar::<_, QueryJobStatus>(&format!(
                "SELECT `status` FROM `{QUERY_JOBS_TABLE_NAME}` WHERE `id` = ?"
            ))
            .bind(job_id)
            .fetch_optional(&self.sql_pool)
            .await?
            .ok_or(ClientError::QueryJobNotFound(job_id))?;
            match status {
                QueryJobStatus::Succeeded => return Ok(()),
                QueryJobStatus::Pending
                | QueryJobStatus::Running
                | QueryJobStatus::CancelRequested
                | QueryJobStatus::Cancelling => {
                    tokio::time::sleep(Duration::from_millis(delay_ms)).await;
                    delay_ms = std::cmp::min(delay_ms.saturating_mul(2), max_delay_ms);
                }
                QueryJobStatus::Cancelled => {
                    return Err(ClientError::InvalidInput(format!(
                        "Extract job {job_id} was cancelled"
                    )));
                }
                QueryJobStatus::Failed | QueryJobStatus::Killed => {
                    return Err(ClientError::InvalidInput(format!(
                        "Extract job {job_id} exited with unexpected status={}",
                        i32::from(status)
                    )));
                }
            }
        }
    }
}

/// The config of an extraction job, by the job's type.
#[derive(Debug, Eq, PartialEq)]
enum ExtractJobConfig {
    Ir(ExtractIrJobConfig),
    Json(ExtractJsonJobConfig),
}

impl ExtractJobConfig {
    /// # Returns
    ///
    /// The type of the job that the config belongs to.
    const fn job_type(&self) -> QueryJobType {
        match self {
            Self::Ir(_) => QueryJobType::ExtractIr,
            Self::Json(_) => QueryJobType::ExtractJson,
        }
    }

    /// Encodes the config into the msgpack form stored in the query jobs table.
    ///
    /// # Returns
    ///
    /// The encoded config on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`rmp_serde::to_vec_named`]'s return values on failure.
    fn encode(&self) -> Result<Vec<u8>, rmp_serde::encode::Error> {
        match self {
            Self::Ir(config) => rmp_serde::to_vec_named(config),
            Self::Json(config) => rmp_serde::to_vec_named(config),
        }
    }
}

/// Where extracted stream files are stored.
#[derive(Clone)]
enum StreamFileStorage {
    Fs,
    S3 {
        client: aws_sdk_s3::Client,
        bucket: NonEmptyString,
        key_prefix: NonEmptyString,
    },
}

impl StreamFileStorage {
    /// Resolves a stream file's path in the results cache to the path that the `WebUI` fetches the
    /// stream file from.
    ///
    /// # Returns
    ///
    /// On success:
    ///
    /// * For S3 storage, a pre-signed GET URL for the object under the storage's key prefix.
    /// * For filesystem storage, the path under the `WebUI`'s `/streams` static mount.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::Aws`] if the pre-signing config is invalid.
    /// * Forwards [`GetObjectFluentBuilder::presigned`]'s return values on failure.
    ///
    /// [`GetObjectFluentBuilder::presigned`]:
    ///   aws_sdk_s3::operation::get_object::builders::GetObjectFluentBuilder::presigned
    async fn resolve_path(&self, path: &str) -> Result<String, ClientError> {
        const PRE_SIGNED_URL_EXPIRY_TIME_SECONDS: u64 = 3600;

        match self {
            Self::Fs => Ok(format!("/streams/{path}")),
            Self::S3 {
                client,
                bucket,
                key_prefix,
            } => {
                let presigning_config = PresigningConfig::expires_in(Duration::from_secs(
                    PRE_SIGNED_URL_EXPIRY_TIME_SECONDS,
                ))
                .map_err(|err| ClientError::Aws {
                    description: err.to_string(),
                })?;
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

/// # Returns
///
/// The `OpenAPI` schema of [`StreamFileExtraction::extract_job_type`], which only accepts the
/// extraction job types.
fn extract_job_type_schema() -> utoipa::openapi::Object {
    utoipa::openapi::ObjectBuilder::new()
        .schema_type(utoipa::openapi::schema::Type::String)
        .description(Some("The type of extraction job to submit."))
        .enum_values(Some(
            [QueryJobType::ExtractIr, QueryJobType::ExtractJson].map(|job_type| {
                serde_json::to_value(job_type).expect("a query job type should serialize")
            }),
        ))
        .build()
}

#[cfg(test)]
mod tests {
    use clp_rust_utils::clp_config::AwsAuthentication;
    use clp_rust_utils::clp_config::AwsCredentials;
    use clp_rust_utils::clp_config::S3Config;
    use clp_rust_utils::clp_config::package::config::ApiServer;
    use clp_rust_utils::clp_config::package::config::StreamOutputStorage;
    use clp_rust_utils::dataset::DatasetName;
    use clp_rust_utils::job_config::ExtractIrJobConfig;
    use clp_rust_utils::job_config::ExtractJsonJobConfig;
    use clp_rust_utils::job_config::QueryJobType;
    use clp_rust_utils::types::non_empty_string::ExpectedNonEmpty;
    use non_empty_string::NonEmptyString;

    use super::ExtractJobConfig;
    use super::StreamFileExtraction;
    use super::StreamFileExtractionSettings;

    const ARCHIVE_ID: &str = "3f1c1a5e-5b6e-4a43-9d6b-0f2b2f5a8c11";
    const TARGET_UNCOMPRESSED_SIZE: u64 = 1024;

    fn extraction(
        extract_job_type: QueryJobType,
        dataset: Option<&str>,
        stream_id: &str,
    ) -> StreamFileExtraction {
        StreamFileExtraction {
            dataset: dataset.map(str::to_owned),
            extract_job_type,
            log_event_idx: 42,
            stream_id: stream_id.to_owned(),
        }
    }

    #[test]
    fn ir_extraction_builds_an_extract_ir_job_config() -> anyhow::Result<()> {
        let job_config = extraction(QueryJobType::ExtractIr, None, "orig-file")
            .job_config(TARGET_UNCOMPRESSED_SIZE)?;
        assert_eq!(job_config.job_type(), QueryJobType::ExtractIr);
        assert_eq!(
            job_config,
            ExtractJobConfig::Ir(ExtractIrJobConfig {
                orig_file_id: NonEmptyString::from_static_str("orig-file"),
                msg_ix: 42,
                file_split_id: None,
                target_uncompressed_size: Some(TARGET_UNCOMPRESSED_SIZE),
            })
        );
        Ok(())
    }

    #[test]
    fn json_extraction_builds_an_extract_json_job_config_in_the_given_or_default_dataset()
    -> anyhow::Result<()> {
        for (dataset, expected_dataset) in [
            (Some("logs"), "logs".parse::<DatasetName>()?),
            (None, DatasetName::default()),
        ] {
            let job_config = extraction(QueryJobType::ExtractJson, dataset, ARCHIVE_ID)
                .job_config(TARGET_UNCOMPRESSED_SIZE)?;
            assert_eq!(job_config.job_type(), QueryJobType::ExtractJson);
            assert_eq!(
                job_config,
                ExtractJobConfig::Json(ExtractJsonJobConfig {
                    dataset: expected_dataset,
                    archive_id: ARCHIVE_ID.parse()?,
                    target_chunk_size: Some(TARGET_UNCOMPRESSED_SIZE),
                }),
                "dataset: {dataset:?}"
            );
        }
        Ok(())
    }

    #[tokio::test]
    async fn fs_stream_files_resolve_under_the_streams_mount() -> anyhow::Result<()> {
        let settings = StreamFileExtractionSettings::from_config(
            &ApiServer::default(),
            &StreamOutputStorage::Fs {
                directory: "/var/data/streams".to_owned(),
            },
        )
        .await;
        assert_eq!(
            settings
                .storage
                .resolve_path("archive/stream.jsonl")
                .await?,
            "/streams/archive/stream.jsonl"
        );
        Ok(())
    }

    #[tokio::test]
    async fn s3_stream_files_resolve_to_a_presigned_url_under_the_key_prefix() -> anyhow::Result<()>
    {
        let s3_config = S3Config {
            bucket: NonEmptyString::from_static_str("stream-bucket"),
            region_code: None,
            key_prefix: NonEmptyString::from_static_str("streams/"),
            endpoint_url: Some(NonEmptyString::from_static_str("http://127.0.0.1:9000")),
            aws_authentication: AwsAuthentication::Credentials {
                credentials: AwsCredentials {
                    access_key_id: "access-key-id".to_owned(),
                    secret_access_key: "secret-access-key".to_owned(),
                    session_token: None,
                },
            },
        };
        let settings = StreamFileExtractionSettings::from_config(
            &ApiServer::default(),
            &StreamOutputStorage::S3 {
                staging_directory: "/var/data/staged-streams".to_owned(),
                s3_config,
            },
        )
        .await;

        let url = settings
            .storage
            .resolve_path("archive/stream.jsonl")
            .await?;
        assert!(
            url.starts_with("http://127.0.0.1:9000/stream-bucket/streams/archive/stream.jsonl?"),
            "unexpected URL: {url}"
        );
        assert!(url.contains("X-Amz-Expires=3600"), "unexpected URL: {url}");
        Ok(())
    }
}

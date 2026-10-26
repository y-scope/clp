//! Extraction of stream files for the `WebUI`'s log viewer.

use std::time::Duration;

use aws_sdk_s3::presigning::PresigningConfig;
use clp_rust_utils::aws::AWS_DEFAULT_REGION;
use clp_rust_utils::clp_config::package::config::ApiServer;
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

use crate::error::ClientError;
use crate::webui_client::WebuiClient;

/// Request body for the stream-file extraction endpoint.
#[derive(Debug, Deserialize, ToSchema)]
#[serde(deny_unknown_fields)]
pub struct StreamFileExtraction {
    /// The dataset of the archive to extract the stream from, for JSON extraction. Defaults to the
    /// default dataset when absent or `null`. For IR extraction, it's validated but otherwise
    /// ignored.
    #[schema(value_type = Option<String>)]
    pub dataset: DatasetName,
    /// The type of extraction job to submit.
    pub extract_job_type: ExtractJobType,
    /// Index of the log event that the extracted stream file must contain. Must not be negative.
    pub log_event_idx: i64,
    /// ID of the stream to extract: the original file's ID for IR extraction, or the archive ID
    /// for JSON extraction.
    #[schema(min_length = 1)]
    pub stream_id: String,
}

/// The type of an extraction job.
#[derive(Clone, Copy, Debug, Deserialize, ToSchema)]
pub enum ExtractJobType {
    ExtractIr,
    ExtractJson,
}

impl From<ExtractJobType> for QueryJobType {
    fn from(value: ExtractJobType) -> Self {
        match value {
            ExtractJobType::ExtractIr => Self::ExtractIr,
            ExtractJobType::ExtractJson => Self::ExtractJson,
        }
    }
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
    /// Extracts a stream file that contains the log event at `log_event_idx` in the stream with ID
    /// `stream_id`.
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
    /// * [`ClientError::StreamExtractionTimeout`] if the extraction job doesn't finish within the
    ///   configured timeout.
    /// * [`ClientError::InvalidInput`] if the extraction job finishes without extracting a stream
    ///   file that contains the log event.
    /// * Forwards [`ExtractJobConfig::from_extraction`]'s return values on failure.
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
        let job_config = ExtractJobConfig::from_extraction(
            &extraction,
            self.config.stream_output.target_uncompressed_size.get(),
        )?;
        let stream_files = self
            .mongodb_client
            .database(&self.config.results_cache.db_name)
            .collection::<StreamFileMetadata>(&self.config.results_cache.stream_collection_name);
        let filter = doc! {
            "stream_id": job_config.stream_id(),
            "begin_msg_ix": {"$lte": extraction.log_event_idx},
            "end_msg_ix": {"$gt": extraction.log_event_idx},
        };

        let mut metadata = if let Some(metadata) = stream_files.find_one(filter.clone()).await? {
            metadata
        } else {
            let timeout_secs = self
                .get_api_server_config()
                .stream_file_extraction_timeout_secs
                .get();
            tokio::time::timeout(
                Duration::from_secs(timeout_secs),
                self.submit_and_wait_for_extraction_job(&job_config),
            )
            .await
            .map_err(|_| ClientError::StreamExtractionTimeout(timeout_secs))??;
            stream_files.find_one(filter).await?.ok_or_else(|| {
                ClientError::InvalidInput(format!(
                    "unable to extract stream with stream_id={} at log_event_idx={}",
                    extraction.stream_id, extraction.log_event_idx
                ))
            })?
        };

        metadata.path = self
            .stream_file_storage
            .resolve_path(&metadata.path)
            .await?;
        Ok(metadata)
    }

    /// Submits an extraction job to the query jobs table, and polls its status until it finishes.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::QueryJobNotFound`] if the job disappears from the query jobs table before
    ///   it finishes.
    /// * [`ClientError::InvalidInput`] if the job fails, is killed, or is cancelled.
    /// * Forwards [`rmp_serde::to_vec_named`]'s return values on failure.
    /// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
    /// * Forwards [`sqlx::query::QueryScalar::fetch_optional`]'s return values on failure.
    ///
    /// # Panics
    ///
    /// Panics if `self.config.api_server` is `None`.
    async fn submit_and_wait_for_extraction_job(
        &self,
        job_config: &ExtractJobConfig,
    ) -> Result<(), ClientError> {
        let job_id = sqlx::query(&format!(
            "INSERT INTO `{QUERY_JOBS_TABLE_NAME}` (`job_config`, `type`) VALUES (?, ?)"
        ))
        .bind(rmp_serde::to_vec_named(job_config)?)
        .bind(QueryJobType::from(job_config.job_type()))
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
                        "extraction job {job_id} was cancelled"
                    )));
                }
                QueryJobStatus::Failed | QueryJobStatus::Killed => {
                    return Err(ClientError::InvalidInput(format!(
                        "extraction job {job_id} exited with status {status:?}"
                    )));
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

/// The config of an extraction job.
#[derive(Debug, Eq, PartialEq, Serialize)]
// The query scheduler decodes a job's config by the job's type, so only the inner config is stored.
#[serde(untagged)]
enum ExtractJobConfig {
    Ir(ExtractIrJobConfig),
    Json(ExtractJsonJobConfig),
}

impl ExtractJobConfig {
    /// Factory function.
    ///
    /// Creates the config of the job that performs the given extraction, with the given target
    /// uncompressed size of each extracted stream file.
    ///
    /// # Returns
    ///
    /// The newly created config on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::InvalidInput`] if:
    ///   * `stream_id` is empty.
    ///   * `log_event_idx` is negative.
    ///   * `stream_id` isn't an archive ID for JSON extraction.
    fn from_extraction(
        extraction: &StreamFileExtraction,
        target_uncompressed_size: u64,
    ) -> Result<Self, ClientError> {
        let Ok(stream_id) = NonEmptyString::new(extraction.stream_id.clone()) else {
            return Err(ClientError::InvalidInput(
                "stream_id must not be empty".to_owned(),
            ));
        };
        let msg_ix = u64::try_from(extraction.log_event_idx).map_err(|_| {
            ClientError::InvalidInput("log_event_idx must not be negative".to_owned())
        })?;
        match extraction.extract_job_type {
            ExtractJobType::ExtractIr => Ok(Self::Ir(ExtractIrJobConfig {
                orig_file_id: stream_id,
                msg_ix,
                file_split_id: None,
                target_uncompressed_size: Some(target_uncompressed_size),
            })),
            ExtractJobType::ExtractJson => {
                let archive_id = stream_id
                    .as_str()
                    .parse::<ArchiveId>()
                    .map_err(|err| ClientError::InvalidInput(err.to_string()))?;
                Ok(Self::Json(ExtractJsonJobConfig {
                    dataset: extraction.dataset.clone(),
                    archive_id,
                    target_chunk_size: Some(target_uncompressed_size),
                }))
            }
        }
    }

    /// # Returns
    ///
    /// The type of the job that the config belongs to.
    const fn job_type(&self) -> ExtractJobType {
        match self {
            Self::Ir(_) => ExtractJobType::ExtractIr,
            Self::Json(_) => ExtractJobType::ExtractJson,
        }
    }

    /// # Returns
    ///
    /// The ID of the stream that the job extracts, as the stream files it extracts record it.
    fn stream_id(&self) -> String {
        match self {
            Self::Ir(config) => config.orig_file_id.to_string(),
            Self::Json(config) => config.archive_id.to_string(),
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
    use clp_rust_utils::clp_config::package::config::StreamOutput;
    use clp_rust_utils::clp_config::package::config::StreamOutputStorage;
    use clp_rust_utils::dataset::DatasetName;
    use clp_rust_utils::job_config::ExtractIrJobConfig;
    use clp_rust_utils::job_config::ExtractJsonJobConfig;
    use clp_rust_utils::job_config::QueryJobType;
    use clp_rust_utils::types::ArchiveId;
    use clp_rust_utils::types::non_empty_string::ExpectedNonEmpty;
    use non_empty_string::NonEmptyString;

    use super::ExtractJobConfig;
    use super::ExtractJobType;
    use super::StreamFileExtraction;
    use super::StreamFileStorage;
    use crate::webui_client::WebuiClient;

    const TARGET_UNCOMPRESSED_SIZE: u64 = 1024;

    #[test]
    fn ir_extraction_builds_an_extract_ir_job_config_of_the_original_file() -> anyhow::Result<()> {
        let extraction = StreamFileExtraction {
            dataset: DatasetName::default(),
            extract_job_type: ExtractJobType::ExtractIr,
            log_event_idx: 42,
            stream_id: "orig-file".to_owned(),
        };

        let job_config = ExtractJobConfig::from_extraction(&extraction, TARGET_UNCOMPRESSED_SIZE)?;

        assert_eq!(
            job_config,
            ExtractJobConfig::Ir(ExtractIrJobConfig {
                orig_file_id: NonEmptyString::from_static_str("orig-file"),
                msg_ix: 42,
                file_split_id: None,
                target_uncompressed_size: Some(TARGET_UNCOMPRESSED_SIZE),
            })
        );
        assert_eq!(
            QueryJobType::from(job_config.job_type()),
            QueryJobType::ExtractIr
        );
        assert_eq!(job_config.stream_id(), "orig-file");
        Ok(())
    }

    #[test]
    fn json_extraction_builds_an_extract_json_job_config_of_the_archive_id() -> anyhow::Result<()> {
        const ARCHIVE_ID: &str = "3f1c1a5e-5b6e-4a43-9d6b-0f2b2f5a8c11";

        let extraction = StreamFileExtraction {
            dataset: "logs".parse()?,
            extract_job_type: ExtractJobType::ExtractJson,
            log_event_idx: 42,
            stream_id: ARCHIVE_ID.to_owned(),
        };

        let job_config = ExtractJobConfig::from_extraction(&extraction, TARGET_UNCOMPRESSED_SIZE)?;

        assert_eq!(
            job_config,
            ExtractJobConfig::Json(ExtractJsonJobConfig {
                dataset: "logs".parse()?,
                archive_id: ARCHIVE_ID.parse()?,
                target_chunk_size: Some(TARGET_UNCOMPRESSED_SIZE),
            })
        );
        assert_eq!(
            QueryJobType::from(job_config.job_type()),
            QueryJobType::ExtractJson
        );
        assert_eq!(job_config.stream_id(), ARCHIVE_ID);
        Ok(())
    }

    #[test]
    fn invalid_extractions_are_rejected() {
        let archive_id_error = "orig-file"
            .parse::<ArchiveId>()
            .expect_err("the stream ID should be an invalid archive ID");
        for (extract_job_type, log_event_idx, stream_id, expected_error) in [
            (
                ExtractJobType::ExtractIr,
                0,
                "",
                "Invalid input: stream_id must not be empty".to_owned(),
            ),
            (
                ExtractJobType::ExtractIr,
                -1,
                "orig-file",
                "Invalid input: log_event_idx must not be negative".to_owned(),
            ),
            (
                ExtractJobType::ExtractJson,
                -1,
                "3f1c1a5e-5b6e-4a43-9d6b-0f2b2f5a8c11",
                "Invalid input: log_event_idx must not be negative".to_owned(),
            ),
            (
                ExtractJobType::ExtractJson,
                0,
                "orig-file",
                format!("Invalid input: {archive_id_error}"),
            ),
        ] {
            let extraction = StreamFileExtraction {
                dataset: DatasetName::default(),
                extract_job_type,
                log_event_idx,
                stream_id: stream_id.to_owned(),
            };
            let error = ExtractJobConfig::from_extraction(&extraction, TARGET_UNCOMPRESSED_SIZE)
                .expect_err("the extraction should be rejected");
            assert_eq!(error.to_string(), expected_error);
        }
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

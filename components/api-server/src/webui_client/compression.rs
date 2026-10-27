//! Submission of compression jobs for paths in the logs input.

use clp_rust_utils::clp_config::package::config::LOGS_INPUT_MOUNT_POINT;
use clp_rust_utils::dataset::DatasetName;
use clp_rust_utils::job_config::COMPRESSION_JOBS_TABLE_NAME;
use clp_rust_utils::job_config::ClpIoConfig;
use clp_rust_utils::job_config::CompressionJobId;
use clp_rust_utils::job_config::FsInputConfig;
use clp_rust_utils::job_config::InputConfig;
use clp_rust_utils::job_config::OutputConfig;
use clp_rust_utils::serde::ZstdMsgpack;
use clp_rust_utils::types::non_empty_string::ExpectedNonEmpty;
use non_empty_string::NonEmptyString;
use serde::Deserialize;
use serde::Serialize;
use utoipa::ToSchema;

use crate::error::ClientError;
use crate::webui_client::LogsInputMountPoint;
use crate::webui_client::LogsInputPath;
use crate::webui_client::WebuiClient;

/// Request body for the compression-job submission endpoint.
#[derive(Debug, Deserialize, ToSchema)]
#[serde(deny_unknown_fields)]
pub struct CompressionJobCreation {
    /// The logs-input paths of the files and directories to compress: relative to the logs-input
    /// mount point, with a leading `/` that denotes the mount point itself. Must not be empty.
    #[schema(value_type = Vec<String>, min_items = 1)]
    pub paths: Vec<LogsInputPath>,
    /// The dataset to compress the logs into. Defaults to the default dataset when absent or
    /// `null`.
    #[schema(value_type = Option<String>)]
    pub dataset: DatasetName,
    /// The key of the log events' timestamp field.
    #[serde(default)]
    #[schema(value_type = Option<String>, min_length = 1)]
    pub timestamp_key: Option<NonEmptyString>,
    /// Whether the logs are unstructured. Defaults to `false` when absent or `null`.
    #[serde(default)]
    pub unstructured: Option<bool>,
}

/// Response body of the compression-job submission endpoint.
#[derive(Debug, Serialize, ToSchema)]
pub struct CompressionJob {
    /// The ID of the submitted compression job.
    #[schema(value_type = i32)]
    pub job_id: CompressionJobId,
}

impl WebuiClient {
    /// Submits a compression job for paths in the logs input.
    ///
    /// The job compresses the paths at their locations under the logs-input mount point, and
    /// records them relative to the mount point.
    ///
    /// # Returns
    ///
    /// The submitted compression job on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::InvalidInput`] if `creation` has no paths.
    /// * [`ClientError::CompressionJobIdOutOfRange`] if the ID of the inserted job doesn't fit in a
    ///   [`CompressionJobId`].
    /// * Forwards [`Self::check_logs_input_access`]'s return values on failure.
    /// * Forwards [`LogsInputMountPoint::resolve`]'s return values on failure.
    /// * Forwards [`LogsInputMountPoint::locate`]'s return values on failure.
    /// * Forwards [`ZstdMsgpack::serialize`]'s return values on failure.
    /// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
    pub async fn submit_compression_job(
        &self,
        creation: CompressionJobCreation,
    ) -> Result<CompressionJob, ClientError> {
        self.check_logs_input_access("compression job submission")?;
        let CompressionJobCreation {
            paths,
            dataset,
            timestamp_key,
            unstructured,
        } = creation;
        if paths.is_empty() {
            return Err(ClientError::InvalidInput(
                "a compression job must have at least one path".to_owned(),
            ));
        }
        let mount_point =
            LogsInputMountPoint::resolve(NonEmptyString::from_static_str(LOGS_INPUT_MOUNT_POINT))
                .await?;
        let mut paths_to_compress = Vec::with_capacity(paths.len());
        for path in &paths {
            paths_to_compress.push(mount_point.locate(path).await?);
        }
        let clp_io_config =
            self.compression_job_config(paths_to_compress, dataset, timestamp_key, unstructured);
        let job_id = sqlx::query(&format!(
            "INSERT INTO `{COMPRESSION_JOBS_TABLE_NAME}` (`clp_config`) VALUES (?)"
        ))
        .bind(ZstdMsgpack::serialize(&clp_io_config)?)
        .execute(&self.sql_pool)
        .await?
        .last_insert_id();
        let job_id = CompressionJobId::try_from(job_id)
            .map_err(|_| ClientError::CompressionJobIdOutOfRange(job_id))?;
        Ok(CompressionJob { job_id })
    }

    /// Builds the config of a compression job for paths in the logs input.
    ///
    /// # Returns
    ///
    /// The job's config, with `paths_to_compress` used as given as the paths to compress, and the
    /// logs-input mount point as the prefix to remove from them.
    fn compression_job_config(
        &self,
        paths_to_compress: Vec<NonEmptyString>,
        dataset: DatasetName,
        timestamp_key: Option<NonEmptyString>,
        unstructured: Option<bool>,
    ) -> ClpIoConfig {
        ClpIoConfig {
            input: InputConfig::FsInputConfig {
                config: FsInputConfig {
                    dataset,
                    paths_to_compress,
                    path_prefix_to_remove: Some(NonEmptyString::from_static_str(
                        LOGS_INPUT_MOUNT_POINT,
                    )),
                    timestamp_key,
                    unstructured: unstructured.unwrap_or(false),
                },
            },
            output: OutputConfig::from(&self.config.archive_output),
        }
    }
}

#[cfg(test)]
mod tests {
    use clp_rust_utils::clp_config::package::config::ArchiveOutput;
    use clp_rust_utils::clp_config::package::config::Config;
    use clp_rust_utils::clp_config::package::config::LOGS_INPUT_MOUNT_POINT;
    use clp_rust_utils::clp_config::package::config::Package;
    use clp_rust_utils::clp_config::package::config::StorageEngine;
    use clp_rust_utils::job_config::ClpIoConfig;
    use clp_rust_utils::job_config::FsInputConfig;
    use clp_rust_utils::job_config::InputConfig;
    use clp_rust_utils::job_config::OutputConfig;
    use non_empty_string::NonEmptyString;

    use super::CompressionJobCreation;
    use crate::webui_client::LogsInputPath;
    use crate::webui_client::WebuiClient;

    /// # Returns
    ///
    /// `value` as a [`NonEmptyString`].
    ///
    /// # Panics
    ///
    /// Panics if `value` is empty.
    fn non_empty(value: &str) -> NonEmptyString {
        NonEmptyString::new(value.to_owned()).expect("the test value should be non-empty")
    }

    #[tokio::test]
    async fn job_config_compresses_the_given_paths_and_removes_the_mount_point_prefix()
    -> anyhow::Result<()> {
        let archive_output = ArchiveOutput {
            target_archive_size: 1,
            target_dictionaries_size: 2,
            target_encoded_file_size: 3,
            target_segment_size: 4,
            compression_level: 5,
            ..ArchiveOutput::default()
        };
        let client = WebuiClient::new_lazy(Config {
            archive_output: archive_output.clone(),
            ..Config::default()
        })
        .await;
        let paths_to_compress = vec![
            non_empty("/mnt/logs/inside-link/inner.log"),
            non_empty("/mnt/logs/dir"),
        ];

        for (unstructured, expected_unstructured) in [(Some(true), true), (None, false)] {
            assert_eq!(
                client.compression_job_config(
                    paths_to_compress.clone(),
                    "logs".parse()?,
                    Some(non_empty("ts")),
                    unstructured,
                ),
                ClpIoConfig {
                    input: InputConfig::FsInputConfig {
                        config: FsInputConfig {
                            dataset: "logs".parse()?,
                            paths_to_compress: vec![
                                non_empty("/mnt/logs/inside-link/inner.log"),
                                non_empty("/mnt/logs/dir"),
                            ],
                            path_prefix_to_remove: Some(non_empty(LOGS_INPUT_MOUNT_POINT)),
                            timestamp_key: Some(non_empty("ts")),
                            unstructured: expected_unstructured,
                        },
                    },
                    output: OutputConfig::from(&archive_output),
                },
                "unstructured: {unstructured:?}"
            );
        }
        Ok(())
    }

    #[tokio::test]
    async fn submission_requires_at_least_one_path() -> anyhow::Result<()> {
        let client = WebuiClient::new_lazy(Config {
            package: Package {
                storage_engine: StorageEngine::ClpS,
            },
            ..Config::default()
        })
        .await;
        let creation = CompressionJobCreation {
            paths: Vec::new(),
            dataset: "logs".parse()?,
            timestamp_key: None,
            unstructured: None,
        };
        let error = client
            .submit_compression_job(creation)
            .await
            .expect_err("the submission should be rejected");
        assert_eq!(
            error.to_string(),
            "Invalid input: a compression job must have at least one path"
        );
        Ok(())
    }

    #[tokio::test]
    async fn submission_is_rejected_under_the_clp_storage_engine() -> anyhow::Result<()> {
        let client = WebuiClient::new_lazy(Config {
            package: Package {
                storage_engine: StorageEngine::Clp,
            },
            ..Config::default()
        })
        .await;
        let creation = CompressionJobCreation {
            paths: vec![LogsInputPath::try_from("/".to_owned())?],
            dataset: "logs".parse()?,
            timestamp_key: None,
            unstructured: None,
        };
        let error = client
            .submit_compression_job(creation)
            .await
            .expect_err("the submission should be rejected");
        assert_eq!(
            error.to_string(),
            "Invalid input: compression job submission requires the clp-s storage engine"
        );
        Ok(())
    }
}

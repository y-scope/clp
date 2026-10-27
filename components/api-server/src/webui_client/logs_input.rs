//! Listing of the logs input's directories.

use clp_rust_utils::clp_config::package::config::LOGS_INPUT_MOUNT_POINT;
use clp_rust_utils::clp_config::package::config::LogsInput;
use clp_rust_utils::clp_config::package::config::StorageEngine;
use clp_rust_utils::logs_input::LogsInputMountPoint;
use clp_rust_utils::logs_input::LogsInputPath;
use clp_rust_utils::types::non_empty_string::ExpectedNonEmpty;
use non_empty_string::NonEmptyString;
use serde::Serialize;
use utoipa::ToSchema;

use crate::error::ClientError;
use crate::webui_client::WebuiClient;

/// An entry of a directory in the logs input.
#[derive(Debug, Eq, PartialEq, Serialize, ToSchema)]
pub struct DirEntry {
    /// Whether the entry is a directory, or a symlink to a directory under the logs-input mount
    /// point, which the `WebUI`'s path picker can expand.
    pub is_expandable: bool,
    /// The entry's file name.
    pub name: String,
    /// The logs-input path of the directory that contains the entry.
    #[schema(value_type = String)]
    pub parent_path: LogsInputPath,
}

impl WebuiClient {
    /// Lists the entries of the directory at a path in the logs input.
    ///
    /// # Returns
    ///
    /// The directory's entries on success, in no particular order, or an empty list if `path` isn't
    /// a directory.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::TaskJoin`] if the task listing the directory panics.
    /// * Forwards [`Self::check_logs_input_access`]'s return values on failure.
    /// * Forwards [`LogsInputMountPoint::resolve`]'s return values on failure.
    /// * Forwards [`LogsInputMountPoint::list`]'s return values on failure.
    pub async fn list_logs_input_entries(
        &self,
        path: LogsInputPath,
    ) -> Result<Vec<DirEntry>, ClientError> {
        self.check_logs_input_access("logs-input listing")?;
        let entries = tokio::task::spawn_blocking(move || {
            LogsInputMountPoint::resolve(NonEmptyString::from_static_str(LOGS_INPUT_MOUNT_POINT))?
                .list(&path)
                .map(|entries| {
                    entries
                        .into_iter()
                        .map(|entry| DirEntry {
                            is_expandable: entry.is_expandable,
                            name: entry.name,
                            parent_path: path.clone(),
                        })
                        .collect()
                })
        })
        .await??;
        Ok(entries)
    }

    /// Checks that the package's configuration lets the API server access the logs input.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::InvalidInput`] if:
    ///   * the package's storage engine isn't clp-s.
    ///   * the package's logs input isn't on the filesystem.
    pub(super) fn check_logs_input_access(&self, operation: &str) -> Result<(), ClientError> {
        if self.config.package.storage_engine != StorageEngine::ClpS {
            return Err(ClientError::InvalidInput(format!(
                "{operation} requires the clp-s storage engine"
            )));
        }
        match self.config.logs_input {
            LogsInput::Fs { .. } => Ok(()),
            LogsInput::S3 { .. } => Err(ClientError::InvalidInput(format!(
                "{operation} requires filesystem logs input"
            ))),
        }
    }
}

#[cfg(test)]
mod tests {
    use clp_rust_utils::clp_config::AwsAuthentication;
    use clp_rust_utils::clp_config::package::config::Config;
    use clp_rust_utils::clp_config::package::config::LogsInput;
    use clp_rust_utils::clp_config::package::config::Package;
    use clp_rust_utils::clp_config::package::config::S3Ingestion;
    use clp_rust_utils::clp_config::package::config::StorageEngine;
    use clp_rust_utils::logs_input::LogsInputPath;

    use crate::webui_client::WebuiClient;

    #[tokio::test]
    async fn logs_input_listing_requires_the_clp_s_storage_engine() -> anyhow::Result<()> {
        let client = WebuiClient::new_lazy(Config {
            package: Package {
                storage_engine: StorageEngine::Clp,
            },
            ..Config::default()
        })
        .await;
        let result = client
            .list_logs_input_entries(LogsInputPath::try_from("/".to_owned())?)
            .await;
        assert_eq!(
            result
                .expect_err("the listing should be rejected")
                .to_string(),
            "Invalid input: logs-input listing requires the clp-s storage engine"
        );
        Ok(())
    }

    #[tokio::test]
    async fn logs_input_listing_requires_filesystem_logs_input() -> anyhow::Result<()> {
        let logs_input = LogsInput::S3 {
            config: S3Ingestion {
                aws_authentication: AwsAuthentication::Default,
            },
        };
        let client = WebuiClient::new_lazy(Config {
            package: Package {
                storage_engine: StorageEngine::ClpS,
            },
            logs_input,
            ..Config::default()
        })
        .await;
        let result = client
            .list_logs_input_entries(LogsInputPath::try_from("/".to_owned())?)
            .await;
        assert_eq!(
            result
                .expect_err("the listing should be rejected")
                .to_string(),
            "Invalid input: logs-input listing requires filesystem logs input"
        );
        Ok(())
    }
}

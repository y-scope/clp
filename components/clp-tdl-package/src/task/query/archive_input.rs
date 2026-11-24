//! Resolves archives for `clp-s` commands.

use std::path::Path;
use std::path::PathBuf;
use std::process::Command;

use clp_rust_utils::aws::AWS_DEFAULT_REGION;
use clp_rust_utils::clp_config::package::config::ArchiveOutputStorage;
use clp_rust_utils::clp_config::package::config::SpiderTaskExecutorConfig;
use clp_rust_utils::dataset::DatasetName;
use clp_rust_utils::s3::generate_s3_url;
use clp_rust_utils::types::ArchiveUuid;
use non_empty_string::NonEmptyString;

use crate::task::utils::s3_credential_env;

/// The archive to read, in the form clp-s addresses it.
pub(super) enum ArchiveDescriptor {
    /// A local dataset archives directory plus the `--archive-id` selecting one archive in it.
    Directory {
        path: PathBuf,
        archive_uuid: ArchiveUuid,
    },

    /// The URL and credentials of an S3-hosted archive.
    S3 {
        url: String,
        credential_env: Vec<(&'static str, String)>,
    },
}

impl ArchiveDescriptor {
    /// Factory function.
    ///
    /// Resolves the archive location and any credentials needed to read it.
    ///
    /// # Returns
    ///
    /// The archive descriptor on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`generate_s3_url`]'s return values on failure.
    /// * Forwards [`s3_credential_env`]'s return values on failure.
    pub(super) fn resolve(
        runtime: &tokio::runtime::Handle,
        clp_home: &Path,
        config: &SpiderTaskExecutorConfig,
        dataset: &DatasetName,
        archive_uuid: ArchiveUuid,
    ) -> anyhow::Result<Self> {
        let s3_config = match &config.archive_output.storage {
            ArchiveOutputStorage::Fs { .. } => {
                return Ok(Self::Directory {
                    path: config
                        .abs_archive_output_staging(clp_home)
                        .join(dataset.as_str()),
                    archive_uuid,
                });
            }
            ArchiveOutputStorage::S3 { s3_config, .. } => s3_config,
        };

        let object_key = config
            .archive_output
            .dataset_archive_object_key(dataset, &archive_uuid);
        let url = generate_s3_url(
            s3_config.endpoint_url.as_ref().map(NonEmptyString::as_str),
            s3_config.region_code.as_ref().map(NonEmptyString::as_str),
            &s3_config.bucket,
            &object_key,
        )?;
        let region = s3_config
            .region_code
            .as_ref()
            .map_or(AWS_DEFAULT_REGION, NonEmptyString::as_str);
        let credential_env = s3_credential_env(runtime, region, &s3_config.aws_authentication)?;
        Ok(Self::S3 {
            url,
            credential_env,
        })
    }

    /// Configures a `clp-s` subcommand to read this archive.
    ///
    /// # Returns
    ///
    /// A command with the subcommand, archive arguments, and any required credential overrides.
    pub(super) fn clp_s_command(&self, clp_s_bin: &Path, subcommand: &str) -> Command {
        let mut command = Command::new(clp_s_bin);
        command.arg(subcommand);
        match self {
            Self::Directory { path, archive_uuid } => {
                command
                    .arg(path)
                    .arg("--archive-id")
                    .arg(archive_uuid.to_string());
            }
            Self::S3 {
                url,
                credential_env,
            } => {
                command
                    .arg(url)
                    .args(["--auth", "s3"])
                    .env_remove("AWS_SESSION_TOKEN")
                    .envs(credential_env.iter().map(|(name, value)| (*name, value)));
            }
        }
        command
    }
}

/// # Returns
///
/// An [`clp_rust_utils::clp_config::S3Config`] for `bucket` with the key prefix `LIB1/`, static AWS
/// credentials, and no endpoint URL.
#[cfg(test)]
pub(super) fn base_s3_config() -> clp_rust_utils::clp_config::S3Config {
    use clp_rust_utils::clp_config::AwsAuthentication;
    use clp_rust_utils::clp_config::AwsCredentials;
    use clp_rust_utils::clp_config::S3Config;
    use clp_rust_utils::types::non_empty_string::ExpectedNonEmpty;

    S3Config {
        bucket: NonEmptyString::from_static_str("bucket"),
        region_code: None,
        key_prefix: NonEmptyString::from_static_str("LIB1/"),
        endpoint_url: None,
        aws_authentication: AwsAuthentication::Credentials {
            credentials: AwsCredentials {
                access_key_id: "the-access-key".to_string(),
                secret_access_key: "the-secret-key".to_string(),
                session_token: None,
            },
        },
    }
}

#[cfg(test)]
mod tests {
    use std::collections::BTreeMap;
    use std::ffi::OsStr;
    use std::path::Path;

    use clp_rust_utils::clp_config::S3Config;
    use clp_rust_utils::clp_config::package::config::ArchiveOutput;
    use clp_rust_utils::clp_config::package::config::ArchiveOutputStorage;
    use clp_rust_utils::clp_config::package::config::Package;
    use clp_rust_utils::clp_config::package::config::SpiderTaskExecutorConfig;
    use clp_rust_utils::clp_config::package::config::StorageEngine;
    use clp_rust_utils::dataset::DatasetName;
    use clp_rust_utils::types::ArchiveUuid;
    use clp_rust_utils::types::non_empty_string::ExpectedNonEmpty;
    use non_empty_string::NonEmptyString;

    use super::ArchiveDescriptor;
    use super::base_s3_config;

    const ARCHIVE_UUID: &str = "018e90e5-8b2a-4a61-a2fc-cac799936caf";

    /// # Returns
    ///
    /// A [`SpiderTaskExecutorConfig`] whose archive output is S3-backed with the given staging
    /// directory and endpoint URL.
    fn s3_backed_config(
        staging_directory: &str,
        endpoint_url: Option<&'static str>,
    ) -> SpiderTaskExecutorConfig {
        SpiderTaskExecutorConfig {
            package: Package {
                storage_engine: StorageEngine::ClpS,
            },
            archive_output: ArchiveOutput {
                storage: ArchiveOutputStorage::S3 {
                    staging_directory: staging_directory.to_owned(),
                    s3_config: S3Config {
                        endpoint_url: endpoint_url.map(NonEmptyString::from_static_str),
                        ..base_s3_config()
                    },
                },
                ..ArchiveOutput::default()
            },
            ..SpiderTaskExecutorConfig::default()
        }
    }

    #[test]
    fn resolve_fs_joins_dataset_and_returns_no_credentials() -> anyhow::Result<()> {
        let runtime = tokio::runtime::Runtime::new().expect("failed to create Tokio runtime");
        let config = SpiderTaskExecutorConfig {
            package: Package {
                storage_engine: StorageEngine::ClpS,
            },
            archive_output: ArchiveOutput {
                storage: ArchiveOutputStorage::Fs {
                    directory: "var/data/archives".to_owned(),
                },
                ..ArchiveOutput::default()
            },
            ..SpiderTaskExecutorConfig::default()
        };

        let descriptor = ArchiveDescriptor::resolve(
            runtime.handle(),
            Path::new("/clp"),
            &config,
            &"ds1".parse::<DatasetName>()?,
            ARCHIVE_UUID
                .parse::<ArchiveUuid>()
                .expect("valid archive UUID"),
        )?;

        let command = descriptor.clp_s_command(Path::new("clp-s"), "s");
        assert_eq!(
            command.get_args().collect::<Vec<_>>(),
            [
                "s",
                "/clp/var/data/archives/ds1",
                "--archive-id",
                ARCHIVE_UUID
            ]
            .map(OsStr::new)
        );
        assert_eq!(command.get_envs().collect::<Vec<_>>(), &[]);

        Ok(())
    }

    #[test]
    fn resolve_s3_builds_object_url_and_credentials() -> anyhow::Result<()> {
        let runtime = tokio::runtime::Runtime::new().expect("failed to create Tokio runtime");
        let config = s3_backed_config("var/data/staged-archives", None);

        let descriptor = ArchiveDescriptor::resolve(
            runtime.handle(),
            Path::new("/clp"),
            &config,
            &"ds1".parse::<DatasetName>()?,
            ARCHIVE_UUID
                .parse::<ArchiveUuid>()
                .expect("valid archive UUID"),
        )?;

        let url = format!("https://bucket.s3.amazonaws.com/LIB1/ds1/{ARCHIVE_UUID}");
        let command = descriptor.clp_s_command(Path::new("clp-s"), "s");
        assert_eq!(
            command.get_args().collect::<Vec<_>>(),
            ["s", &url, "--auth", "s3"].map(OsStr::new)
        );
        assert_eq!(
            command.get_envs().collect::<BTreeMap<_, _>>(),
            BTreeMap::from([
                (
                    OsStr::new("AWS_ACCESS_KEY_ID"),
                    Some(OsStr::new("the-access-key"))
                ),
                (
                    OsStr::new("AWS_SECRET_ACCESS_KEY"),
                    Some(OsStr::new("the-secret-key"))
                ),
                (OsStr::new("AWS_SESSION_TOKEN"), None),
            ])
        );

        Ok(())
    }

    #[test]
    fn resolve_s3_ignores_staging_directory() -> anyhow::Result<()> {
        const STAGING_DIRECTORY: &str = "/wrong-staging-directory";

        let runtime = tokio::runtime::Runtime::new().expect("failed to create Tokio runtime");
        let config = s3_backed_config(STAGING_DIRECTORY, None);

        let descriptor = ArchiveDescriptor::resolve(
            runtime.handle(),
            Path::new("/clp"),
            &config,
            &"ds1".parse::<DatasetName>()?,
            ARCHIVE_UUID
                .parse::<ArchiveUuid>()
                .expect("valid archive UUID"),
        )?;
        let command = descriptor.clp_s_command(Path::new("clp-s"), "s");

        assert!(
            !command
                .get_args()
                .any(|arg| arg.to_string_lossy().contains(STAGING_DIRECTORY))
        );

        Ok(())
    }

    #[test]
    fn resolve_s3_custom_endpoint_uses_path_style_url() -> anyhow::Result<()> {
        let runtime = tokio::runtime::Runtime::new().expect("failed to create Tokio runtime");
        let config = s3_backed_config("var/data/staged-archives", Some("http://minio:9000"));

        let descriptor = ArchiveDescriptor::resolve(
            runtime.handle(),
            Path::new("/clp"),
            &config,
            &"ds1".parse::<DatasetName>()?,
            ARCHIVE_UUID
                .parse::<ArchiveUuid>()
                .expect("valid archive UUID"),
        )?;

        let url = format!("http://minio:9000/bucket/LIB1/ds1/{ARCHIVE_UUID}");
        let command = descriptor.clp_s_command(Path::new("clp-s"), "s");
        assert_eq!(
            command.get_args().collect::<Vec<_>>(),
            ["s", &url, "--auth", "s3"].map(OsStr::new)
        );

        Ok(())
    }
}

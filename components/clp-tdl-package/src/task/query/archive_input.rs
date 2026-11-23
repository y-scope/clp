//! Resolves archives for `clp-s` commands.

use std::ffi::OsString;
use std::path::Path;
use std::path::PathBuf;

use clp_rust_utils::aws::AWS_DEFAULT_REGION;
use clp_rust_utils::clp_config::package::config::ArchiveOutputStorage;
use clp_rust_utils::clp_config::package::config::SpiderTaskExecutorConfig;
use clp_rust_utils::dataset::DatasetName;
use clp_rust_utils::s3::generate_s3_url;
use clp_rust_utils::types::ArchiveId;
use non_empty_string::NonEmptyString;

use crate::task::utils::s3_credential_env;

/// The archive to read, in the form clp-s addresses it.
pub(super) enum ArchiveDescriptor {
    /// A local dataset archives directory plus the `--archive-id` selecting one archive in it.
    Directory {
        path: PathBuf,
        archive_id: ArchiveId,
    },

    /// The URL of an S3-hosted archive, read with `--auth s3`.
    ObjectUrl(String),
}

impl ArchiveDescriptor {
    /// Converts the descriptor into the archive input and selection or authentication arguments for
    /// `clp-s`.
    ///
    /// # Returns
    ///
    /// The ordered arguments that follow the subcommand.
    pub(super) fn into_clp_s_args(self) -> [OsString; 3] {
        match self {
            Self::Directory { path, archive_id } => [
                path.into_os_string(),
                OsString::from("--archive-id"),
                OsString::from(archive_id.to_string()),
            ],
            Self::ObjectUrl(url) => [
                OsString::from(url),
                OsString::from("--auth"),
                OsString::from("s3"),
            ],
        }
    }
}

/// Resolves how clp-s addresses the archive, and the credential env vars it needs to read it.
///
/// # Returns
///
/// A tuple on success, containing:
///
/// * The archive descriptor.
/// * The credential env vars clp-s should run with, which are empty for filesystem-backed archive
///   output.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`generate_s3_url`]'s return values on failure.
/// * Forwards [`s3_credential_env`]'s return values on failure.
pub(super) fn resolve_archive_input(
    runtime: &tokio::runtime::Handle,
    clp_home: &Path,
    config: &SpiderTaskExecutorConfig,
    dataset: &DatasetName,
    archive_id: ArchiveId,
) -> anyhow::Result<(ArchiveDescriptor, Vec<(&'static str, String)>)> {
    let s3_config = match &config.archive_output.storage {
        ArchiveOutputStorage::Fs { .. } => {
            return Ok((
                ArchiveDescriptor::Directory {
                    path: config
                        .abs_archive_output_staging(clp_home)
                        .join(dataset.as_str()),
                    archive_id,
                },
                Vec::new(),
            ));
        }
        ArchiveOutputStorage::S3 { s3_config, .. } => s3_config,
    };

    let object_key = config
        .archive_output
        .dataset_archive_object_key(dataset, &archive_id);
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
    Ok((ArchiveDescriptor::ObjectUrl(url), credential_env))
}

/// # Returns
///
/// An [`clp_rust_utils::clp_config::S3Config`] for `bucket` with the key prefix `LIB1/`,
/// static AWS credentials, and no endpoint URL.
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
    use std::path::Path;
    use std::path::PathBuf;

    use clp_rust_utils::clp_config::S3Config;
    use clp_rust_utils::clp_config::package::config::ArchiveOutput;
    use clp_rust_utils::clp_config::package::config::ArchiveOutputStorage;
    use clp_rust_utils::clp_config::package::config::Package;
    use clp_rust_utils::clp_config::package::config::SpiderTaskExecutorConfig;
    use clp_rust_utils::clp_config::package::config::StorageEngine;
    use clp_rust_utils::dataset::DatasetName;
    use clp_rust_utils::types::ArchiveId;
    use clp_rust_utils::types::non_empty_string::ExpectedNonEmpty;
    use non_empty_string::NonEmptyString;

    use super::ArchiveDescriptor;
    use super::base_s3_config;
    use super::resolve_archive_input;

    const ARCHIVE_ID: &str = "018e90e5-8b2a-4a61-a2fc-cac799936caf";

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
    fn resolve_archive_input_fs_joins_dataset_and_returns_no_credentials() -> anyhow::Result<()> {
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

        let (descriptor, credential_env) = resolve_archive_input(
            runtime.handle(),
            Path::new("/clp"),
            &config,
            &"ds1".parse::<DatasetName>()?,
            ARCHIVE_ID.parse::<ArchiveId>().expect("valid archive UUID"),
        )?;

        let ArchiveDescriptor::Directory { path, archive_id } = descriptor else {
            panic!("expected a directory descriptor");
        };
        assert_eq!(path, PathBuf::from("/clp/var/data/archives/ds1"));
        assert_eq!(
            archive_id,
            ARCHIVE_ID.parse::<ArchiveId>().expect("valid archive UUID")
        );
        assert_eq!(credential_env, &[]);

        Ok(())
    }

    #[test]
    fn resolve_archive_input_s3_builds_object_url_and_credentials() -> anyhow::Result<()> {
        let runtime = tokio::runtime::Runtime::new().expect("failed to create Tokio runtime");
        let config = s3_backed_config("var/data/staged-archives", None);

        let (descriptor, credential_env) = resolve_archive_input(
            runtime.handle(),
            Path::new("/clp"),
            &config,
            &"ds1".parse::<DatasetName>()?,
            ARCHIVE_ID.parse::<ArchiveId>().expect("valid archive UUID"),
        )?;

        let ArchiveDescriptor::ObjectUrl(url) = descriptor else {
            panic!("expected an object-URL descriptor");
        };
        assert_eq!(
            url,
            format!("https://bucket.s3.amazonaws.com/LIB1/ds1/{ARCHIVE_ID}")
        );
        assert_eq!(
            credential_env,
            vec![
                ("AWS_ACCESS_KEY_ID", "the-access-key".to_string()),
                ("AWS_SECRET_ACCESS_KEY", "the-secret-key".to_string()),
            ]
        );

        Ok(())
    }

    #[test]
    fn resolve_archive_input_s3_ignores_staging_directory() -> anyhow::Result<()> {
        const STAGING_DIRECTORY: &str = "/wrong-staging-directory";

        let runtime = tokio::runtime::Runtime::new().expect("failed to create Tokio runtime");
        let config = s3_backed_config(STAGING_DIRECTORY, None);

        let (descriptor, _) = resolve_archive_input(
            runtime.handle(),
            Path::new("/clp"),
            &config,
            &"ds1".parse::<DatasetName>()?,
            ARCHIVE_ID.parse::<ArchiveId>().expect("valid archive UUID"),
        )?;
        let args = descriptor.into_clp_s_args();

        assert!(
            !args
                .iter()
                .any(|arg| arg.to_string_lossy().contains(STAGING_DIRECTORY))
        );

        Ok(())
    }

    #[test]
    fn resolve_archive_input_s3_custom_endpoint_uses_path_style_url() -> anyhow::Result<()> {
        let runtime = tokio::runtime::Runtime::new().expect("failed to create Tokio runtime");
        let config = s3_backed_config("var/data/staged-archives", Some("http://minio:9000"));

        let (descriptor, _) = resolve_archive_input(
            runtime.handle(),
            Path::new("/clp"),
            &config,
            &"ds1".parse::<DatasetName>()?,
            ARCHIVE_ID.parse::<ArchiveId>().expect("valid archive UUID"),
        )?;

        let ArchiveDescriptor::ObjectUrl(url) = descriptor else {
            panic!("expected an object-URL descriptor");
        };
        assert_eq!(
            url,
            format!("http://minio:9000/bucket/LIB1/ds1/{ARCHIVE_ID}")
        );

        Ok(())
    }
}

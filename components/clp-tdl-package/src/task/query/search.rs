//! The `clp-s` search worker that queries a single archive.

use std::ffi::OsString;
use std::io::Read;
use std::path::Path;
use std::path::PathBuf;
use std::process::Command;
use std::process::Stdio;

use anyhow::Context;
use clp_rust_utils::aws::AWS_DEFAULT_REGION;
use clp_rust_utils::clp_config::package::config::ArchiveOutputStorage;
use clp_rust_utils::clp_config::package::config::SpiderTaskExecutorConfig;
use clp_rust_utils::clp_config::package::config::StorageEngine;
use clp_rust_utils::dataset::resolve_dataset_name;
use clp_rust_utils::job_config::QueryJobId;
use clp_rust_utils::s3::generate_s3_url;
use clp_rust_utils::task_io::query::ClpSQueryOption;
use clp_rust_utils::task_io::query::OutputHandle;
use non_empty_string::NonEmptyString;

use crate::common::clp_home;
use crate::common::runtime;
use crate::task::utils::clp_binary_path;
use crate::task::utils::s3_credential_env;

/// Searches one archive with clp-s, handles the search results according to the given
/// `output_handle`.
///
/// A pure worker function called by a spider-tdl task wrapper, which formats any returned
/// `anyhow::Error` into a user-space TDL error.
///
/// # Errors
///
/// Returns an error if:
///
/// * The configured storage engine is not [`StorageEngine::ClpS`].
/// * `output_handle` is not [`OutputHandle::ResultsCache`]. The current implementation only
///   supports result cache output streaming.
/// * Forwards [`resolve_archive_input`]'s return values on failure.
/// * Forwards [`run_clp_s_search`]'s return values on failure.
pub(super) fn search(
    ctx: &spider_tdl::TaskContext,
    config: &SpiderTaskExecutorConfig,
    query_job_id: QueryJobId,
    clp_s_query_option: &ClpSQueryOption,
    output_handle: &OutputHandle,
    dataset: Option<&str>,
    archive_id: String,
) -> anyhow::Result<()> {
    if StorageEngine::ClpS != config.package.storage_engine {
        anyhow::bail!("the clp-s query task requires the `clp-s` storage engine");
    }
    let OutputHandle::ResultsCache { uri } = output_handle else {
        anyhow::bail!("unsupported query output handler");
    };

    let dataset = resolve_dataset_name(dataset);

    tracing::info!(
        job_id = % ctx.job_id,
        task_id = % ctx.task_id,
        task_instance_id = % ctx.task_instance_id,
        query_job_id = % query_job_id,
        dataset = % dataset,
        archive_id = % archive_id,
        "clp-s query task started.",
    );

    let clp_home = clp_home();
    let (archive_selector, credential_env) =
        resolve_archive_input(&runtime(), clp_home, config, dataset, archive_id).inspect_err(
            |e| {
                tracing::error!(
                    job_id = % ctx.job_id,
                    task_id = % ctx.task_id,
                    task_instance_id = % ctx.task_instance_id,
                    query_job_id = % query_job_id,
                    error = % e,
                    "Failed to resolve the archive input."
                );
            },
        )?;
    let args = build_clp_s_search_args_for_result_cache(
        &archive_selector,
        clp_s_query_option,
        uri.as_str(),
        query_job_id,
        dataset,
    );
    run_clp_s_search(&clp_binary_path(clp_home, "clp-s"), args, &credential_env)?;

    tracing::info!(
        job_id = % ctx.job_id,
        task_id = % ctx.task_id,
        task_instance_id = % ctx.task_instance_id,
        query_job_id = % query_job_id,
        "clp-s query task completed successfully.",
    );
    Ok(())
}

/// Selector for clp-s to address the archive to search.
enum ArchiveSelector {
    /// A local dataset archives directory plus the `--archive-id` selecting one archive in it.
    Directory { path: PathBuf, archive_id: String },

    /// The URL of an S3-hosted archive, read with `--auth s3`.
    ObjectUrl(String),
}

/// Resolves how clp-s addresses the archive, and the credential env vars it needs to read it.
///
/// # Returns
///
/// A tuple containing:
///
/// * The archive selector.
/// * The credential env vars clp-s should run with, which are empty for filesystem-backed archive
///   output.
///
/// # Errors
///
/// Returns an error if:
///
/// * The archive's object key is empty.
/// * Forwards [`generate_s3_url`]'s return values on failure.
/// * Forwards [`s3_credential_env`]'s return values on failure.
fn resolve_archive_input(
    runtime: &tokio::runtime::Handle,
    clp_home: &Path,
    config: &SpiderTaskExecutorConfig,
    dataset: &str,
    archive_id: String,
) -> anyhow::Result<(ArchiveSelector, Vec<(&'static str, String)>)> {
    let s3_config = match &config.archive_output.storage {
        ArchiveOutputStorage::Fs { .. } => {
            return Ok((
                ArchiveSelector::Directory {
                    path: config.abs_archive_output_staging(clp_home).join(dataset),
                    archive_id,
                },
                Vec::new(),
            ));
        }
        ArchiveOutputStorage::S3 { s3_config, .. } => s3_config,
    };

    let object_key = config
        .archive_output
        .dataset_archive_object_key(Some(dataset), &archive_id);
    let object_key = NonEmptyString::try_from(object_key)
        .map_err(|_| anyhow::anyhow!("archive object key must not be empty"))?;
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
    Ok((ArchiveSelector::ObjectUrl(url), credential_env))
}

/// Builds the clp-s command-line arguments for a single-archive search writing to the result cache.
///
/// # Returns
///
/// The ordered clp-s arguments.
fn build_clp_s_search_args_for_result_cache(
    archive_selector: &ArchiveSelector,
    clp_s_query_option: &ClpSQueryOption,
    result_cache_uri: &str,
    query_job_id: QueryJobId,
    dataset: &str,
) -> Vec<OsString> {
    let mut args = vec![OsString::from("s")];
    match archive_selector {
        ArchiveSelector::Directory { path, archive_id } => {
            args.push(path.as_os_str().to_os_string());
            args.push(OsString::from("--archive-id"));
            args.push(OsString::from(archive_id));
        }
        ArchiveSelector::ObjectUrl(url) => {
            args.push(OsString::from(url));
            args.push(OsString::from("--auth"));
            args.push(OsString::from("s3"));
        }
    }

    args.push(OsString::from(clp_s_query_option.query_string.as_str()));
    if let Some(begin_timestamp_millisecs) = clp_s_query_option.begin_timestamp_millisecs {
        args.push(OsString::from("--tge"));
        args.push(OsString::from(begin_timestamp_millisecs.to_string()));
    }
    if let Some(end_timestamp_millisecs) = clp_s_query_option.end_timestamp_millisecs {
        args.push(OsString::from("--tle"));
        args.push(OsString::from(end_timestamp_millisecs.to_string()));
    }
    if clp_s_query_option.ignore_case {
        args.push(OsString::from("--ignore-case"));
    }

    args.extend([
        OsString::from("results-cache"),
        OsString::from("--uri"),
        OsString::from(result_cache_uri),
        OsString::from("--collection"),
        OsString::from(query_job_id.to_string()),
    ]);
    if let Some(max_num_results) = clp_s_query_option.max_num_results {
        args.push(OsString::from("--max-num-results"));
        args.push(OsString::from(max_num_results.to_string()));
    }
    args.extend([OsString::from("--dataset"), OsString::from(dataset)]);
    args
}

/// Runs clp-s with the given search arguments, blocking until it exits.
///
/// # Observability
///
/// This method logs errors on failure before returning to the caller. `clp-s`' stderr is logged if
/// successfully captured.
///
/// # Errors
///
/// Returns an error if:
///
/// * clp-s exits with a non-zero status.
/// * Forwards [`Command::spawn`]'s return values on failure.
/// * Forwards [`std::process::Child::wait`]'s return values on failure.
///
/// # Panics
///
/// Panics if clp-s's piped stderr is unexpectedly absent (should be unreachable).
fn run_clp_s_search(
    clp_s_bin: &Path,
    args: Vec<OsString>,
    credential_env: &[(&'static str, String)],
) -> anyhow::Result<()> {
    let mut child = Command::new(clp_s_bin)
        .args(args)
        .envs(credential_env.iter().cloned())
        .stdout(Stdio::null())
        .stderr(Stdio::piped())
        .spawn()
        .with_context(|| format!("failed to spawn clp-s at {}", clp_s_bin.display()))
        .inspect_err(|e| {
            tracing::error!(
                error = % e,
                clp_s_bin = % clp_s_bin.display(),
                "Failed to spawn clp-s.",
            );
        })?;

    let mut stderr = child
        .stderr
        .take()
        .expect("piped stderr should always be present");

    let mut captured_stderr = String::new();
    if let Err(e) = stderr.read_to_string(&mut captured_stderr) {
        captured_stderr = format!("failed to read clp-s stderr: {e}");
    }

    let status = child
        .wait()
        .context("failed to wait for clp-s to exit")
        .inspect_err(|e| {
            tracing::error!(
                error = % e,
                stderr = % captured_stderr,
                "Failed to wait for clp-s to exit."
            );
        })?;
    if !status.success() {
        tracing::error!(
            status = status.code(),
            stderr = % captured_stderr,
            "clp-s exited on failure."
        );
        anyhow::bail!("clp-s exited on error with status={status}");
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use std::ffi::OsString;
    use std::num::NonZeroU32;
    use std::path::Path;
    use std::path::PathBuf;

    use clp_rust_utils::clp_config::AwsAuthentication;
    use clp_rust_utils::clp_config::AwsCredentials;
    use clp_rust_utils::clp_config::S3Config;
    use clp_rust_utils::clp_config::package::config::ArchiveOutput;
    use clp_rust_utils::clp_config::package::config::ArchiveOutputStorage;
    use clp_rust_utils::clp_config::package::config::Package;
    use clp_rust_utils::clp_config::package::config::SpiderTaskExecutorConfig;
    use clp_rust_utils::clp_config::package::config::StorageEngine;
    use clp_rust_utils::task_io::query::ClpSQueryOption;
    use clp_rust_utils::task_io::query::OutputHandle;
    use clp_rust_utils::types::non_empty_string::ExpectedNonEmpty;
    use non_empty_string::NonEmptyString;
    use spider_core::types::id::JobId;
    use spider_core::types::id::ResourceGroupId;
    use spider_core::types::id::TaskId;
    use spider_tdl::TaskContext;

    use super::ArchiveSelector;
    use super::build_clp_s_search_args_for_result_cache;
    use super::resolve_archive_input;
    use super::search;

    /// # Returns
    ///
    /// A query option with no timestamp bounds, no result limit, and case-sensitive matching.
    fn unbounded_query_option() -> ClpSQueryOption {
        ClpSQueryOption {
            query_string: NonEmptyString::from_static_str("level: \"ERROR\""),
            max_num_results: None,
            begin_timestamp_millisecs: None,
            end_timestamp_millisecs: None,
            ignore_case: false,
        }
    }

    /// # Returns
    ///
    /// The AWS credentials the S3-backed test configs authenticate with.
    fn test_aws_authentication() -> AwsAuthentication {
        AwsAuthentication::Credentials {
            credentials: AwsCredentials {
                access_key_id: "the-access-key".to_string(),
                secret_access_key: "the-secret-key".to_string(),
                session_token: None,
            },
        }
    }

    /// # Returns
    ///
    /// An [`ArchiveSelector`] addressing `archive-id` under `/archives/ds1`.
    fn directory_selector() -> ArchiveSelector {
        ArchiveSelector::Directory {
            path: PathBuf::from("/archives/ds1"),
            archive_id: "archive-id".to_string(),
        }
    }

    /// # Returns
    ///
    /// A [`SpiderTaskExecutorConfig`] whose archive output is S3-backed with the given staging
    /// directory and endpoint URL.
    ///
    /// # Panics
    ///
    /// Panics if any of the static S3 config strings is empty.
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
                        bucket: NonEmptyString::from_static_str("bucket"),
                        region_code: None,
                        key_prefix: NonEmptyString::from_static_str("LIB1/"),
                        endpoint_url: endpoint_url.map(NonEmptyString::from_static_str),
                        aws_authentication: test_aws_authentication(),
                    },
                },
                ..ArchiveOutput::default()
            },
            ..SpiderTaskExecutorConfig::default()
        }
    }

    /// # Returns
    ///
    /// A [`TaskContext`] for a non-commit task.
    ///
    /// # Panics
    ///
    /// Panics if [`TaskContext::new`] returns an error.
    fn task_context() -> TaskContext {
        TaskContext::new(
            JobId::random(),
            TaskId::Index(0),
            1,
            ResourceGroupId::random(),
            None,
        )
        .expect("a non-commit task context without graph outputs is valid")
    }

    #[test]
    fn build_clp_s_search_args_for_result_cache_fs_with_timestamps_and_ignore_case() {
        let clp_s_query_option = ClpSQueryOption {
            query_string: NonEmptyString::from_static_str("level: \"ERROR\""),
            max_num_results: Some(NonZeroU32::new(7).expect("7 is nonzero")),
            begin_timestamp_millisecs: Some(1_310_138_944_000),
            end_timestamp_millisecs: Some(1_311_208_074_120),
            ignore_case: true,
        };

        assert_eq!(
            build_clp_s_search_args_for_result_cache(
                &directory_selector(),
                &clp_s_query_option,
                "mongodb://results-cache:27017/clp-query-results",
                42,
                "ds1",
            ),
            vec![
                OsString::from("s"),
                OsString::from("/archives/ds1"),
                OsString::from("--archive-id"),
                OsString::from("archive-id"),
                OsString::from("level: \"ERROR\""),
                OsString::from("--tge"),
                OsString::from("1310138944000"),
                OsString::from("--tle"),
                OsString::from("1311208074120"),
                OsString::from("--ignore-case"),
                OsString::from("results-cache"),
                OsString::from("--uri"),
                OsString::from("mongodb://results-cache:27017/clp-query-results"),
                OsString::from("--collection"),
                OsString::from("42"),
                OsString::from("--max-num-results"),
                OsString::from("7"),
                OsString::from("--dataset"),
                OsString::from("ds1"),
            ]
        );
    }

    #[test]
    fn build_clp_s_search_args_for_result_cache_omits_max_num_results_when_unset() {
        let clp_s_query_option = ClpSQueryOption {
            query_string: NonEmptyString::from_static_str("level: \"ERROR\""),
            max_num_results: None,
            begin_timestamp_millisecs: Some(1_310_138_944_000),
            end_timestamp_millisecs: Some(1_311_208_074_120),
            ignore_case: true,
        };

        assert_eq!(
            build_clp_s_search_args_for_result_cache(
                &directory_selector(),
                &clp_s_query_option,
                "mongodb://results-cache:27017/clp-query-results",
                42,
                "ds1",
            ),
            vec![
                OsString::from("s"),
                OsString::from("/archives/ds1"),
                OsString::from("--archive-id"),
                OsString::from("archive-id"),
                OsString::from("level: \"ERROR\""),
                OsString::from("--tge"),
                OsString::from("1310138944000"),
                OsString::from("--tle"),
                OsString::from("1311208074120"),
                OsString::from("--ignore-case"),
                OsString::from("results-cache"),
                OsString::from("--uri"),
                OsString::from("mongodb://results-cache:27017/clp-query-results"),
                OsString::from("--collection"),
                OsString::from("42"),
                OsString::from("--dataset"),
                OsString::from("ds1"),
            ]
        );
    }

    #[test]
    fn build_clp_s_search_args_for_result_cache_fs_without_timestamps_or_ignore_case() {
        assert_eq!(
            build_clp_s_search_args_for_result_cache(
                &directory_selector(),
                &unbounded_query_option(),
                "mongodb://results-cache:27017/clp-query-results",
                42,
                "default",
            ),
            vec![
                OsString::from("s"),
                OsString::from("/archives/ds1"),
                OsString::from("--archive-id"),
                OsString::from("archive-id"),
                OsString::from("level: \"ERROR\""),
                OsString::from("results-cache"),
                OsString::from("--uri"),
                OsString::from("mongodb://results-cache:27017/clp-query-results"),
                OsString::from("--collection"),
                OsString::from("42"),
                OsString::from("--dataset"),
                OsString::from("default"),
            ]
        );
    }

    #[test]
    fn build_clp_s_search_args_for_result_cache_s3_uses_object_url_and_no_archive_id() {
        assert_eq!(
            build_clp_s_search_args_for_result_cache(
                &ArchiveSelector::ObjectUrl(
                    "https://bucket.s3.amazonaws.com/LIB1/ds1/archive-id".to_string()
                ),
                &unbounded_query_option(),
                "mongodb://results-cache:27017/clp-query-results",
                42,
                "ds1",
            ),
            vec![
                OsString::from("s"),
                OsString::from("https://bucket.s3.amazonaws.com/LIB1/ds1/archive-id"),
                OsString::from("--auth"),
                OsString::from("s3"),
                OsString::from("level: \"ERROR\""),
                OsString::from("results-cache"),
                OsString::from("--uri"),
                OsString::from("mongodb://results-cache:27017/clp-query-results"),
                OsString::from("--collection"),
                OsString::from("42"),
                OsString::from("--dataset"),
                OsString::from("ds1"),
            ]
        );
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

        let (selector, credential_env) = resolve_archive_input(
            runtime.handle(),
            Path::new("/clp"),
            &config,
            "ds1",
            "archive-id".to_string(),
        )?;

        let ArchiveSelector::Directory { path, archive_id } = selector else {
            panic!("expected a directory selector");
        };
        assert_eq!(path, PathBuf::from("/clp/var/data/archives/ds1"));
        assert_eq!(archive_id, "archive-id");
        assert_eq!(credential_env, &[]);

        Ok(())
    }

    #[test]
    fn resolve_archive_input_s3_builds_object_url_and_credentials() -> anyhow::Result<()> {
        let runtime = tokio::runtime::Runtime::new().expect("failed to create Tokio runtime");
        let config = s3_backed_config("var/data/staged-archives", None);

        let (selector, credential_env) = resolve_archive_input(
            runtime.handle(),
            Path::new("/clp"),
            &config,
            "ds1",
            "archive-id".to_string(),
        )?;

        let ArchiveSelector::ObjectUrl(url) = selector else {
            panic!("expected an object-URL selector");
        };
        assert_eq!(url, "https://bucket.s3.amazonaws.com/LIB1/ds1/archive-id");
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

        let (selector, _) = resolve_archive_input(
            runtime.handle(),
            Path::new("/clp"),
            &config,
            "ds1",
            "archive-id".to_string(),
        )?;
        let args = build_clp_s_search_args_for_result_cache(
            &selector,
            &unbounded_query_option(),
            "mongodb://results-cache:27017/clp-query-results",
            42,
            "ds1",
        );

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

        let (selector, _) = resolve_archive_input(
            runtime.handle(),
            Path::new("/clp"),
            &config,
            "ds1",
            "archive-id".to_string(),
        )?;

        let ArchiveSelector::ObjectUrl(url) = selector else {
            panic!("expected an object-URL selector");
        };
        assert_eq!(url, "http://minio:9000/bucket/LIB1/ds1/archive-id");

        Ok(())
    }

    #[test]
    fn search_rejects_file_output_handle() {
        let config = SpiderTaskExecutorConfig {
            package: Package {
                storage_engine: StorageEngine::ClpS,
            },
            ..SpiderTaskExecutorConfig::default()
        };

        let error = search(
            &task_context(),
            &config,
            42,
            &unbounded_query_option(),
            &OutputHandle::File,
            None,
            "archive-id".to_string(),
        )
        .expect_err("the file output handler is unsupported");

        assert!(error.to_string().contains("unsupported"));
    }

    #[test]
    fn search_rejects_non_clp_s_storage_engine() {
        let config = SpiderTaskExecutorConfig::default();
        assert_eq!(config.package.storage_engine, StorageEngine::Clp);

        let error = search(
            &task_context(),
            &config,
            42,
            &unbounded_query_option(),
            &OutputHandle::ResultsCache {
                uri: NonEmptyString::from_static_str(
                    "mongodb://results-cache:27017/clp-query-results",
                ),
            },
            None,
            "archive-id".to_string(),
        )
        .expect_err("the clp storage engine is unsupported");

        assert!(error.to_string().contains("clp-s"));
    }
}

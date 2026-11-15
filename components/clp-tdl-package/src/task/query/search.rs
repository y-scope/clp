//! The `clp-s` search worker that queries a single archive.

use std::ffi::OsString;
use std::io::Read;
use std::num::NonZeroU32;
use std::path::Path;
use std::path::PathBuf;
use std::process::Command;
use std::process::Stdio;

use anyhow::Context;
use clp_rust_utils::aws::AWS_DEFAULT_REGION;
use clp_rust_utils::clp_config::package::config::ArchiveOutputStorage;
use clp_rust_utils::clp_config::package::config::ResultsCache;
use clp_rust_utils::clp_config::package::config::SpiderTaskExecutorConfig;
use clp_rust_utils::clp_config::package::config::StorageEngine;
use clp_rust_utils::dataset::DatasetName;
use clp_rust_utils::job_config::Aggregation;
use clp_rust_utils::job_config::NetworkOutput;
use clp_rust_utils::job_config::QueryJobId;
use clp_rust_utils::job_config::QueryOutput;
use clp_rust_utils::job_config::SearchQuery;
use clp_rust_utils::s3::generate_s3_url;
use clp_rust_utils::timeline;
use clp_rust_utils::types::ArchiveId;
use non_empty_string::NonEmptyString;
use spider_core::task::TaskIndex;
use spider_core::types::id::TaskId;

use crate::common::clp_home;
use crate::common::runtime;
use crate::task::utils::clp_binary_path;
use crate::task::utils::s3_credential_env;

/// Searches one archive with clp-s and publishes normal and optional timeline results.
///
/// A pure worker function called by a spider-tdl task wrapper, which formats any returned
/// `anyhow::Error` into a user-space TDL error.
///
/// # Errors
///
/// Returns an error if:
///
/// * The configured storage engine is not [`StorageEngine::ClpS`].
/// * `ctx.task_id` isn't a [`TaskId::Index`].
/// * `output` is [`QueryOutput::File`], which is unsupported.
/// * Forwards [`resolve_archive_input`]'s return values on failure.
/// * Forwards [`run_clp_s_search`]'s return values on failure.
pub(super) fn search(
    ctx: &spider_tdl::TaskContext,
    config: &SpiderTaskExecutorConfig,
    query_job_id: QueryJobId,
    query: &SearchQuery,
    archive_id: ArchiveId,
    dataset: &DatasetName,
    output: &QueryOutput,
) -> anyhow::Result<()> {
    if StorageEngine::ClpS != config.package.storage_engine {
        anyhow::bail!("the clp-s query task requires the `clp-s` storage engine");
    }
    let TaskId::Index(task_index) = ctx.task_id else {
        anyhow::bail!(
            "the clp-s query task requires a task index, got task ID {}",
            ctx.task_id
        );
    };

    if matches!(output, QueryOutput::File {}) {
        anyhow::bail!("unsupported query output: file");
    }

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
    let (archive_descriptor, credential_env) =
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
    let arg_lists = build_clp_s_arg_lists(
        &archive_descriptor,
        query,
        output,
        &config.results_cache,
        query_job_id,
        dataset.as_str(),
        task_index,
    );
    for args in arg_lists {
        run_clp_s_search(&clp_binary_path(clp_home, "clp-s"), args, &credential_env)?;
    }

    tracing::info!(
        job_id = % ctx.job_id,
        task_id = % ctx.task_id,
        task_instance_id = % ctx.task_instance_id,
        query_job_id = % query_job_id,
        "clp-s query task completed successfully.",
    );
    Ok(())
}

/// The archive to search, in the form clp-s addresses it.
enum ArchiveDescriptor {
    /// A local dataset archives directory plus the `--archive-id` selecting one archive in it.
    Directory {
        path: PathBuf,
        archive_id: ArchiveId,
    },

    /// The URL of an S3-hosted archive, read with `--auth s3`.
    ObjectUrl(String),
}

/// Builds arguments for normal results and optional timeline aggregation.
///
/// # Returns
///
/// Ordered argument lists, each corresponding to an individual `clp-s` invocation.
///
/// # Panics
///
/// Panics if `output` is [`QueryOutput::File`], which the caller must reject.
fn build_clp_s_arg_lists(
    archive_descriptor: &ArchiveDescriptor,
    query: &SearchQuery,
    output: &QueryOutput,
    results_cache: &ResultsCache,
    query_job_id: QueryJobId,
    dataset: &str,
    task_index: TaskIndex,
) -> Vec<Vec<OsString>> {
    let mut normal_args = build_clp_s_query_args(archive_descriptor, query);
    match output {
        QueryOutput::ResultsCache(output) => {
            normal_args.extend(build_clp_s_results_cache_output_args(
                results_cache,
                output.max_num_results,
                &query_job_id.to_string(),
                dataset,
            ));
            let mut arg_lists = vec![normal_args];
            if let Some(Aggregation::CountByTime {
                bucket_size_millisecs,
            }) = output.aggregation
            {
                let mut timeline_args = build_clp_s_query_args(archive_descriptor, query);
                timeline_args.extend([
                    OsString::from("--count-by-time"),
                    OsString::from(bucket_size_millisecs.get().to_string()),
                ]);
                timeline_args.extend(build_clp_s_results_cache_output_args(
                    results_cache,
                    None,
                    &timeline::collection_name(query_job_id),
                    dataset,
                ));
                arg_lists.push(timeline_args);
            }
            arg_lists
        }
        QueryOutput::Network(network_output) => {
            normal_args.extend(build_clp_s_network_output_args(network_output, task_index));
            vec![normal_args]
        }
        QueryOutput::File {} => {
            unreachable!("file output must be rejected before building arguments")
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
fn resolve_archive_input(
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

/// Builds the clp-s command-line arguments of the `results-cache` output handler.
///
/// # Returns
///
/// The ordered clp-s arguments, which follow the search's arguments.
fn build_clp_s_results_cache_output_args(
    results_cache: &ResultsCache,
    max_num_results: Option<NonZeroU32>,
    collection: &str,
    dataset: &str,
) -> Vec<OsString> {
    let mut args = vec![
        OsString::from("results-cache"),
        OsString::from("--uri"),
        OsString::from(results_cache.uri().as_str()),
        OsString::from("--collection"),
        OsString::from(collection),
    ];
    if let Some(max_num_results) = max_num_results {
        args.push(OsString::from("--max-num-results"));
        args.push(OsString::from(max_num_results.to_string()));
    }
    args.extend([OsString::from("--dataset"), OsString::from(dataset)]);
    args
}

/// Builds the clp-s command-line arguments of the `network` output handler.
///
/// # Returns
///
/// The ordered clp-s arguments, which follow the search's arguments.
fn build_clp_s_network_output_args(
    network_output: &NetworkOutput,
    task_index: TaskIndex,
) -> Vec<OsString> {
    vec![
        OsString::from("network"),
        OsString::from("--host"),
        OsString::from(network_output.host.as_str()),
        OsString::from("--port"),
        OsString::from(network_output.port.to_string()),
        OsString::from("--session-token"),
        OsString::from(network_output.session_token.to_string()),
        OsString::from("--task-index"),
        OsString::from(task_index.to_string()),
    ]
}

/// Builds the clp-s command-line arguments that select the archive and the query to run on it,
/// shared by every output handler.
///
/// # Returns
///
/// The ordered clp-s arguments, which the output handler's arguments follow.
fn build_clp_s_query_args(
    archive_descriptor: &ArchiveDescriptor,
    query: &SearchQuery,
) -> Vec<OsString> {
    let mut args = vec![OsString::from("s")];
    match archive_descriptor {
        ArchiveDescriptor::Directory { path, archive_id } => {
            args.push(path.as_os_str().to_os_string());
            args.push(OsString::from("--archive-id"));
            args.push(OsString::from(archive_id.to_string()));
        }
        ArchiveDescriptor::ObjectUrl(url) => {
            args.push(OsString::from(url));
            args.push(OsString::from("--auth"));
            args.push(OsString::from("s3"));
        }
    }

    args.push(OsString::from(query.query_string.as_str()));
    if let Some(begin_timestamp_millisecs) = query.time_range.begin_timestamp_millisecs() {
        args.push(OsString::from("--tge"));
        args.push(OsString::from(begin_timestamp_millisecs.to_string()));
    }
    if let Some(end_timestamp_millisecs) = query.time_range.end_timestamp_millisecs() {
        args.push(OsString::from("--tle"));
        args.push(OsString::from(end_timestamp_millisecs.to_string()));
    }
    if query.ignore_case {
        args.push(OsString::from("--ignore-case"));
    }
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
    use std::num::NonZeroU16;
    use std::num::NonZeroU32;
    use std::path::Path;
    use std::path::PathBuf;

    use clp_rust_utils::clp_config::AwsAuthentication;
    use clp_rust_utils::clp_config::AwsCredentials;
    use clp_rust_utils::clp_config::S3Config;
    use clp_rust_utils::clp_config::package::config::ArchiveOutput;
    use clp_rust_utils::clp_config::package::config::ArchiveOutputStorage;
    use clp_rust_utils::clp_config::package::config::Package;
    use clp_rust_utils::clp_config::package::config::ResultsCache;
    use clp_rust_utils::clp_config::package::config::SpiderTaskExecutorConfig;
    use clp_rust_utils::clp_config::package::config::StorageEngine;
    use clp_rust_utils::dataset::DatasetName;
    use clp_rust_utils::dataset::NonEmptyDatasetNames;
    use clp_rust_utils::job_config::Aggregation;
    use clp_rust_utils::job_config::NetworkOutput;
    use clp_rust_utils::job_config::QueryOutput;
    use clp_rust_utils::job_config::ResultsCacheOutput;
    use clp_rust_utils::job_config::SearchQuery;
    use clp_rust_utils::job_config::TimeRange;
    use clp_rust_utils::types::ArchiveId;
    use clp_rust_utils::types::PositiveI64;
    use clp_rust_utils::types::SessionToken;
    use clp_rust_utils::types::non_empty_string::ExpectedNonEmpty;
    use non_empty_string::NonEmptyString;
    use spider_core::types::id::JobId;
    use spider_core::types::id::ResourceGroupId;
    use spider_core::types::id::TaskId;
    use spider_tdl::TaskContext;

    use super::ArchiveDescriptor;
    use super::build_clp_s_arg_lists;
    use super::build_clp_s_network_output_args;
    use super::build_clp_s_query_args;
    use super::build_clp_s_results_cache_output_args;
    use super::resolve_archive_input;
    use super::search;

    const ARCHIVE_ID: &str = "018e90e5-8b2a-4a61-a2fc-cac799936caf";
    const SESSION_TOKEN: &str = "6f1d3b52-8a4e-4c1b-9f6e-2d7a5c0b9e13";

    /// # Returns
    ///
    /// A search query with no timestamp bounds and case-sensitive matching.
    ///
    /// # Panics
    ///
    /// Panics if the dataset names are empty.
    fn unbounded_query() -> SearchQuery {
        SearchQuery {
            datasets: NonEmptyDatasetNames::try_from(vec![DatasetName::default()])
                .expect("dataset names should be non-empty"),
            query_string: NonEmptyString::from_static_str("level: \"ERROR\""),
            time_range: TimeRange::default(),
            ignore_case: false,
        }
    }

    /// # Returns
    ///
    /// The results cache config whose URI is `mongodb://results-cache:27017/clp-query-results`.
    fn results_cache() -> ResultsCache {
        ResultsCache {
            host: "results-cache".to_owned(),
            ..ResultsCache::default()
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
    /// An [`ArchiveDescriptor`] addressing [`ARCHIVE_ID`] under `/archives/ds1`.
    fn directory_descriptor() -> ArchiveDescriptor {
        ArchiveDescriptor::Directory {
            path: PathBuf::from("/archives/ds1"),
            archive_id: ARCHIVE_ID.parse::<ArchiveId>().expect("valid archive UUID"),
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
    /// A network output to a listener at `10.0.0.7:40123`, with [`SESSION_TOKEN`] as its session
    /// token.
    ///
    /// # Panics
    ///
    /// Panics if [`SESSION_TOKEN`] isn't a valid UUID.
    fn network_output() -> NetworkOutput {
        NetworkOutput {
            host: NonEmptyString::from_static_str("10.0.0.7"),
            port: NonZeroU16::new(40_123).expect("40,123 is nonzero"),
            session_token: SessionToken::from(
                uuid::Uuid::parse_str(SESSION_TOKEN).expect("valid session token UUID"),
            ),
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
    fn build_clp_s_query_args_fs_with_timestamps_and_ignore_case() {
        let query = SearchQuery {
            time_range: TimeRange::new(Some(1_310_138_944_000), Some(1_311_208_074_120))
                .expect("range is ordered"),
            ignore_case: true,
            ..unbounded_query()
        };

        assert_eq!(
            build_clp_s_query_args(&directory_descriptor(), &query),
            vec![
                OsString::from("s"),
                OsString::from("/archives/ds1"),
                OsString::from("--archive-id"),
                OsString::from(ARCHIVE_ID),
                OsString::from("level: \"ERROR\""),
                OsString::from("--tge"),
                OsString::from("1310138944000"),
                OsString::from("--tle"),
                OsString::from("1311208074120"),
                OsString::from("--ignore-case"),
            ]
        );
    }

    #[test]
    fn build_clp_s_query_args_s3_uses_object_url_and_no_archive_id() {
        let url = format!("https://bucket.s3.amazonaws.com/LIB1/ds1/{ARCHIVE_ID}");

        assert_eq!(
            build_clp_s_query_args(
                &ArchiveDescriptor::ObjectUrl(url.clone()),
                &unbounded_query()
            ),
            vec![
                OsString::from("s"),
                OsString::from(url),
                OsString::from("--auth"),
                OsString::from("s3"),
                OsString::from("level: \"ERROR\""),
            ]
        );
    }

    #[test]
    fn build_clp_s_results_cache_output_args_with_max_num_results() {
        assert_eq!(
            build_clp_s_results_cache_output_args(
                &results_cache(),
                NonZeroU32::new(7),
                "42",
                "ds1"
            ),
            vec![
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
    fn build_clp_s_results_cache_output_args_omits_max_num_results_when_unset() {
        assert_eq!(
            build_clp_s_results_cache_output_args(&results_cache(), None, "42", "ds1"),
            vec![
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
    fn build_clp_s_network_output_args_names_the_listener_and_the_task() {
        assert_eq!(
            build_clp_s_network_output_args(&network_output(), 5),
            vec![
                OsString::from("network"),
                OsString::from("--host"),
                OsString::from("10.0.0.7"),
                OsString::from("--port"),
                OsString::from("40123"),
                OsString::from("--session-token"),
                OsString::from(SESSION_TOKEN),
                OsString::from("--task-index"),
                OsString::from("5"),
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
        let args = build_clp_s_query_args(&descriptor, &unbounded_query());

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

    #[test]
    fn search_rejects_unsupported_outputs() {
        let config = SpiderTaskExecutorConfig {
            package: Package {
                storage_engine: StorageEngine::ClpS,
            },
            ..SpiderTaskExecutorConfig::default()
        };

        let output = QueryOutput::File {};
        let error = search(
            &task_context(),
            &config,
            42,
            &unbounded_query(),
            ARCHIVE_ID.parse::<ArchiveId>().expect("valid archive UUID"),
            &DatasetName::default(),
            &output,
        )
        .expect_err("the output should be unsupported");

        assert!(
            error.to_string().contains("unsupported query output"),
            "unexpected error for {output:?}: {error}"
        );
    }

    #[test]
    fn search_rejects_a_task_without_a_task_index() {
        let config = SpiderTaskExecutorConfig {
            package: Package {
                storage_engine: StorageEngine::ClpS,
            },
            ..SpiderTaskExecutorConfig::default()
        };
        let cleanup_task_context = TaskContext::new(
            JobId::random(),
            TaskId::Cleanup,
            1,
            ResourceGroupId::random(),
            None,
        )
        .expect("a cleanup task context without graph outputs is valid");

        let error = search(
            &cleanup_task_context,
            &config,
            42,
            &unbounded_query(),
            ARCHIVE_ID.parse::<ArchiveId>().expect("valid archive UUID"),
            &DatasetName::default(),
            &QueryOutput::Network(network_output()),
        )
        .expect_err("a query task requires a task index");

        assert!(error.to_string().contains("task index"));
    }

    #[test]
    fn search_rejects_non_clp_s_storage_engine() {
        let config = SpiderTaskExecutorConfig {
            package: Package {
                storage_engine: StorageEngine::Clp,
            },
            ..SpiderTaskExecutorConfig::default()
        };

        let error = search(
            &task_context(),
            &config,
            42,
            &unbounded_query(),
            ARCHIVE_ID.parse::<ArchiveId>().expect("valid archive UUID"),
            &DatasetName::default(),
            &QueryOutput::ResultsCache(ResultsCacheOutput {
                max_num_results: None,
                aggregation: None,
            }),
        )
        .expect_err("the clp storage engine is unsupported");

        assert!(error.to_string().contains("clp-s"));
    }

    #[test]
    fn timeline_search_preserves_filters_and_omits_result_limit() -> anyhow::Result<()> {
        let query = SearchQuery {
            time_range: TimeRange::new(Some(-1), Some(120_000))?,
            ignore_case: true,
            ..unbounded_query()
        };
        for max_num_results in [None, NonZeroU32::new(7)] {
            let output = QueryOutput::ResultsCache(ResultsCacheOutput {
                max_num_results,
                aggregation: Some(Aggregation::CountByTime {
                    bucket_size_millisecs: PositiveI64::new(60_000).expect("60,000 is positive"),
                }),
            });
            let arg_lists = build_clp_s_arg_lists(
                &directory_descriptor(),
                &query,
                &output,
                &results_cache(),
                42,
                "ds1",
                0,
            );
            assert_eq!(arg_lists.len(), 2);
            let args = &arg_lists[1];
            let mut expected_normal = [
                "s",
                "/archives/ds1",
                "--archive-id",
                ARCHIVE_ID,
                "level: \"ERROR\"",
                "--tge",
                "-1",
                "--tle",
                "120000",
                "--ignore-case",
                "results-cache",
                "--uri",
                "mongodb://results-cache:27017/clp-query-results",
                "--collection",
                "42",
            ]
            .map(OsString::from)
            .to_vec();
            if max_num_results.is_some() {
                expected_normal.extend(["--max-num-results", "7"].map(OsString::from));
            }
            expected_normal.extend(["--dataset", "ds1"].map(OsString::from));
            assert_eq!(arg_lists[0], expected_normal);
            let expected = [
                "s",
                "/archives/ds1",
                "--archive-id",
                ARCHIVE_ID,
                "level: \"ERROR\"",
                "--tge",
                "-1",
                "--tle",
                "120000",
                "--ignore-case",
                "--count-by-time",
                "60000",
                "results-cache",
                "--uri",
                "mongodb://results-cache:27017/clp-query-results",
                "--collection",
                "42-timeline",
                "--dataset",
                "ds1",
            ];
            anyhow::ensure!(
                *args == expected.map(OsString::from),
                "unexpected timeline arguments: {args:?}"
            );
        }
        Ok(())
    }

    #[test]
    fn timeline_search_preserves_s3_authentication_and_large_bucket_width() -> anyhow::Result<()> {
        let output = QueryOutput::ResultsCache(ResultsCacheOutput {
            max_num_results: NonZeroU32::new(7),
            aggregation: Some(Aggregation::CountByTime {
                bucket_size_millisecs: PositiveI64::new(i64::MAX).expect("i64::MAX is positive"),
            }),
        });
        let arg_lists = build_clp_s_arg_lists(
            &ArchiveDescriptor::ObjectUrl("https://bucket.s3.amazonaws.com/archive".to_owned()),
            &unbounded_query(),
            &output,
            &results_cache(),
            42,
            "ds1",
            0,
        );
        let args = &arg_lists[1];
        let expected = [
            "s",
            "https://bucket.s3.amazonaws.com/archive",
            "--auth",
            "s3",
            "level: \"ERROR\"",
            "--count-by-time",
            "9223372036854775807",
            "results-cache",
            "--uri",
            "mongodb://results-cache:27017/clp-query-results",
            "--collection",
            "42-timeline",
            "--dataset",
            "ds1",
        ];
        anyhow::ensure!(
            *args == expected.map(OsString::from),
            "unexpected timeline arguments: {args:?}"
        );
        Ok(())
    }
}

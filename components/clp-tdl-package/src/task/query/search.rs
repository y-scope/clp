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
use clp_rust_utils::clp_config::S3Config;
use clp_rust_utils::clp_config::package::config::ArchiveOutputStorage;
use clp_rust_utils::clp_config::package::config::ResultsCache;
use clp_rust_utils::clp_config::package::config::SpiderTaskExecutorConfig;
use clp_rust_utils::clp_config::package::config::StorageEngine;
use clp_rust_utils::clp_config::package::config::StreamOutputStorage;
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
use crate::task::utils::build_s3_client;
use crate::task::utils::clp_binary_path;
use crate::task::utils::s3_credential_env;
use crate::task::utils::upload_file_to_s3;

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
/// * Forwards [`resolve_archive_input`]'s return values on failure.
/// * Forwards [`ResultsFile::create`]'s return values on failure.
/// * Forwards [`run_clp_s_search`]'s return values on failure.
/// * Forwards [`ResultsFile::publish`]'s return values on failure.
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
    let runtime = runtime();
    let (archive_descriptor, credential_env) = resolve_archive_input(
        &runtime, clp_home, config, dataset, archive_id,
    )
    .inspect_err(|e| {
        tracing::error!(
            job_id = % ctx.job_id,
            task_id = % ctx.task_id,
            task_instance_id = % ctx.task_instance_id,
            query_job_id = % query_job_id,
            error = % e,
            "Failed to resolve the archive input."
        );
    })?;
    let results_file = match output {
        QueryOutput::File {} => Some(
            ResultsFile::create(clp_home, config, query_job_id, archive_id).inspect_err(|e| {
                tracing::error!(
                    job_id = % ctx.job_id,
                    task_id = % ctx.task_id,
                    task_instance_id = % ctx.task_instance_id,
                    query_job_id = % query_job_id,
                    error = % e,
                    "Failed to create the query results file's directories."
                );
            })?,
        ),
        QueryOutput::ResultsCache(_) | QueryOutput::Network(_) => None,
    };
    let arg_lists = build_clp_s_arg_lists(
        build_clp_s_query_args(&archive_descriptor, query),
        output,
        &config.results_cache,
        results_file
            .as_ref()
            .map(|results_file| results_file.staging_path.as_path()),
        query_job_id,
        dataset.as_str(),
        task_index,
    );
    let clp_s_bin = clp_binary_path(clp_home, "clp-s");
    for args in arg_lists {
        run_clp_s_search(&clp_s_bin, args, &credential_env)?;
    }
    if let Some(results_file) = results_file {
        results_file.publish(&runtime).inspect_err(|e| {
            tracing::error!(
                job_id = % ctx.job_id,
                task_id = % ctx.task_id,
                task_instance_id = % ctx.task_instance_id,
                query_job_id = % query_job_id,
                error = % e,
                "Failed to publish the query results file."
            );
        })?;
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

/// The file that clp-s's `file` output handler writes one archive's query results to, staged under
/// a unique name and then published to its destination.
///
/// The staged file is removed when the [`ResultsFile`] is dropped.
///
/// # Type Parameters
///
/// * `'config` - The lifetime of the executor config that the results file borrows.
struct ResultsFile<'config> {
    staging_path: PathBuf,
    destination: ResultsFileDestination<'config>,
}

impl<'config> ResultsFile<'config> {
    /// Factory function.
    ///
    /// Creates the staging directory and, for filesystem stream output, the query job's directory
    /// that the results file is published to.
    ///
    /// # Returns
    ///
    /// A newly created [`ResultsFile`] on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`std::fs::create_dir_all`]'s return values on failure.
    fn create(
        clp_home: &Path,
        config: &'config SpiderTaskExecutorConfig,
        query_job_id: QueryJobId,
        archive_id: ArchiveId,
    ) -> anyhow::Result<Self> {
        const STAGING_DIRECTORY_NAME: &str = ".staging";

        let create_directory = |directory: &Path| {
            std::fs::create_dir_all(directory).with_context(|| {
                format!(
                    "failed to create the query results directory {}",
                    directory.display()
                )
            })
        };
        let local_directory = config.abs_stream_output_staging(clp_home);
        let staging_directory = local_directory.join(STAGING_DIRECTORY_NAME);
        create_directory(&staging_directory)?;
        let destination = match &config.stream_output.storage {
            StreamOutputStorage::Fs { .. } => {
                let query_job_directory = local_directory.join(query_job_id.to_string());
                create_directory(&query_job_directory)?;
                ResultsFileDestination::Fs {
                    path: query_job_directory.join(archive_id.to_string()),
                }
            }
            StreamOutputStorage::S3 { s3_config, .. } => ResultsFileDestination::S3 {
                s3_config,
                object_key: format!("{}{query_job_id}/{archive_id}", s3_config.key_prefix),
            },
        };
        Ok(Self {
            staging_path: staging_directory.join(format!(
                "{query_job_id}.{archive_id}.{}",
                uuid::Uuid::new_v4()
            )),
            destination,
        })
    }

    /// Publishes the staged results file to its destination once clp-s has written it, unless
    /// clp-s didn't create it or left it empty.
    ///
    /// The file is moved to a filesystem destination, or uploaded to an S3 destination.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`std::fs::metadata`]'s return values on failure, unless the staged file is not
    ///   found.
    /// * Forwards [`std::fs::rename`]'s return values on failure.
    /// * Forwards [`upload_file_to_s3`]'s return values on failure.
    fn publish(self, runtime: &tokio::runtime::Handle) -> anyhow::Result<()> {
        let is_missing_or_empty = match std::fs::metadata(&self.staging_path) {
            Ok(metadata) => 0 == metadata.len(),
            Err(e) if std::io::ErrorKind::NotFound == e.kind() => true,
            Err(e) => {
                return Err(e).with_context(|| {
                    format!(
                        "failed to read the metadata of {}",
                        self.staging_path.display()
                    )
                });
            }
        };
        if is_missing_or_empty {
            return Ok(());
        }
        match &self.destination {
            ResultsFileDestination::Fs { path } => std::fs::rename(&self.staging_path, path)
                .with_context(|| {
                    format!(
                        "failed to move {} to {}",
                        self.staging_path.display(),
                        path.display()
                    )
                }),
            ResultsFileDestination::S3 {
                s3_config,
                object_key,
            } => {
                let client = build_s3_client(runtime, s3_config);
                runtime.block_on(upload_file_to_s3(
                    &client,
                    s3_config.bucket.as_str(),
                    object_key,
                    &self.staging_path,
                ))
            }
        }
    }
}

impl Drop for ResultsFile<'_> {
    fn drop(&mut self) {
        match std::fs::remove_file(&self.staging_path) {
            Ok(()) => {}
            Err(e) if std::io::ErrorKind::NotFound == e.kind() => {}
            Err(e) => {
                tracing::warn!(
                    error = % e,
                    path = % self.staging_path.display(),
                    "Failed to remove the staged query results file."
                );
            }
        }
    }
}

/// Where a [`ResultsFile`] is published.
///
/// # Type Parameters
///
/// * `'config` - The lifetime of the S3 config that the destination borrows.
enum ResultsFileDestination<'config> {
    /// A file in the query job's directory under the stream output directory.
    Fs { path: PathBuf },

    /// An object in the stream output S3 bucket.
    S3 {
        s3_config: &'config S3Config,
        object_key: String,
    },
}

/// Builds arguments for normal results and optional timeline aggregation, each starting with
/// `query_args`.
///
/// # Returns
///
/// Ordered argument lists, each corresponding to an individual `clp-s` invocation.
///
/// # Panics
///
/// Panics if `output` is [`QueryOutput::File`] but `results_file_path` is `None`.
fn build_clp_s_arg_lists(
    mut query_args: Vec<OsString>,
    output: &QueryOutput,
    results_cache: &ResultsCache,
    results_file_path: Option<&Path>,
    query_job_id: QueryJobId,
    dataset: &str,
    task_index: TaskIndex,
) -> Vec<Vec<OsString>> {
    match output {
        QueryOutput::ResultsCache(output) => {
            let mut normal_args = query_args.clone();
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
                let mut timeline_args = query_args;
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
            query_args.extend(build_clp_s_network_output_args(network_output, task_index));
            vec![query_args]
        }
        QueryOutput::File {} => {
            let results_file_path =
                results_file_path.expect("file output requires a results file path");
            query_args.extend(build_clp_s_file_output_args(results_file_path));
            vec![query_args]
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

/// Builds the clp-s command-line arguments of the `file` output handler.
///
/// # Returns
///
/// The ordered clp-s arguments, which follow the search's arguments.
fn build_clp_s_file_output_args(results_file_path: &Path) -> Vec<OsString> {
    vec![
        OsString::from("file"),
        OsString::from("--path"),
        results_file_path.as_os_str().to_os_string(),
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
    use clp_rust_utils::clp_config::package::config::StreamOutput;
    use clp_rust_utils::clp_config::package::config::StreamOutputStorage;
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
    use super::ResultsFile;
    use super::ResultsFileDestination;
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
    /// An [`S3Config`] for the bucket `bucket` with the key prefix `LIB1/` and no endpoint URL,
    /// which authenticates with static AWS credentials.
    fn base_s3_config() -> S3Config {
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

    /// # Returns
    ///
    /// A [`SpiderTaskExecutorConfig`] whose stream output is S3-backed with a relative staging
    /// directory, the key prefix `streams/`, and an endpoint that refuses connections.
    fn s3_stream_output_config() -> SpiderTaskExecutorConfig {
        SpiderTaskExecutorConfig {
            stream_output: StreamOutput {
                storage: StreamOutputStorage::S3 {
                    staging_directory: "var/data/staged-streams".to_owned(),
                    s3_config: S3Config {
                        key_prefix: NonEmptyString::from_static_str("streams/"),
                        endpoint_url: Some(NonEmptyString::from_static_str("http://127.0.0.1:1")),
                        ..base_s3_config()
                    },
                },
                ..StreamOutput::default()
            },
            ..SpiderTaskExecutorConfig::default()
        }
    }

    /// # Type Parameters
    ///
    /// * `'config` - The lifetime of the executor config that the results file borrows.
    ///
    /// # Returns
    ///
    /// A [`ResultsFile`] for query job 42 and [`ARCHIVE_ID`], created under `clp_home`.
    ///
    /// # Panics
    ///
    /// Panics if [`ARCHIVE_ID`] isn't a valid UUID, or if [`ResultsFile::create`] returns an error.
    fn create_results_file<'config>(
        clp_home: &Path,
        config: &'config SpiderTaskExecutorConfig,
    ) -> ResultsFile<'config> {
        ResultsFile::create(
            clp_home,
            config,
            42,
            ARCHIVE_ID.parse::<ArchiveId>().expect("valid archive UUID"),
        )
        .expect("failed to create the results file")
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
    fn build_clp_s_arg_lists_file_output_writes_to_the_results_file_path() {
        let results_file_path = PathBuf::from("/results");

        assert_eq!(
            build_clp_s_arg_lists(
                build_clp_s_query_args(&directory_descriptor(), &unbounded_query()),
                &QueryOutput::File {},
                &results_cache(),
                Some(&results_file_path),
                42,
                "ds1",
                0,
            ),
            vec![vec![
                OsString::from("s"),
                OsString::from("/archives/ds1"),
                OsString::from("--archive-id"),
                OsString::from(ARCHIVE_ID),
                OsString::from("level: \"ERROR\""),
                OsString::from("file"),
                OsString::from("--path"),
                OsString::from(results_file_path),
            ]]
        );
    }

    #[test]
    fn results_file_fs_is_staged_uniquely_outside_the_query_job_directory() -> anyhow::Result<()> {
        let clp_home = tempfile::tempdir()?;
        let config = SpiderTaskExecutorConfig::default();

        let results_file = create_results_file(clp_home.path(), &config);
        let other_results_file = create_results_file(clp_home.path(), &config);

        let staging_directory = config
            .abs_stream_output_staging(clp_home.path())
            .join(".staging");
        assert_eq!(
            results_file.staging_path.parent(),
            Some(staging_directory.as_path())
        );
        let staging_file_name = results_file
            .staging_path
            .file_name()
            .expect("the staging path should have a file name")
            .to_string_lossy();
        assert!(
            staging_file_name.starts_with(&format!("42.{ARCHIVE_ID}.")),
            "unexpected staging file name: {staging_file_name}"
        );
        assert_ne!(results_file.staging_path, other_results_file.staging_path);
        assert!(staging_directory.is_dir());

        Ok(())
    }

    #[test]
    fn results_file_s3_is_keyed_by_query_job_and_archive_without_a_local_query_job_directory()
    -> anyhow::Result<()> {
        let clp_home = tempfile::tempdir()?;
        let config = s3_stream_output_config();

        let results_file = create_results_file(clp_home.path(), &config);

        let ResultsFileDestination::S3 { object_key, .. } = &results_file.destination else {
            panic!("expected an S3 destination");
        };
        assert_eq!(object_key, &format!("streams/42/{ARCHIVE_ID}"));
        assert!(
            !config
                .abs_stream_output_staging(clp_home.path())
                .join("42")
                .exists()
        );

        Ok(())
    }

    #[test]
    fn dropping_a_results_file_removes_the_staged_file() -> anyhow::Result<()> {
        let clp_home = tempfile::tempdir()?;
        let config = SpiderTaskExecutorConfig::default();
        let results_file = create_results_file(clp_home.path(), &config);
        let staging_path = results_file.staging_path.clone();
        std::fs::write(&staging_path, b"results")?;

        drop(results_file);

        assert!(!staging_path.exists());
        Ok(())
    }

    #[test]
    fn publish_moves_the_staged_fs_results_file_to_its_destination() -> anyhow::Result<()> {
        let clp_home = tempfile::tempdir()?;
        let config = SpiderTaskExecutorConfig::default();
        let runtime = tokio::runtime::Runtime::new().expect("failed to create Tokio runtime");
        let results_file = create_results_file(clp_home.path(), &config);
        let staging_path = results_file.staging_path.clone();
        std::fs::write(&staging_path, b"results")?;

        results_file.publish(runtime.handle())?;

        let path = config
            .abs_stream_output_staging(clp_home.path())
            .join(format!("42/{ARCHIVE_ID}"));
        assert_eq!(std::fs::read(path)?, b"results");
        Ok(())
    }

    #[test]
    fn publish_creates_no_fs_destination_file_if_the_staged_results_file_is_missing_or_empty()
    -> anyhow::Result<()> {
        let clp_home = tempfile::tempdir()?;
        let config = SpiderTaskExecutorConfig::default();
        let runtime = tokio::runtime::Runtime::new().expect("failed to create Tokio runtime");
        let path = config
            .abs_stream_output_staging(clp_home.path())
            .join(format!("42/{ARCHIVE_ID}"));

        create_results_file(clp_home.path(), &config).publish(runtime.handle())?;
        assert!(!path.exists());

        let results_file = create_results_file(clp_home.path(), &config);
        std::fs::write(&results_file.staging_path, b"")?;
        results_file.publish(runtime.handle())?;
        assert!(!path.exists());
        Ok(())
    }

    #[test]
    fn publish_skips_uploading_an_empty_s3_results_file() -> anyhow::Result<()> {
        let clp_home = tempfile::tempdir()?;
        let config = s3_stream_output_config();
        let runtime = tokio::runtime::Runtime::new().expect("failed to create Tokio runtime");
        let results_file = create_results_file(clp_home.path(), &config);
        let staging_path = results_file.staging_path.clone();
        std::fs::write(&staging_path, b"")?;

        results_file.publish(runtime.handle())?;

        Ok(())
    }

    #[test]
    fn publish_fails_if_the_staged_results_file_cannot_be_inspected() -> anyhow::Result<()> {
        let clp_home = tempfile::tempdir()?;
        let config = s3_stream_output_config();
        let runtime = tokio::runtime::Runtime::new().expect("failed to create Tokio runtime");
        let not_a_directory = clp_home.path().join("not-a-directory");
        std::fs::write(&not_a_directory, b"")?;
        let mut results_file = create_results_file(clp_home.path(), &config);
        results_file.staging_path = not_a_directory.join(format!("42.{ARCHIVE_ID}"));

        let error = results_file
            .publish(runtime.handle())
            .expect_err("an uninspectable staged file should fail the publication");

        assert!(
            error.to_string().contains("failed to read the metadata"),
            "unexpected error: {error}"
        );
        Ok(())
    }

    #[test]
    fn publish_fails_on_an_s3_upload_error() -> anyhow::Result<()> {
        let clp_home = tempfile::tempdir()?;
        let config = s3_stream_output_config();
        let runtime = tokio::runtime::Runtime::new().expect("failed to create Tokio runtime");
        let results_file = create_results_file(clp_home.path(), &config);
        let staging_path = results_file.staging_path.clone();
        std::fs::write(&staging_path, b"results")?;

        let error = results_file
            .publish(runtime.handle())
            .expect_err("an unreachable S3 endpoint should fail the publication");

        assert!(
            error.to_string().contains("failed to upload"),
            "unexpected error: {error}"
        );
        Ok(())
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
                build_clp_s_query_args(&directory_descriptor(), &query),
                &output,
                &results_cache(),
                None,
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
            build_clp_s_query_args(
                &ArchiveDescriptor::ObjectUrl("https://bucket.s3.amazonaws.com/archive".to_owned()),
                &unbounded_query(),
            ),
            &output,
            &results_cache(),
            None,
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

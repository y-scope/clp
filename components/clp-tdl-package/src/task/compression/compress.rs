//! The `clp-s` compression worker that turns an S3 input source into archives.

use std::{
    ffi::OsString,
    io::{BufRead, BufReader, Read},
    path::{Path, PathBuf},
    process::{Command, Stdio},
};

use anyhow::Context;
use clp_rust_utils::{
    aws::AWS_DEFAULT_REGION,
    clp_config::{
        AwsAuthentication,
        S3Config,
        package::config::{
            ArchiveOutput,
            ArchiveOutputStorage,
            Database,
            SpiderTaskExecutorConfig,
        },
    },
    dataset::resolve_dataset_name,
    s3::{create_new_client, generate_s3_url},
    task_io::compression::{
        ArchiveMetadata,
        ClpSCompressionOption,
        CompressionTaskOutput,
        S3InputSource,
    },
};
use non_empty_string::NonEmptyString;

use crate::common::{clp_home, runtime};

/// Compresses the given S3 objects into archives, uploads them to S3, and returns their metadata
/// for the commit task.
///
/// A pure worker function called by a spider-tdl task wrapper, which formats any returned
/// `anyhow::Error` into a user-space TDL error.
///
/// # Returns
///
/// The metadata of every archive this task produced.
///
/// # Errors
///
/// Returns an error if:
///
/// * A spawned archive finisher panics.
/// * Forwards [`std::fs::write`]'s return values on failure.
/// * Forwards [`std::fs::create_dir`]'s return values on failure.
/// * Forwards [`extract_s3_output_config`]'s return values on failure.
/// * Forwards [`run_log_converter`]'s return values on failure.
/// * Forwards [`run_clp_s`]'s return values on failure.
/// * Forwards [`ArchiveFinisher::finish`]'s return values on failure.
pub(super) fn compress(
    ctx: &spider_tdl::TaskContext,
    config: &SpiderTaskExecutorConfig,
    clp_s_option: &ClpSCompressionOption,
    dataset: Option<String>,
    input_source: S3InputSource,
) -> anyhow::Result<CompressionTaskOutput> {
    let clp_home = clp_home();
    let list_path = config.abs_tmp_directory(clp_home).join(format!(
        "compression-{}-{}-{}-log-paths.txt",
        ctx.job_id, ctx.task_id, ctx.task_instance_id,
    ));

    tracing::info!(
        job_id = % ctx.job_id,
        task_id = % ctx.task_id,
        task_instance_id = % ctx.task_instance_id,
        list_path = % list_path.display(),
        "CLP S3 compression task started.",
    );

    let mut tmp_file_deleter = TmpFileDeleter::new();

    std::fs::write(&list_path, build_s3_logs_list(&input_source))
        .with_context(|| format!("failed to write S3 logs list to {}", list_path.display()))
        .inspect_err(|e| {
            tracing::error!(
                error = % e,
                list_path = % list_path.display(),
                "Failed to write S3 logs list to tmp directory."
            );
        })?;
    tmp_file_deleter.add(list_path.clone());

    let S3InputSource {
        aws_authentication, ..
    } = input_source;
    let credential_env = s3_credential_env(&aws_authentication);

    let s3_config = extract_s3_output_config(config)?;
    let archive_dir = config
        .abs_archive_output_staging(clp_home)
        .join(resolve_dataset_name(dataset.as_deref()));

    let clp_s_bin = clp_binary_path(clp_home, "clp-s");
    let indexer_bin = clp_binary_path(clp_home, "indexer");

    let (clp_s_input, clp_s_credential_env) = prepare_clp_s_input(
        ctx,
        clp_home,
        config,
        clp_s_option,
        &list_path,
        credential_env,
        &mut tmp_file_deleter,
    )?;

    let runtime = runtime();
    let client = build_s3_client(&runtime, s3_config);
    let bucket = s3_config.bucket.to_string();

    let mut archives = Vec::new();
    let mut finishers = tokio::task::JoinSet::new();

    let archive_dir_clone = archive_dir.clone();
    let archive_callback = |archive: ArchiveMetadata| {
        let archive_staging_path = archive_dir_clone.join(&archive.id);
        tmp_file_deleter.add(archive_staging_path.clone());
        finishers.spawn_on(
            ArchiveFinisher {
                client: client.clone(),
                bucket: bucket.clone(),
                key: create_archive_s3_key(&config.archive_output, dataset.as_deref(), &archive.id),
                indexer_bin: indexer_bin.clone(),
                database: config.database.clone(),
                dataset: dataset.clone(),
                local_path: archive_staging_path,
                archive_id: archive.id.clone(),
            }
            .finish(),
            &runtime,
        );
        archives.push(archive);
        Ok(())
    };

    let clp_s_result = run_clp_s(
        &clp_s_bin,
        &archive_dir,
        clp_s_option,
        &clp_s_input,
        &clp_s_credential_env,
        archive_callback,
    );

    let finish_error = drain_finishers(&runtime, finishers);

    clp_s_result?;
    if let Some(e) = finish_error {
        return Err(e);
    }

    tracing::info!(
        job_id = % ctx.job_id,
        task_id = % ctx.task_id,
        task_instance_id = % ctx.task_instance_id,
        "CLP S3 compression task completed successfully.",
    );
    Ok(CompressionTaskOutput { dataset, archives })
}

/// Buffers tmp paths and deletes them when dropped, warning (rather than failing) on any deletion
/// error.
///
/// # Note
///
/// * Deletion is sequential and blocking.
/// * A path that cannot be deleted is logged as a warning and skipped rather than propagated, since
///   a destructor cannot fail.
struct TmpFileDeleter {
    paths: Vec<PathBuf>,
}

impl TmpFileDeleter {
    /// Factory function.
    ///
    /// # Returns
    ///
    /// An empty [`TmpFileDeleter`].
    const fn new() -> Self {
        Self { paths: Vec::new() }
    }

    /// Registers `path` for deletion when this deleter is dropped.
    fn add(&mut self, path: PathBuf) {
        self.paths.push(path);
    }
}

impl Drop for TmpFileDeleter {
    fn drop(&mut self) {
        for path in &self.paths {
            let result = if path.is_dir() {
                std::fs::remove_dir_all(path)
            } else {
                std::fs::remove_file(path)
            };
            if let Err(e) = result {
                tracing::warn!(
                    error = % e,
                    path = % path.display(),
                    "Failed to delete tmp path."
                );
            }
        }
    }
}

/// How clp-s reads its input.
enum ClpSInput {
    /// A `--files-from` list file of S3 object URLs; clp-s reads the objects directly from S3.
    FilesFrom(PathBuf),

    /// A local directory of converted logs files passed positionally. clp-s reads the local files.
    Directory(PathBuf),
}

/// An async finisher for publishing a single archive.
struct ArchiveFinisher {
    client: aws_sdk_s3::Client,
    bucket: String,
    key: String,
    indexer_bin: PathBuf,
    database: Database,
    dataset: Option<String>,
    local_path: PathBuf,
    archive_id: String,
}

impl ArchiveFinisher {
    /// Publishes one produced archive: indexes it and uploads it to S3 concurrently.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * The indexer task panics.
    /// * Forwards [`upload_file_to_s3`]'s return values on failure.
    /// * Forwards [`run_indexer`]'s return values on failure.
    async fn finish(self) -> anyhow::Result<()> {
        let Self {
            client,
            bucket,
            key,
            indexer_bin,
            database,
            dataset,
            local_path,
            archive_id,
        } = self;

        let index_path = local_path.clone();
        let index = tokio::task::spawn_blocking(move || {
            run_indexer(&indexer_bin, &database, dataset.as_deref(), &index_path)
        });
        let (upload_result, index_result) = tokio::join!(
            upload_file_to_s3(&client, &bucket, &key, &local_path),
            index
        );
        upload_result
            .context("failed to upload archive to S3")
            .inspect_err(|e| {
                tracing::error!(
                    error = % e,
                    archive_id = % archive_id,
                    bucket = % bucket,
                    key = % key,
                    "Failed to upload archive to S3."
                );
            })?;
        index_result
            .map_err(|e| anyhow::anyhow!("indexer task panicked: {e}"))
            .inspect_err(|e| {
                tracing::error!(
                    error = % e,
                    "indexer panicked."
                );
            })??;
        anyhow::Ok(())
    }
}

/// Builds an S3 client for the archive-output bucket, resolving the region to
/// [`AWS_DEFAULT_REGION`] when the config omits it.
///
/// # Returns
///
/// The S3 client the archives are uploaded through.
fn build_s3_client(runtime: &tokio::runtime::Handle, s3_config: &S3Config) -> aws_sdk_s3::Client {
    let region = s3_config
        .region_code
        .as_ref()
        .map_or(AWS_DEFAULT_REGION, NonEmptyString::as_str);
    runtime.block_on(create_new_client(
        region,
        s3_config.endpoint_url.as_ref(),
        &s3_config.aws_authentication,
    ))
}

/// Builds the `--files-from` list of S3 object URLs for clp-s.
///
/// # Returns
///
/// The newline-terminated list of object URLs, one per object key in `input_source`.
fn build_s3_logs_list(input_source: &S3InputSource) -> String {
    let endpoint = input_source
        .endpoint_url
        .as_ref()
        .map(NonEmptyString::as_str);
    let region = input_source
        .region_code
        .as_ref()
        .map(NonEmptyString::as_str);
    let bucket = input_source.bucket.as_str();

    let mut list = String::new();
    for object_key in &input_source.object_keys {
        list.push_str(&generate_s3_url(endpoint, region, bucket, object_key));
        list.push('\n');
    }
    list
}

/// Resolves the AWS credential env vars clp-s needs to access the S3 objects.
///
/// # Returns
///
/// * The env-var name-value pairs for [`AwsAuthentication::Credentials`].
/// * An empty vector for [`AwsAuthentication::Default`] (which assumes credentials are already in
///   the ambient env).
fn s3_credential_env(auth: &AwsAuthentication) -> Vec<(&'static str, String)> {
    /// The env var holding the AWS access key ID.
    const AWS_ACCESS_KEY_ID_ENV_VAR: &str = "AWS_ACCESS_KEY_ID";

    /// The env var holding the AWS secret access key.
    const AWS_SECRET_ACCESS_KEY_ENV_VAR: &str = "AWS_SECRET_ACCESS_KEY";

    match auth {
        AwsAuthentication::Credentials { credentials } => vec![
            (AWS_ACCESS_KEY_ID_ENV_VAR, credentials.access_key_id.clone()),
            (
                AWS_SECRET_ACCESS_KEY_ENV_VAR,
                credentials.secret_access_key.clone(),
            ),
        ],
        AwsAuthentication::Default => Vec::new(),
    }
}

/// Parses a single clp-s `--print-archive-stats` stdout line into an [`ArchiveMetadata`].
///
/// NOTE: clp-s emits a superset of [`ArchiveMetadata`]'s fields per line; unknown fields are
/// ignored.
///
/// # Returns
///
/// The parsed [`ArchiveMetadata`].
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`serde_json::from_str`]'s return values on failure.
fn parse_archive_stats(line: &str) -> anyhow::Result<ArchiveMetadata> {
    serde_json::from_str(line)
        .with_context(|| format!("failed to parse clp-s archive stats: {line}"))
}

/// Waits for every spawned archive finisher to complete, returning the first error encountered. A
/// panicked finisher is surfaced as an error.
///
/// # Returns
///
/// The first finisher error, or `None` if all finishers succeeded.
fn drain_finishers(
    runtime: &tokio::runtime::Handle,
    mut finishers: tokio::task::JoinSet<anyhow::Result<()>>,
) -> Option<anyhow::Error> {
    runtime.block_on(async {
        let mut first_error: Option<anyhow::Error> = None;
        while let Some(joined) = finishers.join_next().await {
            let result = joined
                .map_err(|e| anyhow::anyhow!("archive callback panicked: {e}"))
                .inspect_err(|e| {
                    tracing::error!(
                        error = % e,
                        "Archive callback panicked."
                    );
                })
                .and_then(|r| r);
            if let Err(e) = result
                && first_error.is_none()
            {
                first_error = Some(e);
            }
        }
        first_error
    })
}

/// Prepares clp-s's input. For unstructured logs, runs log-converter over `list_path` into a fresh
/// tmp directory (registered with `tmp_file_deleter`) and returns that directory as the input; for
/// structured logs, returns the S3 object list directly. clp-s needs the S3 credentials only in the
/// structured case, so the unstructured case returns an empty credential env.
///
/// # Returns
///
/// The clp-s input and the credential env clp-s should run with.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`std::fs::create_dir`]'s return values on failure.
/// * Forwards [`run_log_converter`]'s return values on failure.
fn prepare_clp_s_input(
    ctx: &spider_tdl::TaskContext,
    clp_home: &Path,
    config: &SpiderTaskExecutorConfig,
    clp_s_option: &ClpSCompressionOption,
    list_path: &Path,
    credential_env: Vec<(&'static str, String)>,
    tmp_file_deleter: &mut TmpFileDeleter,
) -> anyhow::Result<(ClpSInput, Vec<(&'static str, String)>)> {
    if !clp_s_option.unstructured {
        return Ok((
            ClpSInput::FilesFrom(list_path.to_path_buf()),
            credential_env,
        ));
    }

    let converted_dir = config.abs_tmp_directory(clp_home).join(format!(
        "compression-{}-{}-{}-converted",
        ctx.job_id, ctx.task_id, ctx.task_instance_id,
    ));
    std::fs::create_dir(&converted_dir)
        .with_context(|| {
            format!(
                "failed to create the log-converter output directory {}",
                converted_dir.display()
            )
        })
        .inspect_err(|e| {
            tracing::error!(
                error = % e,
                converted_dir = % converted_dir.display(),
                "Failed to create the log-converter output directory."
            );
        })?;
    tmp_file_deleter.add(converted_dir.clone());
    let log_converter_bin = clp_binary_path(clp_home, "log-converter");
    run_log_converter(
        &log_converter_bin,
        &converted_dir,
        list_path,
        &credential_env,
    )?;
    Ok((ClpSInput::Directory(converted_dir), Vec::new()))
}

/// Builds the clp-s command-line arguments for a single-file-archive compression run.
///
/// The unstructured path drops S3 auth (clp-s reads the local converted directory) and pins the
/// timestamp key to `timestamp`.
///
/// # Returns
///
/// The ordered clp-s arguments.
fn build_clp_s_args(
    archive_output_dir: &Path,
    input: &ClpSInput,
    clp_s_option: &ClpSCompressionOption,
) -> Vec<OsString> {
    let mut args = vec![
        OsString::from("c"),
        archive_output_dir.as_os_str().to_os_string(),
        OsString::from("--print-archive-stats"),
        OsString::from("--target-encoded-size"),
        OsString::from(clp_s_option.target_encoded_size.to_string()),
        OsString::from("--compression-level"),
        OsString::from(clp_s_option.compression_level.to_string()),
        OsString::from("--single-file-archive"),
    ];
    match input {
        ClpSInput::FilesFrom(path) => {
            // Compressing from S3.
            args.push(OsString::from("--auth"));
            args.push(OsString::from("s3"));
            if let Some(timestamp_key) = &clp_s_option.timestamp_key {
                args.push(OsString::from("--timestamp-key"));
                args.push(OsString::from(timestamp_key));
            }
            args.push(OsString::from("--files-from"));
            args.push(path.as_os_str().to_os_string());
        }
        ClpSInput::Directory(path) => {
            // Compressing from local converted files.
            args.push(OsString::from("--timestamp-key"));
            args.push(OsString::from("timestamp"));
            args.push(path.as_os_str().to_os_string());
        }
    }
    args
}

/// Builds the log-converter command-line arguments for converting S3-sourced unstructured logs.
///
/// # Returns
///
/// The ordered log-converter arguments.
fn build_log_converter_args(output_dir: &Path, inputs_from_path: &Path) -> Vec<OsString> {
    vec![
        OsString::from("--output-dir"),
        output_dir.as_os_str().to_os_string(),
        OsString::from("--inputs-from"),
        inputs_from_path.as_os_str().to_os_string(),
        OsString::from("--auth"),
        OsString::from("s3"),
    ]
}

/// Resolves the path of a CLP binary under `clp_home`, joining `bin/{binary}`.
///
/// # Returns
///
/// The path to the named binary under the CLP installation.
fn clp_binary_path(clp_home: &Path, binary: &str) -> PathBuf {
    clp_home.join("bin").join(binary)
}

/// Resolves the S3 config the archives are uploaded to from `config`.
///
/// # Returns
///
/// The S3 config the archives are uploaded to.
///
/// # Errors
///
/// Returns an error if:
///
/// * `config`'s archive output is not S3-backed.
fn extract_s3_output_config(config: &SpiderTaskExecutorConfig) -> anyhow::Result<&S3Config> {
    match &config.archive_output.storage {
        ArchiveOutputStorage::S3 { s3_config, .. } => Ok(s3_config),
        ArchiveOutputStorage::Fs { .. } => {
            anyhow::bail!("S3 archive output is required for the S3 compression flow")
        }
    }
}

/// Builds the S3 object key for an archive by appending `archive_id` to
/// [`ArchiveOutput::dataset_archive_storage_directory`].
///
/// # Returns
///
/// The archive's S3 object key.
fn create_archive_s3_key(
    archive_output: &ArchiveOutput,
    dataset: Option<&str>,
    archive_id: &str,
) -> String {
    format!(
        "{}/{archive_id}",
        archive_output.dataset_archive_storage_directory(dataset)
    )
}

/// Uploads a local file to S3 through `PutObject`.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`aws_sdk_s3::primitives::ByteStream::from_path`]'s return values on failure.
/// * Forwards [`aws_sdk_s3::operation::put_object::builders::PutObjectFluentBuilder::send`]'s
///   return values on failure.
async fn upload_file_to_s3(
    client: &aws_sdk_s3::Client,
    bucket: &str,
    key: &str,
    src: &Path,
) -> anyhow::Result<()> {
    let body = aws_sdk_s3::primitives::ByteStream::from_path(src)
        .await
        .with_context(|| format!("failed to read {} for upload", src.display()))?;
    client
        .put_object()
        .bucket(bucket)
        .key(key)
        .body(body)
        .send()
        .await
        .with_context(|| format!("failed to upload to s3://{bucket}/{key}"))?;
    Ok(())
}

/// Builds the `indexer` command-line arguments for indexing one archive.
///
/// # Returns
///
/// The ordered `indexer` arguments.
fn build_indexer_args(
    database: &Database,
    dataset: Option<&str>,
    archive_path: &Path,
) -> Vec<OsString> {
    vec![
        OsString::from("--db-type"),
        // NOTE: clp-core's indexer only supports MySQL, regardless of the configured DB engine.
        OsString::from("mysql"),
        OsString::from("--db-host"),
        OsString::from(&database.host),
        OsString::from("--db-port"),
        OsString::from(database.port.to_string()),
        OsString::from("--db-name"),
        OsString::from(&database.names.clp),
        OsString::from("--db-table-prefix"),
        OsString::from(&database.table_prefix),
        OsString::from(resolve_dataset_name(dataset)),
        archive_path.as_os_str().to_os_string(),
    ]
}

/// Runs the `indexer` on one archive, blocking until it exits.
///
/// # Returns
///
/// `()` once the indexer exits successfully.
///
/// # Errors
///
/// Returns an error if:
///
/// * The indexer exits with a non-zero status.
/// * Forwards [`Command::status`]'s return values on failure.
fn run_indexer(
    indexer_bin: &Path,
    database: &Database,
    dataset: Option<&str>,
    archive_path: &Path,
) -> anyhow::Result<()> {
    let status = Command::new(indexer_bin)
        .args(build_indexer_args(database, dataset, archive_path))
        .status()
        .with_context(|| format!("failed to spawn indexer at {}", indexer_bin.display()))?;
    if !status.success() {
        tracing::error!(
            status = % status,
            archive_path = % archive_path.display(),
            "indexer exited on failure."
        );
        anyhow::bail!(
            "indexer exited with {status} for archive {}",
            archive_path.display()
        );
    }
    Ok(())
}

/// Runs log-converter to convert the unstructured text logs listed in `inputs_from_path` into
/// kv-ir under `output_dir` for clp-s ingestion.
///
/// stdout is discarded; stderr is captured so it can be surfaced if the converter fails.
///
/// # Errors
///
/// Returns an error if:
///
/// * log-converter exits with a non-zero status.
/// * Forwards [`Command::spawn`]'s return values on failure.
/// * Forwards [`std::process::Child::wait`]'s return values on failure.
///
/// # Panics
///
/// Panics if log-converter's piped stderr is unexpectedly absent (should be unreachable).
fn run_log_converter(
    log_converter_bin: &Path,
    output_dir: &Path,
    inputs_from_path: &Path,
    credential_env: &[(&'static str, String)],
) -> anyhow::Result<()> {
    let mut child = Command::new(log_converter_bin)
        .args(build_log_converter_args(output_dir, inputs_from_path))
        .envs(credential_env.iter().cloned())
        .stdout(Stdio::null())
        .stderr(Stdio::piped())
        .spawn()
        .with_context(|| {
            format!(
                "failed to spawn log-converter at {}",
                log_converter_bin.display()
            )
        })?;

    let mut stderr = child
        .stderr
        .take()
        .expect("piped stderr should always be present");

    let mut captured_stderr = String::new();
    if let Err(e) = stderr.read_to_string(&mut captured_stderr) {
        captured_stderr = format!("failed to read log-converter stderr: {e}");
    }

    let status = child
        .wait()
        .context("failed to wait for log-converter to exit")
        .inspect_err(|e| {
            tracing::error!(
                error = % e,
                stderr = % captured_stderr,
                "Failed to wait log-converter."
            );
        })?;
    if !status.success() {
        tracing::error!(
            status = % status,
            stderr = % captured_stderr,
            "log-converter exited on failure."
        );
        anyhow::bail!("log-converter exited with {status}");
    }
    Ok(())
}

/// Runs clp-s, invoking `on_archive` for each archive it reports on stdout.
///
/// Spawns clp-s with the resolved arguments and credential env vars, draining stderr on a dedicated
/// thread to avoid a pipe deadlock while stdout is streamed line by line. Each stdout line is
/// parsed into an [`ArchiveMetadata`] and forwarded to `on_archive`. If `on_archive` fails, clp-s
/// is killed and reaped before the error is returned.
///
/// # Errors
///
/// Returns an error if:
///
/// * clp-s exits with a non-zero status.
/// * Forwards `on_archive`'s return values on failure.
/// * Forwards [`Command::spawn`]'s return values on failure.
/// * Forwards [`parse_archive_stats`]'s return values on failure.
/// * Forwards [`std::process::Child::wait`]'s return values on failure.
///
/// # Panics
///
/// Panics if clp-s's piped stdout or stderr is unexpectedly absent (should be unreachable).
fn run_clp_s(
    clp_s_bin: &Path,
    archive_dir: &Path,
    clp_s_option: &ClpSCompressionOption,
    input: &ClpSInput,
    credential_env: &[(&'static str, String)],
    mut on_archive: impl FnMut(ArchiveMetadata) -> anyhow::Result<()>,
) -> anyhow::Result<()> {
    let mut child = Command::new(clp_s_bin)
        .args(build_clp_s_args(archive_dir, input, clp_s_option))
        .envs(credential_env.iter().cloned())
        .stdout(Stdio::piped())
        .stderr(Stdio::piped())
        .spawn()
        .with_context(|| format!("failed to spawn clp-s at {}", clp_s_bin.display()))?;

    let mut stderr = child
        .stderr
        .take()
        .expect("piped stderr should always be present");
    let stderr_reader = std::thread::spawn(move || {
        let mut buffer = String::new();
        stderr.read_to_string(&mut buffer).map(|_| buffer)
    });

    let stdout = child
        .stdout
        .take()
        .expect("piped stdout should always be present");
    for line in BufReader::new(stdout).lines() {
        let line = line.context("failed to read a line from clp-s stdout")?;
        if let Err(e) = parse_archive_stats(&line).and_then(&mut on_archive) {
            let _ = child.kill();
            let _ = child.wait();
            let stderr = match stderr_reader.join() {
                Ok(Ok(stderr)) => stderr,
                _ => "Failed to read clp-s stderr.".to_string(),
            };
            tracing::error!(
                error = % e,
                archive_stats = % line,
                stderr = % stderr,
                "Failed to execute per-archive callback. Killing clp-s."
            );
            return Err(e.context("clp-s killed on error"));
        }
    }

    let status = child.wait().context("failed to wait for clp-s to exit")?;
    let stderr = match stderr_reader.join() {
        Ok(Ok(stderr)) => stderr,
        _ => "Failed to read clp-s stderr.".to_string(),
    };
    if !status.success() {
        tracing::error!(
            status = % status,
            stderr = % stderr,
            "clp-s exited on failure."
        );
        anyhow::bail!("clp-s exited with {status}");
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use std::{
        ffi::OsString,
        path::{Path, PathBuf},
    };

    use clp_rust_utils::{
        clp_config::{
            AwsAuthentication,
            AwsCredentials,
            S3Config,
            package::config::{ArchiveOutput, ArchiveOutputStorage, ClpDbNames, Database},
        },
        task_io::compression::{ArchiveMetadata, ClpSCompressionOption, S3InputSource},
    };
    use non_empty_string::NonEmptyString;

    use super::{
        ClpSInput,
        build_clp_s_args,
        build_indexer_args,
        build_log_converter_args,
        build_s3_logs_list,
        create_archive_s3_key,
        parse_archive_stats,
        s3_credential_env,
    };

    #[test]
    fn build_s3_logs_list_default_endpoint() {
        let input_source = S3InputSource {
            endpoint_url: None,
            region_code: Some(
                NonEmptyString::try_from("us-east-1".to_string())
                    .expect("region code is non-empty"),
            ),
            bucket: NonEmptyString::try_from("logs".to_string()).expect("bucket is non-empty"),
            aws_authentication: AwsAuthentication::Default,
            object_keys: vec!["a/b.json".to_string(), "c/d.json".to_string()],
        };

        assert_eq!(
            build_s3_logs_list(&input_source),
            "https://logs.s3.us-east-1.amazonaws.com/a/b.json\n\
             https://logs.s3.us-east-1.amazonaws.com/c/d.json\n"
        );
    }

    #[test]
    fn s3_credential_env_default() {
        assert_eq!(s3_credential_env(&AwsAuthentication::Default), Vec::new());
    }

    #[test]
    fn s3_credential_env_credentials() {
        let auth = AwsAuthentication::Credentials {
            credentials: AwsCredentials {
                access_key_id: "the-access-key".to_string(),
                secret_access_key: "the-secret-key".to_string(),
            },
        };

        assert_eq!(
            s3_credential_env(&auth),
            vec![
                ("AWS_ACCESS_KEY_ID", "the-access-key".to_string()),
                ("AWS_SECRET_ACCESS_KEY", "the-secret-key".to_string()),
            ]
        );
    }

    #[test]
    fn parse_archive_stats_ignores_extra_keys() {
        let line = concat!(
            r#"{"id":"abc","begin_timestamp":10,"end_timestamp":20,"#,
            r#""uncompressed_size":100,"size":40,"is_split":false,"range_index":{}}"#,
        );

        assert_eq!(
            parse_archive_stats(line).expect("valid archive stats line"),
            ArchiveMetadata {
                id: "abc".to_string(),
                begin_timestamp: 10,
                end_timestamp: 20,
                size: 40,
                uncompressed_size: 100,
            }
        );
    }

    #[test]
    fn parse_archive_stats_rejects_malformed_line() {
        assert!(parse_archive_stats("not json").is_err());
    }

    #[test]
    fn build_clp_s_args_structured_with_timestamp_key() {
        let clp_s_option = ClpSCompressionOption {
            target_encoded_size: 268_435_456,
            compression_level: 3,
            timestamp_key: Some("ts".to_string()),
            unstructured: false,
        };

        assert_eq!(
            build_clp_s_args(
                Path::new("/archives"),
                &ClpSInput::FilesFrom(PathBuf::from("/tmp/log-paths.txt")),
                &clp_s_option
            ),
            vec![
                OsString::from("c"),
                OsString::from("/archives"),
                OsString::from("--print-archive-stats"),
                OsString::from("--target-encoded-size"),
                OsString::from("268435456"),
                OsString::from("--compression-level"),
                OsString::from("3"),
                OsString::from("--single-file-archive"),
                OsString::from("--auth"),
                OsString::from("s3"),
                OsString::from("--timestamp-key"),
                OsString::from("ts"),
                OsString::from("--files-from"),
                OsString::from("/tmp/log-paths.txt"),
            ]
        );
    }

    #[test]
    fn build_clp_s_args_structured_without_timestamp_key() {
        let clp_s_option = ClpSCompressionOption {
            target_encoded_size: 268_435_456,
            compression_level: 3,
            timestamp_key: None,
            unstructured: false,
        };

        assert_eq!(
            build_clp_s_args(
                Path::new("/archives"),
                &ClpSInput::FilesFrom(PathBuf::from("/tmp/log-paths.txt")),
                &clp_s_option
            ),
            vec![
                OsString::from("c"),
                OsString::from("/archives"),
                OsString::from("--print-archive-stats"),
                OsString::from("--target-encoded-size"),
                OsString::from("268435456"),
                OsString::from("--compression-level"),
                OsString::from("3"),
                OsString::from("--single-file-archive"),
                OsString::from("--auth"),
                OsString::from("s3"),
                OsString::from("--files-from"),
                OsString::from("/tmp/log-paths.txt"),
            ]
        );
    }

    #[test]
    fn build_clp_s_args_unstructured_uses_directory_and_no_s3_auth() {
        let clp_s_option = ClpSCompressionOption {
            target_encoded_size: 268_435_456,
            compression_level: 3,
            timestamp_key: Some("ignored".to_string()),
            unstructured: true,
        };

        assert_eq!(
            build_clp_s_args(
                Path::new("/archives"),
                &ClpSInput::Directory(PathBuf::from("/tmp/converted")),
                &clp_s_option
            ),
            vec![
                OsString::from("c"),
                OsString::from("/archives"),
                OsString::from("--print-archive-stats"),
                OsString::from("--target-encoded-size"),
                OsString::from("268435456"),
                OsString::from("--compression-level"),
                OsString::from("3"),
                OsString::from("--single-file-archive"),
                OsString::from("--timestamp-key"),
                OsString::from("timestamp"),
                OsString::from("/tmp/converted"),
            ]
        );
    }

    #[test]
    fn build_log_converter_args_expected_order() {
        assert_eq!(
            build_log_converter_args(Path::new("/tmp/converted"), Path::new("/tmp/log-paths.txt")),
            vec![
                OsString::from("--output-dir"),
                OsString::from("/tmp/converted"),
                OsString::from("--inputs-from"),
                OsString::from("/tmp/log-paths.txt"),
                OsString::from("--auth"),
                OsString::from("s3"),
            ]
        );
    }

    #[test]
    fn archive_s3_key_joins_prefix_dataset_and_id() {
        let archive_output = ArchiveOutput {
            storage: ArchiveOutputStorage::S3 {
                staging_directory: "var/data/staged-archives".to_owned(),
                s3_config: S3Config {
                    bucket: NonEmptyString::try_from("bucket".to_string())
                        .expect("bucket is non-empty"),
                    region_code: None,
                    key_prefix: NonEmptyString::try_from("LIB1/".to_string())
                        .expect("key prefix is non-empty"),
                    endpoint_url: None,
                    aws_authentication: AwsAuthentication::Default,
                },
            },
            ..ArchiveOutput::default()
        };

        assert_eq!(
            create_archive_s3_key(&archive_output, None, "abc"),
            "LIB1/default/abc"
        );
    }

    #[test]
    fn build_indexer_args_uses_mysql_and_expected_order() {
        let database = Database {
            host: "db-host".to_string(),
            port: 3306,
            names: ClpDbNames {
                clp: "clp-db".to_string(),
                spider: "spider-db".to_string(),
            },
            table_prefix: "clp_".to_string(),
        };

        assert_eq!(
            build_indexer_args(&database, None, Path::new("/archives/abc")),
            vec![
                OsString::from("--db-type"),
                OsString::from("mysql"),
                OsString::from("--db-host"),
                OsString::from("db-host"),
                OsString::from("--db-port"),
                OsString::from("3306"),
                OsString::from("--db-name"),
                OsString::from("clp-db"),
                OsString::from("--db-table-prefix"),
                OsString::from("clp_"),
                OsString::from("default"),
                OsString::from("/archives/abc"),
            ]
        );
    }
}

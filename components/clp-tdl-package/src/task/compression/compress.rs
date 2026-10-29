//! The `clp-s` compression worker that turns filesystem or S3 inputs into archives.

use std::ffi::OsString;
use std::io::BufRead;
use std::io::BufReader;
use std::io::Read;
use std::marker::PhantomData;
use std::path::Path;
use std::path::PathBuf;
use std::process::Command;
use std::process::Stdio;

use anyhow::Context;
use clp_rust_utils::aws::AWS_DEFAULT_REGION;
use clp_rust_utils::clp_config::S3Config;
use clp_rust_utils::clp_config::package::config::ArchiveOutputStorage;
use clp_rust_utils::clp_config::package::config::Database;
use clp_rust_utils::clp_config::package::config::LOGS_INPUT_MOUNT_POINT;
use clp_rust_utils::clp_config::package::config::SpiderTaskExecutorConfig;
use clp_rust_utils::clp_config::package::config::StorageEngine;
use clp_rust_utils::dataset::DatasetName;
use clp_rust_utils::logs_input::LogsInputMountPoint;
use clp_rust_utils::s3::create_new_client;
use clp_rust_utils::s3::generate_s3_url;
use clp_rust_utils::task_io::compression::ArchiveMetadata;
use clp_rust_utils::task_io::compression::ClpSCompressionOption;
use clp_rust_utils::task_io::compression::CompressionTaskOutput;
use clp_rust_utils::task_io::compression::FsInputSource;
use clp_rust_utils::task_io::compression::S3InputSource;
use clp_rust_utils::types::ArchiveId;
use clp_rust_utils::types::non_empty_string::ExpectedNonEmpty;
use non_empty_string::NonEmptyString;

use crate::common::clp_home;
use crate::common::runtime;
use crate::task::utils::clp_binary_path;
use crate::task::utils::s3_credential_env;

/// Source-specific validation and preparation for compression.
pub(super) trait Source: Sized {
    /// Whether the original input list requires S3 authentication.
    const USE_S3_AUTH: bool;

    /// Validates the source and prepares its explicit input list and credentials.
    ///
    /// # Returns
    ///
    /// The prepared source, or `None` for a validated file-free filesystem manifest, on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Source entries are invalid or cannot be read.
    /// * S3 credentials cannot be resolved.
    ///
    /// # Panics
    ///
    /// Panics for S3 inputs if the package runtime has not been initialized.
    fn prepare(self) -> anyhow::Result<Option<PreparedSource<Self>>>;
}

/// Validated input-list contents and their read credentials.
pub(super) struct PreparedSource<SourceType: Source> {
    logs_list: String,
    credential_env: Vec<(&'static str, String)>,
    source_type: PhantomData<SourceType>,
}

impl Source for S3InputSource {
    const USE_S3_AUTH: bool = true;

    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`build_s3_logs_list`]'s return values on failure.
    /// * Forwards [`s3_credential_env`]'s return values on failure.
    fn prepare(self) -> anyhow::Result<Option<PreparedSource<Self>>> {
        let logs_list = build_s3_logs_list(&self)?;
        let input_region = self
            .region_code
            .as_ref()
            .map_or(AWS_DEFAULT_REGION, NonEmptyString::as_str);
        let credential_env = s3_credential_env(&runtime(), input_region, &self.aws_authentication)?;
        Ok(Some(PreparedSource {
            logs_list,
            credential_env,
            source_type: PhantomData,
        }))
    }
}

impl Source for FsInputSource {
    const USE_S3_AUTH: bool = false;

    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`LogsInputMountPoint::resolve`]'s return values on failure.
    /// * Forwards [`build_fs_logs_list`]'s return values on failure.
    fn prepare(self) -> anyhow::Result<Option<PreparedSource<Self>>> {
        let mount_point =
            LogsInputMountPoint::resolve(NonEmptyString::from_static_str(LOGS_INPUT_MOUNT_POINT))?;
        let logs_list = build_fs_logs_list(&self, &mount_point)?;
        if logs_list.is_empty() {
            return Ok(None);
        }
        Ok(Some(PreparedSource {
            logs_list,
            credential_env: Vec::new(),
            source_type: PhantomData,
        }))
    }
}

/// Compresses the given inputs into archives, uploads them to S3, and returns their metadata
/// for the commit task.
///
/// # Type Parameters
///
/// * `SourceType` - The input manifest's validation and preparation implementation.
///
/// # Returns
///
/// The metadata of every archive this task produced on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`anyhow::Error`] if the configured storage engine is not CLP-S.
/// * Forwards [`Source::prepare`]'s return values on failure.
/// * Forwards [`InputList::new`]'s return values on failure.
/// * Forwards [`extract_s3_output_config`]'s return values on failure.
/// * Forwards [`PreparedInput::new`]'s return values on failure.
/// * Forwards [`run_clp_s`]'s return values on failure.
/// * Forwards [`drain_finishers`]'s collected error, if any.
///
/// # Panics
///
/// Panics if required package globals (the runtime or CLP home) have not been initialized or this
/// function is called from an async runtime context.
pub(super) fn compress<SourceType: Source>(
    ctx: &spider_tdl::TaskContext,
    config: &SpiderTaskExecutorConfig,
    clp_s_option: &ClpSCompressionOption,
    dataset: DatasetName,
    input_source: SourceType,
) -> anyhow::Result<CompressionTaskOutput> {
    if StorageEngine::ClpS != config.package.storage_engine {
        anyhow::bail!("compression requires the clp-s storage engine");
    }
    let Some(prepared_source) = input_source.prepare()? else {
        return Ok(CompressionTaskOutput {
            dataset,
            archives: Vec::new(),
        });
    };
    let clp_home = clp_home();
    let list_path = config.abs_tmp_directory(clp_home).join(format!(
        "compression-{}-{}-{}-log-paths.txt",
        ctx.job_id, ctx.task_id, ctx.task_instance_id,
    ));
    let mut tmp_file_deleter = TmpFileDeleter::new();
    let input_list = InputList::new(list_path, prepared_source, &mut tmp_file_deleter)?;
    let runtime = runtime();

    let s3_config = extract_s3_output_config(config)?;
    let archive_dir = config
        .abs_archive_output_staging(clp_home)
        .join(dataset.as_str());

    let clp_s_bin = clp_binary_path(clp_home, "clp-s");
    let indexer_bin = clp_binary_path(clp_home, "indexer");

    let prepared_input = PreparedInput::new(
        ctx,
        clp_home,
        config,
        clp_s_option,
        input_list,
        &mut tmp_file_deleter,
    )?;

    let client = build_s3_client(&runtime, s3_config);
    let bucket = s3_config.bucket.to_string();

    let mut archives = Vec::new();
    let mut finishers = tokio::task::JoinSet::new();

    let archive_callback = |archive: ArchiveMetadata| {
        let archive_staging_path = archive_dir.join(archive.id.to_string());
        tmp_file_deleter.add(archive_staging_path.clone());
        finishers.spawn_on(
            ArchiveFinisher {
                client: client.clone(),
                bucket: bucket.clone(),
                key: config
                    .archive_output
                    .dataset_archive_object_key(&dataset, &archive.id)
                    .into_inner(),
                indexer_bin: indexer_bin.clone(),
                database: config.database.clone(),
                dataset: dataset.clone(),
                local_path: archive_staging_path,
                archive_id: archive.id,
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
        &prepared_input,
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
        "CLP compression task completed successfully.",
    );
    Ok(CompressionTaskOutput { dataset, archives })
}

/// How clp-s reads its input.
enum ClpSInput {
    /// An explicit `--files-from` list of local paths or S3 object URLs.
    FilesFrom(PathBuf),

    /// A local directory of converted logs.
    Directory(PathBuf),
}

struct InputList<SourceType: Source> {
    path: PathBuf,
    credential_env: Vec<(&'static str, String)>,
    source_type: PhantomData<SourceType>,
}

impl<SourceType: Source> InputList<SourceType> {
    /// Creates a temporary input list from a prepared source.
    ///
    /// # Type Parameters
    ///
    /// * `SourceType` - The source associated with the prepared contents.
    ///
    /// # Returns
    ///
    /// The input-list path and credentials on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`std::fs::write`]'s return values on failure.
    fn new(
        path: PathBuf,
        prepared_source: PreparedSource<SourceType>,
        tmp_file_deleter: &mut TmpFileDeleter,
    ) -> anyhow::Result<Self> {
        tmp_file_deleter.add(path.clone());
        std::fs::write(&path, prepared_source.logs_list)
            .with_context(|| format!("failed to write logs list to {}", path.display()))?;
        Ok(Self {
            path,
            credential_env: prepared_source.credential_env,
            source_type: prepared_source.source_type,
        })
    }
}

struct PreparedInput<SourceType: Source> {
    input: ClpSInput,
    credential_env: Vec<(&'static str, String)>,
    source_type: PhantomData<SourceType>,
}

impl<SourceType: Source> PreparedInput<SourceType> {
    /// Prepares structured input for compression, converting unstructured sources as needed.
    ///
    /// # Type Parameters
    ///
    /// * `SourceType` - Determines authentication for the original input list.
    ///
    /// # Returns
    ///
    /// The clp-s input and the credential env clp-s should run with, on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`std::fs::create_dir`]'s return values on failure.
    /// * Forwards [`run_log_converter`]'s return values on failure.
    fn new(
        ctx: &spider_tdl::TaskContext,
        clp_home: &Path,
        config: &SpiderTaskExecutorConfig,
        clp_s_option: &ClpSCompressionOption,
        input_list: InputList<SourceType>,
        tmp_file_deleter: &mut TmpFileDeleter,
    ) -> anyhow::Result<Self> {
        if !clp_s_option.unstructured {
            return Ok(Self {
                input: ClpSInput::FilesFrom(input_list.path),
                credential_env: input_list.credential_env,
                source_type: input_list.source_type,
            });
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
        run_log_converter(&log_converter_bin, &converted_dir, &input_list)?;
        Ok(Self {
            input: ClpSInput::Directory(converted_dir),
            credential_env: Vec::new(),
            source_type: input_list.source_type,
        })
    }
}

/// Attempts to remove registered temporary paths on drop, logging cleanup failures.
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

/// An async finisher for publishing a single archive.
struct ArchiveFinisher {
    client: aws_sdk_s3::Client,
    bucket: String,
    key: String,
    indexer_bin: PathBuf,
    database: Database,
    dataset: DatasetName,
    local_path: PathBuf,
    archive_id: ArchiveId,
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
            run_indexer(&indexer_bin, &database, &dataset, &index_path)
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

/// Builds the `--files-from` list of S3 object URLs for clp-s.
///
/// # Returns
///
/// The newline-terminated list of object URLs, one per object key in `input_source`, on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * An object key in `input_source` is empty.
/// * Forwards [`generate_s3_url`]'s return values on failure.
fn build_s3_logs_list(input_source: &S3InputSource) -> anyhow::Result<String> {
    let endpoint = input_source
        .endpoint_url
        .as_ref()
        .map(NonEmptyString::as_str);
    let region = input_source
        .region_code
        .as_ref()
        .map(NonEmptyString::as_str);

    let mut list = String::new();
    for object_key in &input_source.object_keys {
        let object_key = NonEmptyString::try_from(object_key.clone())
            .map_err(|_| anyhow::anyhow!("s3 object key must not be empty"))?;
        list.push_str(&generate_s3_url(
            endpoint,
            region,
            &input_source.bucket,
            &object_key,
        )?);
        list.push('\n');
    }
    Ok(list)
}

/// Revalidates filesystem entries against the logs-input mount point.
///
/// # Returns
///
/// A newline-terminated list of checked regular-file paths on success. Empty directories are
/// validated but omitted from the list.
///
/// # Errors
///
/// Returns an error if:
///
/// * A file entry is not a regular file or an empty-directory entry contains entries.
/// * Forwards [`LogsInputMountPoint::to_container_path`]'s return values on failure.
/// * Forwards [`std::fs::read_dir`]'s return values on failure.
/// * Forwards [`std::fs::ReadDir::next`]'s entry errors on failure.
fn build_fs_logs_list(
    source: &FsInputSource,
    mount_point: &LogsInputMountPoint,
) -> anyhow::Result<String> {
    let mut list = String::new();
    for file in &source.file_paths {
        let path = mount_point.to_container_path(file)?;
        if !Path::new(path.as_str()).is_file() {
            anyhow::bail!("input {} is not a regular file", path.as_str());
        }
        list.push_str(path.as_str());
        list.push('\n');
    }
    for directory in &source.empty_directories {
        let path = mount_point.to_container_path(directory)?;
        let mut entries = std::fs::read_dir(path.as_str())
            .with_context(|| format!("input {} is not a readable directory", path.as_str()))?;
        if entries.next().transpose()?.is_some() {
            anyhow::bail!("input directory {} is no longer empty", path.as_str());
        }
    }
    Ok(list)
}

/// Resolves the S3 config the archives are uploaded to from `config`.
///
/// # Returns
///
/// The archive-output S3 configuration on success.
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
            anyhow::bail!("s3 archive output is required for compression")
        }
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

/// Waits for all archive finishers, collecting the first execution or join error.
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
                .map_err(|e| anyhow::anyhow!("failed to join archive finisher: {e}"))
                .inspect_err(|e| {
                    tracing::error!(
                        error = % e,
                        "Failed to join archive finisher."
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

/// Builds the clp-s command-line arguments for a single-file-archive compression run.
///
/// Converted inputs use local files and the `timestamp` key.
///
/// # Type Parameters
///
/// * `SourceType` - Determines authentication for the original input list.
///
/// # Returns
///
/// The ordered clp-s arguments.
fn build_clp_s_args<SourceType: Source>(
    archive_output_dir: &Path,
    prepared_input: &PreparedInput<SourceType>,
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
    match &prepared_input.input {
        ClpSInput::FilesFrom(path) => {
            if SourceType::USE_S3_AUTH {
                args.push(OsString::from("--auth"));
                args.push(OsString::from("s3"));
            }
            if let Some(timestamp_key) = &clp_s_option.timestamp_key {
                args.push(OsString::from("--timestamp-key"));
                args.push(OsString::from(timestamp_key));
            }
            args.push(OsString::from("--files-from"));
            args.push(path.as_os_str().to_os_string());
        }
        ClpSInput::Directory(path) => {
            args.push(OsString::from("--timestamp-key"));
            args.push(OsString::from("timestamp"));
            args.push(path.as_os_str().to_os_string());
        }
    }
    args
}

/// Runs clp-s, invoking `on_archive` for each archive it reports on stdout.
///
/// Attempts to stop and reap the process after a stdout or callback failure. Cleanup failures
/// are logged without replacing the original error.
///
/// # Type Parameters
///
/// * `SourceType` - Determines authentication for the original input list.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`anyhow::Error`] if clp-s exits with a non-zero status.
/// * Forwards [`std::io::Lines::next`]'s line-reading errors on failure.
/// * Forwards `on_archive`'s return values on failure.
/// * Forwards [`Command::spawn`]'s return values on failure.
/// * Forwards [`parse_archive_stats`]'s return values on failure.
/// * Forwards [`std::process::Child::wait`]'s return values on failure.
///
/// # Panics
///
/// Panics if clp-s's piped stdout or stderr is unexpectedly absent (should be unreachable), or the
/// stderr-reader thread cannot be spawned.
fn run_clp_s<SourceType: Source>(
    clp_s_bin: &Path,
    archive_dir: &Path,
    clp_s_option: &ClpSCompressionOption,
    prepared_input: &PreparedInput<SourceType>,
    mut on_archive: impl FnMut(ArchiveMetadata) -> anyhow::Result<()>,
) -> anyhow::Result<()> {
    let mut child = Command::new(clp_s_bin)
        .args(build_clp_s_args(archive_dir, prepared_input, clp_s_option))
        .envs(
            prepared_input
                .credential_env
                .iter()
                .map(|(key, value)| (*key, value)),
        )
        .stdout(Stdio::piped())
        .stderr(Stdio::piped())
        .spawn()
        .with_context(|| format!("failed to spawn clp-s at {}", clp_s_bin.display()))?;

    let mut stderr = child
        .stderr
        .take()
        .expect("piped stderr should always be present");
    // Drain stderr concurrently so a full pipe cannot block stdout processing.
    let stderr_reader = std::thread::spawn(move || {
        let mut buffer = String::new();
        stderr.read_to_string(&mut buffer).map(|_| buffer)
    });

    let stdout = child
        .stdout
        .take()
        .expect("piped stdout should always be present");
    for line in BufReader::new(stdout).lines() {
        let line = match line.context("failed to read a line from clp-s stdout") {
            Ok(line) => line,
            Err(e) => {
                let stderr = kill_clp_s_and_read_stderr(&mut child, stderr_reader);
                tracing::error!(
                    error = % e,
                    stderr = % stderr,
                    "Failed to read a line from clp-s stdout."
                );
                return Err(e.context("clp-s output processing failed"));
            }
        };
        if let Err(e) = parse_archive_stats(&line).and_then(&mut on_archive) {
            let stderr = kill_clp_s_and_read_stderr(&mut child, stderr_reader);
            tracing::error!(
                error = % e,
                archive_stats = % line,
                stderr = % stderr,
                "Failed to execute per-archive callback."
            );
            return Err(e.context("clp-s output processing failed"));
        }
    }

    let status = child.wait().context("failed to wait for clp-s to exit")?;
    let stderr = match stderr_reader.join() {
        Ok(Ok(stderr)) => stderr,
        Ok(Err(error)) => {
            tracing::warn!(error = % error, "Failed to read clp-s stderr.");
            format!("failed to read clp-s stderr: {error}")
        }
        Err(_) => {
            tracing::warn!("Clp-s stderr reader panicked.");
            "clp-s stderr reader panicked".to_owned()
        }
    };
    if !status.success() {
        tracing::error!(
            status = status.code(),
            stderr = % stderr,
            "Clp-s exited on failure."
        );
        anyhow::bail!("clp-s exited with {status}");
    }
    Ok(())
}

/// Attempts to stop and reap clp-s, logging failures, then collects captured stderr.
///
/// # Returns
///
/// The captured stderr, or a diagnostic if reading it fails or its reader panics.
fn kill_clp_s_and_read_stderr(
    child: &mut std::process::Child,
    stderr_reader: std::thread::JoinHandle<std::io::Result<String>>,
) -> String {
    if let Err(error) = child.kill() {
        tracing::warn!(error = % error, "Failed to kill clp-s during cleanup.");
    }
    if let Err(error) = child.wait() {
        tracing::warn!(error = % error, "Failed to reap clp-s during cleanup.");
    }
    match stderr_reader.join() {
        Ok(Ok(stderr)) => stderr,
        Ok(Err(error)) => {
            tracing::warn!(error = % error, "Failed to read clp-s stderr.");
            format!("failed to read clp-s stderr: {error}")
        }
        Err(_) => {
            tracing::warn!("Clp-s stderr reader panicked.");
            "clp-s stderr reader panicked".to_owned()
        }
    }
}

/// Parses a single clp-s `--print-archive-stats` stdout line into an [`ArchiveMetadata`].
///
/// NOTE: clp-s emits a superset of [`ArchiveMetadata`]'s fields per line; unknown fields are
/// ignored.
///
/// # Returns
///
/// The parsed [`ArchiveMetadata`] on success.
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

/// Builds the log-converter command-line arguments for converting unstructured logs.
///
/// # Type Parameters
///
/// * `SourceType` - Determines authentication for the original input list.
///
/// # Returns
///
/// The ordered log-converter arguments.
fn build_log_converter_args<SourceType: Source>(
    output_dir: &Path,
    input_list: &InputList<SourceType>,
) -> Vec<OsString> {
    let mut args = vec![
        OsString::from("--output-dir"),
        output_dir.as_os_str().to_os_string(),
        OsString::from("--inputs-from"),
        input_list.path.as_os_str().to_os_string(),
    ];
    if SourceType::USE_S3_AUTH {
        args.push(OsString::from("--auth"));
        args.push(OsString::from("s3"));
    }
    args
}

/// Converts the listed unstructured logs into local kv-ir files.
///
/// # Type Parameters
///
/// * `SourceType` - Determines authentication for the original input list.
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
fn run_log_converter<SourceType: Source>(
    log_converter_bin: &Path,
    output_dir: &Path,
    input_list: &InputList<SourceType>,
) -> anyhow::Result<()> {
    let mut child = Command::new(log_converter_bin)
        .args(build_log_converter_args(output_dir, input_list))
        .envs(
            input_list
                .credential_env
                .iter()
                .map(|(key, value)| (*key, value)),
        )
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
    if let Err(error) = stderr.read_to_string(&mut captured_stderr) {
        tracing::warn!(error = % error, "Failed to read log-converter stderr.");
        captured_stderr = format!("failed to read log-converter stderr: {error}");
    }

    let status = child
        .wait()
        .context("failed to wait for log-converter to exit")
        .inspect_err(|e| {
            tracing::error!(
                error = % e,
                stderr = % captured_stderr,
                "Failed to wait for log-converter."
            );
        })?;
    if !status.success() {
        tracing::error!(
            status = status.code(),
            stderr = % captured_stderr,
            "Log-converter exited on failure."
        );
        anyhow::bail!("log-converter exited with {status}");
    }
    Ok(())
}

/// Builds the `indexer` command-line arguments for indexing one archive.
///
/// # Returns
///
/// The ordered `indexer` arguments.
fn build_indexer_args(
    database: &Database,
    dataset: &DatasetName,
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
        OsString::from(dataset.as_str()),
        archive_path.as_os_str().to_os_string(),
    ]
}

/// Runs the `indexer` on one archive, blocking until it exits.
///
/// # Returns
///
/// `()` on success.
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
    dataset: &DatasetName,
    archive_path: &Path,
) -> anyhow::Result<()> {
    let status = Command::new(indexer_bin)
        .args(build_indexer_args(database, dataset, archive_path))
        .status()
        .with_context(|| format!("failed to spawn indexer at {}", indexer_bin.display()))?;
    if !status.success() {
        tracing::error!(
            status = status.code(),
            archive_path = % archive_path.display(),
            "Indexer exited on failure."
        );
        anyhow::bail!(
            "indexer exited with {status} for archive {}",
            archive_path.display()
        );
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    //! Tests for input preparation, compression arguments, and subprocess errors.

    use std::ffi::OsString;
    use std::marker::PhantomData;
    use std::path::Path;
    use std::path::PathBuf;

    use clp_rust_utils::clp_config::AwsAuthentication;
    use clp_rust_utils::clp_config::package::config::FsIngestion;
    use clp_rust_utils::clp_config::package::config::LogsInput;
    use clp_rust_utils::clp_config::package::config::Package;
    use clp_rust_utils::clp_config::package::config::SpiderTaskExecutorConfig;
    use clp_rust_utils::clp_config::package::config::StorageEngine;
    use clp_rust_utils::dataset::DatasetName;
    use clp_rust_utils::logs_input::LogsInputMountPoint;
    use clp_rust_utils::logs_input::LogsInputPath;
    use clp_rust_utils::task_io::compression::ClpSCompressionOption;
    use clp_rust_utils::task_io::compression::FsInputSource;
    use clp_rust_utils::task_io::compression::S3InputSource;
    use clp_rust_utils::types::non_empty_string::ExpectedNonEmpty;
    use non_empty_string::NonEmptyString;
    use spider_core::types::id::JobId;
    use spider_core::types::id::ResourceGroupId;
    use spider_core::types::id::TaskId;
    use spider_tdl::TaskContext;

    use super::ClpSInput;
    use super::InputList;
    use super::PreparedInput;
    use super::PreparedSource;
    use super::Source;
    use super::TmpFileDeleter;
    use super::build_clp_s_args;
    use super::build_fs_logs_list;
    use super::build_indexer_args;
    use super::build_log_converter_args;
    use super::build_s3_logs_list;
    use super::compress;
    use super::parse_archive_stats;

    const BASE_OPTION: ClpSCompressionOption = ClpSCompressionOption {
        target_encoded_size: 268_435_456,
        compression_level: 3,
        timestamp_key: None,
        unstructured: false,
    };

    struct FsFixture {
        root: PathBuf,
        _cleanup: TmpFileDeleter,
        mount_point: LogsInputMountPoint,
    }

    impl FsFixture {
        /// Creates a temporary input root that is removed when the fixture is dropped.
        ///
        /// # Returns
        ///
        /// The temporary fixture.
        ///
        /// # Panics
        ///
        /// Panics if the temporary root cannot be created, represented as UTF-8, or resolved.
        fn new() -> Self {
            let root = std::env::temp_dir().join(format!("clp-worker-input-{}", JobId::random()));
            std::fs::create_dir(&root).expect("temporary input root should be created");
            let mut cleanup = TmpFileDeleter::new();
            cleanup.add(root.clone());
            let mount_point = LogsInputMountPoint::resolve(
                NonEmptyString::new(root.to_str().expect("test root is UTF-8").to_owned())
                    .expect("test root is nonempty"),
            )
            .expect("test mount should resolve");
            Self {
                mount_point,
                root: root
                    .canonicalize()
                    .expect("temporary input root should resolve"),
                _cleanup: cleanup,
            }
        }

        /// # Returns
        ///
        /// A CLP-S worker configuration with a host-side input path unavailable to the worker.
        fn config() -> SpiderTaskExecutorConfig {
            SpiderTaskExecutorConfig {
                package: Package {
                    storage_engine: StorageEngine::ClpS,
                },
                logs_input: LogsInput::Fs {
                    config: FsIngestion {
                        directory: "/host-only/input/path".to_owned(),
                    },
                },
                ..SpiderTaskExecutorConfig::default()
            }
        }
    }

    /// # Returns
    ///
    /// A task context without graph outputs.
    ///
    /// # Panics
    ///
    /// Panics if [`TaskContext::new`] rejects the context.
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
    fn s3_preparation_preserves_urls_credentials_and_auth() {
        use clp_rust_utils::clp_config::AwsCredentials;

        crate::common::init_runtime().expect("test runtime should initialize");
        let fixture = FsFixture::new();
        let config = FsFixture::config();
        let source = S3InputSource {
            endpoint_url: Some(NonEmptyString::from_static_str("http://localhost:4566")),
            region_code: None,
            bucket: NonEmptyString::from_static_str("logs"),
            aws_authentication: AwsAuthentication::Credentials {
                credentials: AwsCredentials {
                    access_key_id: "test-access-key".to_owned(),
                    secret_access_key: "test-secret-key".to_owned(),
                    session_token: None,
                },
            },
            object_keys: vec!["input.json".to_owned()],
        };
        let prepared_source = source
            .prepare()
            .expect("valid source should be prepared")
            .expect("s3 preparation produces an explicit list");
        let list_path = fixture.root.join("s3-list.txt");
        let mut cleanup = TmpFileDeleter::new();
        let input_list = InputList::new(list_path.clone(), prepared_source, &mut cleanup)
            .expect("input list should be written");
        assert_eq!(
            std::fs::read_to_string(&list_path).expect("input list should be readable"),
            "http://localhost:4566/logs/input.json\n"
        );
        let prepared_input = PreparedInput::new(
            &task_context(),
            &fixture.root,
            &config,
            &BASE_OPTION,
            input_list,
            &mut cleanup,
        )
        .expect("structured compressor input should be prepared");
        assert_eq!(
            prepared_input.credential_env,
            vec![
                ("AWS_ACCESS_KEY_ID", "test-access-key".to_owned()),
                ("AWS_SECRET_ACCESS_KEY", "test-secret-key".to_owned()),
            ]
        );
        let args = build_clp_s_args::<S3InputSource>(
            Path::new("/archives"),
            &prepared_input,
            &BASE_OPTION,
        );
        assert!(
            args.windows(2)
                .any(|pair| pair == [OsString::from("--auth"), OsString::from("s3")])
        );
    }

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
            build_s3_logs_list(&input_source).expect("valid object keys should produce an S3 list"),
            "https://logs.s3.us-east-1.amazonaws.com/a/b.json\n\
             https://logs.s3.us-east-1.amazonaws.com/c/d.json\n"
        );
    }

    #[test]
    fn build_clp_s_args_structured_with_timestamp_key() {
        let clp_s_option = ClpSCompressionOption {
            timestamp_key: Some("ts".to_string()),
            ..BASE_OPTION
        };

        assert_eq!(
            build_clp_s_args::<S3InputSource>(
                Path::new("/archives"),
                &PreparedInput {
                    input: ClpSInput::FilesFrom(PathBuf::from("/tmp/log-paths.txt")),
                    credential_env: vec![],
                    source_type: PhantomData,
                },
                &clp_s_option
            ),
            vec![
                OsString::from("c"),
                OsString::from("/archives"),
                OsString::from("--print-archive-stats"),
                OsString::from("--target-encoded-size"),
                OsString::from(BASE_OPTION.target_encoded_size.to_string()),
                OsString::from("--compression-level"),
                OsString::from(BASE_OPTION.compression_level.to_string()),
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
        let clp_s_option = BASE_OPTION;

        assert_eq!(
            build_clp_s_args::<S3InputSource>(
                Path::new("/archives"),
                &PreparedInput {
                    input: ClpSInput::FilesFrom(PathBuf::from("/tmp/log-paths.txt")),
                    credential_env: vec![],
                    source_type: PhantomData,
                },
                &clp_s_option
            ),
            vec![
                OsString::from("c"),
                OsString::from("/archives"),
                OsString::from("--print-archive-stats"),
                OsString::from("--target-encoded-size"),
                OsString::from(BASE_OPTION.target_encoded_size.to_string()),
                OsString::from("--compression-level"),
                OsString::from(BASE_OPTION.compression_level.to_string()),
                OsString::from("--single-file-archive"),
                OsString::from("--auth"),
                OsString::from("s3"),
                OsString::from("--files-from"),
                OsString::from("/tmp/log-paths.txt"),
            ]
        );
    }

    #[test]
    fn build_log_converter_args_expected_order() {
        assert_eq!(
            build_log_converter_args::<S3InputSource>(
                Path::new("/tmp/converted"),
                &InputList {
                    path: PathBuf::from("/tmp/log-paths.txt"),
                    credential_env: vec![],
                    source_type: PhantomData
                }
            ),
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
    fn fs_preparation_preserves_zero_byte_files_and_omits_auth() {
        let fixture = FsFixture::new();
        let file = fixture.root.join("empty.json");
        std::fs::write(&file, []).expect("test input file should be created");
        let empty_directory = fixture.root.join("empty-directory");
        std::fs::create_dir(&empty_directory).expect("empty test directory should be created");
        let source = FsInputSource {
            file_paths: vec![
                LogsInputPath::try_from("/empty.json".to_owned())
                    .expect("test input path should be valid"),
            ],
            empty_directories: vec![
                LogsInputPath::try_from("/empty-directory".to_owned())
                    .expect("test directory path should be valid"),
            ],
        };
        let config = FsFixture::config();
        let logs_list = build_fs_logs_list(&source, &fixture.mount_point)
            .expect("valid source should be prepared");
        assert!(
            !logs_list.is_empty(),
            "a zero-byte file still requires compression"
        );
        let prepared_source = PreparedSource::<FsInputSource> {
            logs_list,
            credential_env: vec![],
            source_type: PhantomData,
        };
        let list_path = fixture.root.join("input-list.txt");
        let mut cleanup = TmpFileDeleter::new();
        let input_list = InputList::new(list_path.clone(), prepared_source, &mut cleanup)
            .expect("input list should be written");
        assert_eq!(
            std::fs::read_to_string(&list_path).expect("input list should be readable"),
            format!("{}\n", file.display())
        );
        let prepared_input = PreparedInput::new(
            &task_context(),
            &fixture.root,
            &config,
            &BASE_OPTION,
            input_list,
            &mut cleanup,
        )
        .expect("structured compressor input should be prepared");
        assert_eq!(prepared_input.credential_env, vec![]);
        let args = build_clp_s_args::<FsInputSource>(
            Path::new("/archives"),
            &prepared_input,
            &BASE_OPTION,
        );
        assert!(!args.contains(&OsString::from("--auth")));
        assert!(args.ends_with(&[
            OsString::from("--files-from"),
            list_path.as_os_str().to_owned()
        ]));
        drop(cleanup);
        assert!(!list_path.exists());
        assert!(file.exists());
    }

    #[test]
    fn fs_list_rejects_directory_in_place_of_file() {
        let fixture = FsFixture::new();
        let source = FsInputSource {
            file_paths: vec![
                LogsInputPath::try_from("/".to_owned()).expect("mount root path should be valid"),
            ],
            empty_directories: vec![],
        };
        let error = build_fs_logs_list(&source, &fixture.mount_point)
            .expect_err("a directory must not be recursively compressed");
        assert!(error.to_string().contains("not a regular file"));
    }

    #[test]
    fn fs_list_revalidates_empty_directories() {
        let fixture = FsFixture::new();
        let source = FsInputSource {
            file_paths: vec![],
            empty_directories: vec![
                LogsInputPath::try_from("/".to_owned()).expect("mount root path should be valid"),
            ],
        };
        assert_eq!(
            build_fs_logs_list(&source, &fixture.mount_point)
                .expect("validated empty directories should succeed"),
            ""
        );
        std::fs::write(fixture.root.join("new.json"), [])
            .expect("new input file should be created");
        let error = build_fs_logs_list(&source, &fixture.mount_point)
            .expect_err("a directory populated after planning must be rejected");
        assert!(error.to_string().contains("no longer empty"));
    }

    #[test]
    fn fs_list_rejects_file_in_place_of_directory() {
        let fixture = FsFixture::new();
        std::fs::write(fixture.root.join("input.json"), [])
            .expect("test input file should be created");
        let source = FsInputSource {
            file_paths: vec![],
            empty_directories: vec![
                LogsInputPath::try_from("/input.json".to_owned())
                    .expect("test input path should be valid"),
            ],
        };
        let error = build_fs_logs_list(&source, &fixture.mount_point)
            .expect_err("a file must not be accepted as an empty directory");
        assert!(error.to_string().contains("not a readable directory"));
    }

    #[cfg(unix)]
    #[test]
    fn fs_list_rejects_special_files() {
        let fixture = FsFixture::new();
        let socket = fixture.root.join("socket");
        let _listener =
            std::os::unix::net::UnixListener::bind(&socket).expect("test socket should be created");
        let source = FsInputSource {
            file_paths: vec![
                LogsInputPath::try_from("/socket".to_owned())
                    .expect("test socket path should be valid"),
            ],
            empty_directories: vec![],
        };
        let error = build_fs_logs_list(&source, &fixture.mount_point)
            .expect_err("a socket must not be accepted as a regular file");
        assert!(error.to_string().contains("not a regular file"));
    }

    #[test]
    fn fs_arguments_use_explicit_lists_without_s3_auth() {
        let input_list = PathBuf::from("/tmp/local-files.txt");
        let args = build_clp_s_args::<FsInputSource>(
            Path::new("/archives"),
            &PreparedInput {
                input: ClpSInput::FilesFrom(input_list.clone()),
                credential_env: vec![],
                source_type: PhantomData,
            },
            &ClpSCompressionOption {
                timestamp_key: Some("ts".to_owned()),
                ..BASE_OPTION
            },
        );
        assert_eq!(
            args,
            vec![
                OsString::from("c"),
                OsString::from("/archives"),
                OsString::from("--print-archive-stats"),
                OsString::from("--target-encoded-size"),
                OsString::from(BASE_OPTION.target_encoded_size.to_string()),
                OsString::from("--compression-level"),
                OsString::from(BASE_OPTION.compression_level.to_string()),
                OsString::from("--single-file-archive"),
                OsString::from("--timestamp-key"),
                OsString::from("ts"),
                OsString::from("--files-from"),
                OsString::from("/tmp/local-files.txt")
            ]
        );
        let input_list = InputList::<FsInputSource> {
            path: input_list,
            credential_env: vec![],
            source_type: PhantomData,
        };
        assert_eq!(
            build_log_converter_args(Path::new("/tmp/converted"), &input_list),
            vec![
                "--output-dir",
                "/tmp/converted",
                "--inputs-from",
                "/tmp/local-files.txt"
            ]
            .into_iter()
            .map(OsString::from)
            .collect::<Vec<_>>()
        );
    }

    #[cfg(unix)]
    #[test]
    fn fs_converter_failure_is_returned() {
        let error = super::run_log_converter::<FsInputSource>(
            Path::new("/bin/false"),
            Path::new("/unused/output"),
            &InputList {
                path: PathBuf::from("/unused/inputs"),
                credential_env: vec![],
                source_type: PhantomData,
            },
        )
        .expect_err("a failed converter must fail the compression task");
        assert!(error.to_string().contains("log-converter exited with"));
    }

    #[test]
    fn compression_rejects_non_clp_s_engine_even_for_file_free_input() {
        let error = compress(
            &task_context(),
            &SpiderTaskExecutorConfig::default(),
            &BASE_OPTION,
            DatasetName::default(),
            FsInputSource {
                file_paths: vec![],
                empty_directories: vec![],
            },
        )
        .expect_err("unsupported engine must be rejected before input processing");
        assert!(error.to_string().contains("clp-s"));
    }

    #[test]
    fn build_clp_s_args_unstructured_uses_directory_and_no_s3_auth() {
        let clp_s_option = ClpSCompressionOption {
            timestamp_key: Some("ignored".to_string()),
            unstructured: true,
            ..BASE_OPTION
        };

        let expected = vec![
            OsString::from("c"),
            OsString::from("/archives"),
            OsString::from("--print-archive-stats"),
            OsString::from("--target-encoded-size"),
            OsString::from(BASE_OPTION.target_encoded_size.to_string()),
            OsString::from("--compression-level"),
            OsString::from(BASE_OPTION.compression_level.to_string()),
            OsString::from("--single-file-archive"),
            OsString::from("--timestamp-key"),
            OsString::from("timestamp"),
            OsString::from("/tmp/converted"),
        ];
        let s3_input = PreparedInput::<S3InputSource> {
            input: ClpSInput::Directory(PathBuf::from("/tmp/converted")),
            credential_env: vec![],
            source_type: PhantomData,
        };
        let fs_input = PreparedInput::<FsInputSource> {
            input: ClpSInput::Directory(PathBuf::from("/tmp/converted")),
            credential_env: vec![],
            source_type: PhantomData,
        };
        for args in [
            build_clp_s_args(Path::new("/archives"), &s3_input, &clp_s_option),
            build_clp_s_args(Path::new("/archives"), &fs_input, &clp_s_option),
        ] {
            assert_eq!(args, expected);
        }
    }

    #[test]
    fn parse_archive_stats_reports_invalid_line() {
        let error = parse_archive_stats("not json").expect_err("malformed stats must fail");
        assert_eq!(
            error.to_string(),
            "failed to parse clp-s archive stats: not json"
        );
    }

    #[test]
    fn build_indexer_args_uses_mysql_and_expected_order() {
        use clp_rust_utils::clp_config::package::config::ClpDbNames;
        use clp_rust_utils::clp_config::package::config::Database;

        let database = Database {
            host: "db-host".to_string(),
            port: 3306,
            names: ClpDbNames {
                clp: "clp-db".to_string(),
            },
            table_prefix: "clp_".to_string(),
        };

        assert_eq!(
            build_indexer_args(
                &database,
                &DatasetName::default(),
                Path::new("/archives/abc")
            ),
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

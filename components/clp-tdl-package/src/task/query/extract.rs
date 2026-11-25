//! Extraction and publication of ordered JSON chunks from a single archive.

use std::fs;
use std::io;
use std::io::BufRead;
use std::io::BufReader;
use std::io::Read;
use std::num::NonZeroU64;
use std::path::Path;
use std::path::PathBuf;
use std::process::Child;
use std::process::Command;
use std::process::ExitStatus;
use std::process::Stdio;
use std::thread;
use std::thread::ScopedJoinHandle;

use anyhow::Context;
use clp_rust_utils::clp_config::package::config::ResultsCache;
use clp_rust_utils::clp_config::package::config::SpiderTaskExecutorConfig;
use clp_rust_utils::clp_config::package::config::StorageEngine;
use clp_rust_utils::clp_config::package::config::StreamOutputStorage;
use clp_rust_utils::task_io::extraction::ExtractTaskInput;
use clp_rust_utils::types::ArchiveId;
use mongodb::Client;
use mongodb::Collection;
use mongodb::Namespace;
use mongodb::bson::Document;
use mongodb::bson::doc;
use mongodb::options::ReplaceOneModel;
use serde::Deserialize;
use spider_core::types::id::TaskInstanceId;

use crate::task::query::archive_input::ArchiveDescriptor;
use crate::task::query::results_cache::parse_results_cache_uri;
use crate::task::utils::build_s3_client;
use crate::task::utils::clp_binary_path;
use crate::task::utils::upload_file_to_s3;

/// Extracts and publishes an archive's ordered JSON chunks and their metadata.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`fs::create_dir_all`]'s return values on failure.
/// * Forwards [`tempfile::Builder::tempdir_in`]'s return values on failure.
/// * Forwards [`run_clp_s_extract`]'s return values on failure.
/// * Forwards [`reject_unordered_output`]'s return values on failure.
/// * Forwards [`ChunkBatch::parse`]'s return values on failure.
/// * Forwards [`prepare_metadata_collection`]'s return values on failure.
/// * Forwards [`publish_files`]'s return values on failure.
/// * Forwards [`upsert_metadata`]'s return values on failure.
pub(super) fn extract(
    runtime: &tokio::runtime::Handle,
    clp_home: &Path,
    config: &SpiderTaskExecutorConfig,
    task_instance_id: TaskInstanceId,
    input: &ExtractTaskInput,
) -> anyhow::Result<()> {
    let staging_root = config.abs_stream_output_staging(clp_home).join(".staging");
    fs::create_dir_all(&staging_root).with_context(|| {
        format!(
            "failed to create the extraction staging root {}",
            staging_root.display()
        )
    })?;
    let staging_directory = tempfile::Builder::new()
        .prefix(&format!("{}.{task_instance_id}", input.job_id))
        .rand_bytes(0)
        .tempdir_in(&staging_root)
        .context("failed to create an extraction staging directory")?;
    let result = (|| {
        let staging_path = staging_directory.path();
        let batch = {
            let paths = run_clp_s_extract(runtime, clp_home, config, input, staging_path)?;
            reject_unordered_output(staging_path)?;
            ChunkBatch::parse(staging_path, input.archive_id, &paths)?
        };
        if batch.chunks.is_empty() {
            return Ok(());
        }
        let collection = runtime.block_on(prepare_metadata_collection(&config.results_cache))?;
        publish_files(runtime, clp_home, config, staging_path, &batch.chunks)?;
        runtime.block_on(upsert_metadata(&collection, &batch))
    })();
    if let Err(error) = staging_directory.close() {
        tracing::warn!(
            error = % error,
            query_job_id = input.job_id,
            task_instance_id,
            "Failed to remove the extraction staging directory."
        );
    }
    result
}

const MAX_STATS_LINE_SIZE: usize = 64 * 1024;
const MAX_STATS_OUTPUT_SIZE: usize = 128 * 1024 * 1024;
const MAX_STATS_READ_SIZE: usize = MAX_STATS_LINE_SIZE + 1;
const MAX_CHUNKS: usize = 1_000_000;
const MAX_STDERR_TAIL_SIZE: usize = 64 * 1024;

const _: () = assert!(
    MAX_STATS_OUTPUT_SIZE
        .checked_add(MAX_STATS_READ_SIZE)
        .is_some(),
    "the maximum output size plus one stats line and its newline must fit in a usize"
);

/// An extracted file and its validated bounds in the archive's event stream.
#[derive(Debug, PartialEq)]
struct Chunk {
    filename: String,
    begin_msg_ix: i64,
    end_msg_ix: i64,
    is_last: bool,
}

/// An archive and its validated chunks.
#[derive(Debug)]
struct ChunkBatch {
    archive_id: ArchiveId,
    chunks: Vec<Chunk>,
}

impl ChunkBatch {
    /// Validates the filenames and contiguous ranges of a sequence of ordered chunks.
    ///
    /// # Returns
    ///
    /// The archive ID and its validated chunks in emission order on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`anyhow::Error`] if:
    ///   * A path is not the exact path of a file directly under `staging_path`.
    ///   * A filename does not match the archive ID and chunk naming format.
    ///   * The ranges do not form a contiguous sequence of positive-length ranges from index zero.
    /// * Forwards [`parse_chunk_index`]'s return values on failure.
    fn parse(
        staging_path: &Path,
        archive_id: ArchiveId,
        paths: &[PathBuf],
    ) -> anyhow::Result<Self> {
        let prefix = format!("{archive_id}_");
        let mut chunks = Vec::with_capacity(paths.len());
        let mut expected_begin = 0;
        for (index, path) in paths.iter().enumerate() {
            let filename = path
                .file_name()
                .and_then(|name| name.to_str())
                .filter(|name| path.as_os_str() == staging_path.join(name).as_os_str())
                .with_context(|| {
                    format!(
                        "reported chunk path {} is not a UTF-8 filename directly under {}",
                        path.display(),
                        staging_path.display()
                    )
                })?;
            let (begin, end) = filename
                .strip_prefix(&prefix)
                .and_then(|suffix| suffix.strip_suffix(".jsonl"))
                .and_then(|range| range.split_once('_'))
                .with_context(|| format!("invalid ordered chunk filename {filename}"))?;
            let begin_msg_ix = parse_chunk_index(begin, filename)?;
            let end_msg_ix = parse_chunk_index(end, filename)?;
            anyhow::ensure!(
                begin_msg_ix == expected_begin && end_msg_ix > begin_msg_ix,
                "invalid range in {filename}: expected start {expected_begin} and a larger end"
            );
            chunks.push(Chunk {
                filename: filename.to_owned(),
                begin_msg_ix,
                end_msg_ix,
                is_last: index + 1 == paths.len(),
            });
            expected_begin = end_msg_ix;
        }
        Ok(Self { archive_id, chunks })
    }

    /// # Returns
    ///
    /// The results-cache upserts for the archive's chunks in emission order.
    fn replacement_models(
        &self,
        namespace: Namespace,
    ) -> impl Iterator<Item = ReplaceOneModel> + '_ {
        let archive_id = self.archive_id.to_string();
        self.chunks.iter().map(move |chunk| {
            ReplaceOneModel::builder()
                .namespace(namespace.clone())
                .filter(doc! { "_id": &chunk.filename })
                .replacement(doc! {
                    "_id": &chunk.filename,
                    "path": &chunk.filename,
                    "stream_id": &archive_id,
                    "begin_msg_ix": chunk.begin_msg_ix,
                    "end_msg_ix": chunk.end_msg_ix,
                    "is_last_chunk": chunk.is_last,
                })
                .upsert(true)
                .build()
        })
    }
}

/// Rejects the `original` file clp-s writes when it falls back to unordered extraction.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`anyhow::Error`] if the unordered output exists.
/// * Forwards [`fs::symlink_metadata`]'s return values on failure, except when the file is absent.
fn reject_unordered_output(staging_path: &Path) -> anyhow::Result<()> {
    let path = staging_path.join("original");
    match fs::symlink_metadata(&path) {
        Ok(_) => anyhow::bail!("clp-s produced unordered output; the archive lacks log ordering"),
        Err(error) if io::ErrorKind::NotFound == error.kind() => Ok(()),
        Err(error) => Err(error).with_context(|| format!("failed to check for {}", path.display())),
    }
}

/// # Returns
///
/// The parsed nonnegative event index on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`anyhow::Error`] if the index is not written in canonical decimal form.
/// * Forwards [`str::parse`]'s return values on failure.
fn parse_chunk_index(index: &str, filename: &str) -> anyhow::Result<i64> {
    anyhow::ensure!(
        !index.is_empty()
            && index.bytes().all(|byte| byte.is_ascii_digit())
            && (index.len() == 1 || !index.starts_with('0')),
        "invalid event index {index:?} in ordered chunk filename {filename}"
    );
    index.parse().with_context(|| {
        format!("invalid event index {index:?} in ordered chunk filename {filename}")
    })
}

/// Publishes staged chunks to the filesystem or S3.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`anyhow::Error`] if a staged chunk is not a regular file.
/// * Forwards [`fs::symlink_metadata`]'s return values on failure.
/// * Forwards [`fs::rename`]'s return values on failure.
/// * Forwards [`upload_file_to_s3`]'s return values on failure.
fn publish_files(
    runtime: &tokio::runtime::Handle,
    clp_home: &Path,
    config: &SpiderTaskExecutorConfig,
    staging_path: &Path,
    chunks: &[Chunk],
) -> anyhow::Result<()> {
    for chunk in chunks {
        let path = staging_path.join(&chunk.filename);
        let metadata = fs::symlink_metadata(&path).with_context(|| {
            format!(
                "failed to read metadata for staged chunk {}",
                path.display()
            )
        })?;
        anyhow::ensure!(
            metadata.is_file(),
            "staged chunk {} is not a regular file",
            path.display()
        );
    }
    match &config.stream_output.storage {
        StreamOutputStorage::Fs { .. } => {
            let local_directory = config.abs_stream_output_staging(clp_home);
            for chunk in chunks {
                let source = staging_path.join(&chunk.filename);
                let destination = local_directory.join(&chunk.filename);
                fs::rename(&source, &destination).with_context(|| {
                    format!(
                        "failed to move staged chunk {} to {}",
                        source.display(),
                        destination.display()
                    )
                })?;
            }
        }
        StreamOutputStorage::S3 { s3_config, .. } => {
            let client = build_s3_client(runtime, s3_config);
            runtime.block_on(async {
                for chunk in chunks {
                    let key = format!("{}{}", s3_config.key_prefix, chunk.filename);
                    upload_file_to_s3(
                        &client,
                        s3_config.bucket.as_str(),
                        &key,
                        &staging_path.join(&chunk.filename),
                    )
                    .await?;
                }
                Ok::<(), anyhow::Error>(())
            })?;
        }
    }
    Ok(())
}

/// Prepares the stream metadata collection using the configured results cache.
///
/// # Returns
///
/// The collection for stream metadata on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`parse_results_cache_uri`]'s return values on failure.
/// * Forwards [`Client::with_options`]'s return values on failure.
async fn prepare_metadata_collection(
    config: &ResultsCache,
) -> anyhow::Result<Collection<Document>> {
    let options = parse_results_cache_uri(config.uri().as_str()).await?;
    let client = Client::with_options(options)?;
    Ok(client
        .database(&config.db_name)
        .collection(&config.stream_collection_name))
}

/// Upserts stream metadata in bounded batches.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`Client::bulk_write`]'s return values on failure.
async fn upsert_metadata(
    collection: &Collection<Document>,
    batch: &ChunkBatch,
) -> anyhow::Result<()> {
    const BATCH_SIZE: usize = 1_000;

    let mut operations = batch.replacement_models(collection.namespace()).peekable();
    while operations.peek().is_some() {
        collection
            .client()
            .bulk_write(operations.by_ref().take(BATCH_SIZE))
            .await
            .context("failed to upsert ordered chunk metadata")?;
    }
    Ok(())
}

/// Extracts one archive into ordered chunks in `output_directory`.
///
/// # Returns
///
/// The chunk paths reported by clp-s on success, in emission order.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`anyhow::Error`] if:
///   * The configured storage engine is not [`StorageEngine::ClpS`].
///   * `clp-s` exits unsuccessfully.
/// * Forwards [`ArchiveDescriptor::resolve`]'s return values on failure.
/// * Forwards [`Command::spawn`]'s return values on failure.
/// * Forwards [`thread::Builder::spawn_scoped`]'s return values on failure.
/// * Forwards [`read_chunk_paths`]'s return values on failure.
/// * Forwards [`ChildGuard::wait`]'s return values on failure.
///
/// # Panics
///
/// Panics if either pipe is absent on the child spawned with piped stdout and stderr.
fn run_clp_s_extract(
    runtime: &tokio::runtime::Handle,
    clp_home: &Path,
    config: &SpiderTaskExecutorConfig,
    input: &ExtractTaskInput,
    output_directory: &Path,
) -> anyhow::Result<Vec<PathBuf>> {
    anyhow::ensure!(
        StorageEngine::ClpS == config.package.storage_engine,
        "the clp-s extraction task requires the `clp-s` storage engine"
    );

    let archive_descriptor =
        ArchiveDescriptor::resolve(runtime, clp_home, config, &input.dataset, input.archive_id)?;
    let clp_s_bin = clp_binary_path(clp_home, "clp-s");
    let mut command = build_clp_s_extract_command(
        &clp_s_bin,
        &archive_descriptor,
        output_directory,
        input.target_chunk_size,
    );
    thread::scope(|scope| {
        let child = command
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()
            .with_context(|| format!("failed to spawn clp-s at {}", clp_s_bin.display()))?;
        let mut guard = ChildGuard { child: Some(child) };
        let child = guard
            .child
            .as_mut()
            .expect("a newly created guard holds the child");
        let stdout = child
            .stdout
            .take()
            .expect("piped stdout should always be present");
        let stderr = child
            .stderr
            .take()
            .expect("piped stderr should always be present");
        let stderr_reader = thread::Builder::new()
            .spawn_scoped(scope, move || drain_stderr_tail(stderr))
            .context("failed to start the clp-s stderr reader")?;

        let result = (|| {
            // Own the guard here so a read failure or panic closes the child's pipes before the
            // stderr thread is joined.
            let guard = guard;
            let paths = read_chunk_paths(BufReader::new(stdout))?;
            let status = guard.wait().context("failed to wait for clp-s to exit")?;
            anyhow::ensure!(status.success(), "clp-s exited with {status}");
            Ok(paths)
        })();
        let stderr_tail = describe_stderr_tail(stderr_reader);
        result.with_context(|| format!("failed to extract with clp-s; stderr tail: {stderr_tail}"))
    })
}

/// A subprocess that is killed and reaped on drop unless already reaped.
#[must_use = "dropping the guard before waiting kills and reaps the subprocess"]
struct ChildGuard {
    child: Option<Child>,
}

impl ChildGuard {
    /// Waits for the subprocess and reaps it.
    ///
    /// # Returns
    ///
    /// The child's exit status on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Child::wait`]'s return values on failure.
    ///
    /// # Panics
    ///
    /// Panics if the guard no longer holds a child. Only this consuming method releases the child.
    fn wait(mut self) -> std::io::Result<ExitStatus> {
        let status = self
            .child
            .as_mut()
            .expect("the guard holds the child until it is reaped")
            .wait()?;
        self.child = None;
        Ok(status)
    }
}

impl Drop for ChildGuard {
    fn drop(&mut self) {
        let Some(mut child) = self.child.take() else {
            return;
        };
        if let Err(error) = child.kill() {
            tracing::warn!(error = % error, pid = child.id(), "Failed to kill clp-s.");
        }
        if let Err(error) = child.wait() {
            tracing::warn!(error = % error, pid = child.id(), "Failed to reap clp-s.");
        }
    }
}

#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct ChunkStatsLine {
    path: PathBuf,
}

/// Builds a command for extracting one archive with `clp-s x`.
///
/// # Returns
///
/// The extraction command, configured to report chunk statistics.
fn build_clp_s_extract_command(
    clp_s_bin: &Path,
    archive_descriptor: &ArchiveDescriptor,
    output_directory: &Path,
    target_chunk_size: Option<NonZeroU64>,
) -> Command {
    let mut command = archive_descriptor.clp_s_command(clp_s_bin, "x");
    command.arg(output_directory).arg("--ordered");
    if let Some(target_chunk_size) = target_chunk_size {
        command
            .arg("--target-ordered-chunk-size")
            .arg(target_chunk_size.to_string());
    }
    command.arg("--print-ordered-chunk-stats");
    command
}

/// Reads newline-delimited clp-s chunk statistics from `reader`.
///
/// # Returns
///
/// The reported paths on success, in input order.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`anyhow::Error`] if:
///   * A line is missing its terminating newline.
///   * A line exceeds [`MAX_STATS_LINE_SIZE`] bytes before its newline.
///   * The output exceeds [`MAX_STATS_OUTPUT_SIZE`] bytes or [`MAX_CHUNKS`] records.
///   * A line does not start with `{` after skipping ASCII whitespace.
/// * Forwards [`BufRead::read_until`]'s return values on failure.
/// * Forwards [`serde_json::from_slice`]'s return values on failure.
///
/// # Panics
///
/// Panics if the constant line size plus its newline does not fit in a `u64`.
fn read_chunk_paths(mut reader: impl BufRead) -> anyhow::Result<Vec<PathBuf>> {
    let read_limit = u64::try_from(MAX_STATS_READ_SIZE)
        .expect("the maximum line size plus a newline should fit in a u64");
    let mut paths = Vec::new();
    let mut line = Vec::new();
    let mut total_size = 0;
    loop {
        line.clear();
        let bytes_read = (&mut reader)
            .take(read_limit)
            .read_until(b'\n', &mut line)
            .context("failed to read clp-s chunk statistics")?;
        if 0 == bytes_read {
            return Ok(paths);
        }

        let content_size = line.len() - usize::from(line.ends_with(b"\n"));
        anyhow::ensure!(
            content_size <= MAX_STATS_LINE_SIZE,
            "clp-s chunk statistics line exceeds {MAX_STATS_LINE_SIZE} bytes"
        );
        let content = line
            .strip_suffix(b"\n")
            .context("unterminated clp-s chunk statistics line")?;
        total_size += bytes_read;
        anyhow::ensure!(
            total_size <= MAX_STATS_OUTPUT_SIZE,
            "clp-s chunk statistics exceed {MAX_STATS_OUTPUT_SIZE} bytes"
        );
        anyhow::ensure!(
            paths.len() < MAX_CHUNKS,
            "clp-s reported more than {MAX_CHUNKS} chunks"
        );
        anyhow::ensure!(
            content.iter().find(|byte| !byte.is_ascii_whitespace()) == Some(&b'{'),
            "clp-s chunk statistics must be JSON objects"
        );
        let stats: ChunkStatsLine =
            serde_json::from_slice(content).context("malformed clp-s chunk statistics")?;
        paths.push(stats.path);
    }
}

/// Drains the child's stderr while retaining its most recent bytes.
///
/// # Returns
///
/// Up to [`MAX_STDERR_TAIL_SIZE`] bytes from the end of stderr on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`Read::read_to_end`]'s return values on failure.
fn drain_stderr_tail(mut stderr: impl Read) -> std::io::Result<Vec<u8>> {
    const READ_SIZE: u64 = 8 * 1024;

    let mut tail = Vec::new();
    while 0 != (&mut stderr).take(READ_SIZE).read_to_end(&mut tail)? {
        if tail.len() > MAX_STDERR_TAIL_SIZE {
            tail.drain(..tail.len() - MAX_STDERR_TAIL_SIZE);
        }
    }
    Ok(tail)
}

/// Joins the stderr reader and describes the diagnostic it captured.
///
/// # Returns
///
/// The tail decoded lossily as UTF-8, or a description of why the reader failed.
fn describe_stderr_tail(reader: ScopedJoinHandle<'_, std::io::Result<Vec<u8>>>) -> String {
    match reader.join() {
        Ok(Ok(tail)) => String::from_utf8_lossy(&tail).into_owned(),
        Ok(Err(error)) => format!("<failed to read clp-s stderr: {error}>"),
        Err(_) => "<clp-s stderr reader panicked>".to_owned(),
    }
}

#[cfg(test)]
mod tests {
    use std::ffi::OsStr;
    use std::fs;
    use std::io;
    use std::io::BufReader;
    use std::io::Read;
    use std::num::NonZeroU64;
    use std::os::unix::fs::PermissionsExt;
    use std::path::Path;
    use std::path::PathBuf;
    use std::process::Command;
    use std::process::Stdio;
    use std::time::Duration;

    use clp_rust_utils::clp_config::package::config::ArchiveOutput;
    use clp_rust_utils::clp_config::package::config::ArchiveOutputStorage;
    use clp_rust_utils::clp_config::package::config::Package;
    use clp_rust_utils::clp_config::package::config::ResultsCache;
    use clp_rust_utils::clp_config::package::config::SpiderTaskExecutorConfig;
    use clp_rust_utils::clp_config::package::config::StorageEngine;
    use clp_rust_utils::clp_config::package::config::StreamOutput;
    use clp_rust_utils::clp_config::package::config::StreamOutputStorage;
    use clp_rust_utils::dataset::DatasetName;
    use clp_rust_utils::task_io::extraction::ExtractTaskInput;
    use clp_rust_utils::types::ArchiveId;
    use mongodb::Namespace;
    use mongodb::bson::doc;

    use super::ChunkBatch;
    use super::MAX_CHUNKS;
    use super::MAX_STATS_LINE_SIZE;
    use super::MAX_STATS_OUTPUT_SIZE;
    use super::MAX_STDERR_TAIL_SIZE;
    use super::build_clp_s_extract_command;
    use super::extract;
    use super::publish_files;
    use super::read_chunk_paths;
    use super::run_clp_s_extract;
    use crate::task::query::archive_input::ArchiveDescriptor;
    use crate::task::query::archive_input::base_s3_config;

    const ARCHIVE_ID: &str = "018e90e5-8b2a-4a61-a2fc-cac799936caf";

    /// A stream of repeated records backed by one borrowed record.
    struct RepeatedRecords<'record> {
        record: &'record [u8],
        unread: &'record [u8],
        remaining_records: usize,
    }

    impl<'record> RepeatedRecords<'record> {
        /// # Returns
        ///
        /// A stream of `num_records` copies of `record`.
        fn new(record: &'record [u8], num_records: usize) -> Self {
            Self {
                record,
                unread: &[],
                remaining_records: num_records,
            }
        }
    }

    impl Read for RepeatedRecords<'_> {
        fn read(&mut self, buffer: &mut [u8]) -> std::io::Result<usize> {
            if self.unread.is_empty() && self.remaining_records > 0 {
                self.unread = self.record;
                self.remaining_records -= 1;
            }
            self.unread.read(buffer)
        }
    }

    #[test]
    fn parse_all_builds_metadata_for_contiguous_chunks() -> anyhow::Result<()> {
        let staging_path = Path::new("/staging");
        let archive_id = ARCHIVE_ID.parse()?;
        let cases: &[&[(i64, i64, bool)]] = &[
            &[(0, 7, true)],
            &[(0, 2, false), (2, 42, true)],
            &[(0, 3, false), (3, 8, false), (8, 17, true)],
        ];
        for &expected in cases {
            let filenames: Vec<_> = expected
                .iter()
                .map(|(begin, end, _)| format!("{ARCHIVE_ID}_{begin}_{end}.jsonl"))
                .collect();
            let paths: Vec<_> = filenames
                .iter()
                .map(|filename| staging_path.join(filename))
                .collect();
            let batch = ChunkBatch::parse(staging_path, archive_id, &paths)?;
            let expected_documents: Vec<_> = filenames
                .iter()
                .zip(expected)
                .map(|(filename, &(begin, end, is_last))| {
                    doc! {
                        "_id": filename,
                        "path": filename,
                        "stream_id": ARCHIVE_ID,
                        "begin_msg_ix": begin,
                        "end_msg_ix": end,
                        "is_last_chunk": is_last,
                    }
                })
                .collect();
            assert_eq!(
                batch
                    .replacement_models(Namespace::new("db", "streams"))
                    .map(|operation| operation.replacement)
                    .collect::<Vec<_>>(),
                expected_documents
            );
        }
        Ok(())
    }

    #[test]
    fn parse_all_rejects_malformed_chunk_paths() {
        let staging_path = Path::new("/staging");
        let archive_id = ARCHIVE_ID
            .parse()
            .expect("the fixture has a valid archive ID");
        let filename = format!("{ARCHIVE_ID}_0_1.jsonl");
        for path in [
            PathBuf::from(&filename),
            Path::new("/other").join(&filename),
            staging_path.join("nested").join(&filename),
            staging_path.join("nested/..").join(&filename),
            staging_path.join(format!("{filename}/")),
            staging_path.join("68243977-37d6-46e1-a02d-7d0f08b04f70_0_1.jsonl"),
            staging_path.join(format!("{ARCHIVE_ID}_0_1.json")),
            staging_path.join(format!("{ARCHIVE_ID}_0_1_2.jsonl")),
            staging_path.join(format!("{ARCHIVE_ID}__1.jsonl")),
            staging_path.join(format!("{ARCHIVE_ID}_-0_1.jsonl")),
            staging_path.join(format!("{ARCHIVE_ID}_0_+1.jsonl")),
            staging_path.join(format!("{ARCHIVE_ID}_0_01.jsonl")),
            staging_path.join(format!("{ARCHIVE_ID}_0_9223372036854775808.jsonl")),
        ] {
            assert!(
                ChunkBatch::parse(staging_path, archive_id, std::slice::from_ref(&path)).is_err(),
                "accepted malformed chunk path {path:?}"
            );
        }
    }

    #[test]
    fn parse_all_rejects_noncontiguous_or_nonpositive_ranges() {
        let staging_path = Path::new("/staging");
        let archive_id = ARCHIVE_ID
            .parse()
            .expect("the fixture has a valid archive ID");
        for ranges in [
            ["1_2", "2_3"],
            ["0_2", "3_5"],
            ["0_2", "1_3"],
            ["0_2", "0_2"],
            ["0_2", "2_2"],
            ["0_2", "2_1"],
        ] {
            let paths =
                ranges.map(|range| staging_path.join(format!("{ARCHIVE_ID}_{range}.jsonl")));
            let error = ChunkBatch::parse(staging_path, archive_id, &paths)
                .expect_err("invalid chunk ranges should be rejected");
            assert!(error.to_string().starts_with("invalid range"), "{error:#}");
        }
    }

    #[test]
    fn parse_all_accepts_an_empty_ordered_stream() -> anyhow::Result<()> {
        assert_eq!(
            ChunkBatch::parse(Path::new("/staging"), ARCHIVE_ID.parse()?, &[])?.chunks,
            []
        );
        Ok(())
    }

    #[test]
    fn extract_validates_the_results_cache_only_when_ordered_chunks_exist() -> anyhow::Result<()> {
        let filename = format!("{ARCHIVE_ID}_0_1.jsonl");
        for (body, expected_error) in [
            (":".to_owned(), None),
            (
                r#"touch "$5/original""#.to_owned(),
                Some("clp-s produced unordered output; the archive lacks log ordering"),
            ),
            (
                format!(
                    r#"touch "$5/{filename}"
printf '{{"path":"%s"}}\n' "$5/{filename}""#
                ),
                Some("results-cache URI must name a database"),
            ),
        ] {
            let clp_home = tempfile::tempdir()?;
            let config = SpiderTaskExecutorConfig {
                package: Package {
                    storage_engine: StorageEngine::ClpS,
                },
                stream_output: StreamOutput {
                    storage: StreamOutputStorage::Fs {
                        directory: "streams".to_owned(),
                    },
                    ..StreamOutput::default()
                },
                results_cache: ResultsCache {
                    db_name: String::new(),
                    ..ResultsCache::default()
                },
                ..SpiderTaskExecutorConfig::default()
            };
            let check_path = r#"test -d "$5"
case "$5" in
    */streams/.staging/42.19) ;;
    *) exit 1 ;;
esac"#;
            install_stand_in(clp_home.path(), &format!("{check_path}\n{body}"))?;

            let runtime = tokio::runtime::Builder::new_current_thread().build()?;
            let input = ExtractTaskInput {
                job_id: 42,
                dataset: DatasetName::default(),
                archive_id: ARCHIVE_ID.parse()?,
                target_chunk_size: None,
            };
            let result = extract(runtime.handle(), clp_home.path(), &config, 19, &input);
            if let Some(expected_error) = expected_error {
                assert_eq!(
                    result.expect_err("extraction should fail").to_string(),
                    expected_error
                );
            } else {
                result?;
            }
            let streams_directory = clp_home.path().join("streams");
            assert_eq!(
                fs::symlink_metadata(streams_directory.join(&filename))
                    .expect_err("no chunk should be published")
                    .kind(),
                io::ErrorKind::NotFound
            );
            assert_eq!(
                fs::read_dir(streams_directory.join(".staging"))?
                    .map(|entry| entry.map(|entry| entry.path()))
                    .collect::<io::Result<Vec<_>>>()?,
                Vec::<PathBuf>::new()
            );
        }
        Ok(())
    }

    #[test]
    fn publish_files_renames_regular_chunks_over_previous_outputs() -> anyhow::Result<()> {
        let clp_home = tempfile::tempdir()?;
        let config = SpiderTaskExecutorConfig {
            stream_output: StreamOutput {
                storage: StreamOutputStorage::Fs {
                    directory: "streams".to_owned(),
                },
                ..StreamOutput::default()
            },
            ..SpiderTaskExecutorConfig::default()
        };
        let destination = clp_home.path().join("streams");
        fs::create_dir(&destination)?;
        let staging = tempfile::tempdir_in(&destination)?;
        let filenames = [
            format!("{ARCHIVE_ID}_0_2.jsonl"),
            format!("{ARCHIVE_ID}_2_5.jsonl"),
        ];
        let paths = filenames.each_ref().map(|name| staging.path().join(name));
        fs::write(destination.join(&filenames[0]), "previous attempt")?;
        for (path, filename) in paths.iter().zip(&filenames) {
            fs::write(path, filename)?;
        }
        let batch = ChunkBatch::parse(staging.path(), ARCHIVE_ID.parse()?, &paths)?;
        let runtime = tokio::runtime::Builder::new_current_thread().build()?;
        publish_files(
            runtime.handle(),
            clp_home.path(),
            &config,
            staging.path(),
            &batch.chunks,
        )?;

        for (path, filename) in paths.iter().zip(&filenames) {
            assert_eq!(fs::read_to_string(destination.join(filename))?, *filename);
            assert_eq!(
                fs::symlink_metadata(path)
                    .expect_err("the chunk should be moved")
                    .kind(),
                io::ErrorKind::NotFound
            );
        }
        Ok(())
    }

    #[test]
    fn extract_args_use_the_filesystem_archive_id_without_a_chunk_target() {
        let descriptor = ArchiveDescriptor::Directory {
            path: PathBuf::from("/archives/ds1"),
            archive_id: ARCHIVE_ID
                .parse()
                .expect("the fixture has a valid archive ID"),
        };
        let command = build_clp_s_extract_command(
            Path::new("clp-s"),
            &descriptor,
            Path::new("/staging"),
            None,
        );
        assert_eq!(
            command.get_args().collect::<Vec<_>>(),
            [
                "x",
                "/archives/ds1",
                "--archive-id",
                ARCHIVE_ID,
                "/staging",
                "--ordered",
                "--print-ordered-chunk-stats",
            ]
            .map(OsStr::new)
        );
    }

    #[test]
    fn extract_args_use_s3_auth_and_include_the_chunk_target() {
        const URL: &str = "https://bucket.s3.amazonaws.com/archives/ds1/archive";
        let descriptor = ArchiveDescriptor::S3 {
            url: URL.to_owned(),
            credential_env: Vec::new(),
        };
        let command = build_clp_s_extract_command(
            Path::new("clp-s"),
            &descriptor,
            Path::new("/staging"),
            NonZeroU64::new(1024),
        );
        assert_eq!(
            command.get_args().collect::<Vec<_>>(),
            [
                "x",
                URL,
                "--auth",
                "s3",
                "/staging",
                "--ordered",
                "--target-ordered-chunk-size",
                "1024",
                "--print-ordered-chunk-stats",
            ]
            .map(OsStr::new)
        );
    }

    #[test]
    fn read_chunk_paths_rejects_malformed_and_unterminated_records() {
        for (stats, expected_error) in [
            (
                "{\"path\":\"/staging/a\"}",
                "unterminated clp-s chunk statistics line",
            ),
            ("not JSON\n", "clp-s chunk statistics must be JSON objects"),
            (
                "[\"/staging/a\"]\n",
                "clp-s chunk statistics must be JSON objects",
            ),
            (
                "{\"other_field\":1,\"path\":\"a\"}\n",
                "malformed clp-s chunk statistics",
            ),
            (
                "{\"path\":\"/staging/a\"\n",
                "malformed clp-s chunk statistics",
            ),
        ] {
            let error = read_chunk_paths(stats.as_bytes())
                .expect_err("invalid chunk statistics should be rejected");
            assert_eq!(error.to_string(), expected_error, "stats: {stats:?}");
        }
    }

    #[test]
    fn read_chunk_paths_accepts_a_record_at_the_line_limit() -> anyhow::Result<()> {
        let mut record = b" {\"path\":\"/staging/a\"}".to_vec();
        record.resize(MAX_STATS_LINE_SIZE, b' ');
        record.push(b'\n');

        assert_eq!(
            read_chunk_paths(record.as_slice())?,
            [PathBuf::from("/staging/a")]
        );
        Ok(())
    }

    #[test]
    fn read_chunk_paths_enforces_the_aggregate_stdout_limit() -> anyhow::Result<()> {
        let num_records = MAX_STATS_OUTPUT_SIZE.div_ceil(MAX_STATS_LINE_SIZE);
        assert!(num_records < MAX_CHUNKS);
        let record_size = MAX_STATS_OUTPUT_SIZE / num_records;
        let num_larger_records = MAX_STATS_OUTPUT_SIZE % num_records;
        let padded_record = |size| {
            let mut record = b"{\"path\":\"a\"}".to_vec();
            assert!(record.len() < size);
            record.resize(size - 1, b' ');
            record.push(b'\n');
            record
        };
        let record = padded_record(record_size);
        let larger_record = padded_record(record_size + 1);
        let at_limit = || {
            RepeatedRecords::new(&record, num_records - num_larger_records)
                .chain(RepeatedRecords::new(&larger_record, num_larger_records))
        };

        assert_eq!(
            read_chunk_paths(BufReader::new(at_limit()))?.len(),
            num_records
        );

        let over_limit = at_limit().chain(b"{\"path\":\"b\"}\n".as_slice());
        let error = read_chunk_paths(BufReader::new(over_limit))
            .map(|paths| paths.len())
            .expect_err("one more record should exceed the stdout limit");
        assert_eq!(
            error.to_string(),
            format!("clp-s chunk statistics exceed {MAX_STATS_OUTPUT_SIZE} bytes")
        );
        Ok(())
    }

    #[test]
    fn read_chunk_paths_enforces_the_chunk_count_limit() -> anyhow::Result<()> {
        let record = b"{\"path\":\"a\"}\n";
        assert!(MAX_CHUNKS < MAX_STATS_OUTPUT_SIZE / record.len());

        let at_limit = BufReader::new(RepeatedRecords::new(record, MAX_CHUNKS));
        assert_eq!(read_chunk_paths(at_limit)?.len(), MAX_CHUNKS);

        let over_limit = BufReader::new(RepeatedRecords::new(record, MAX_CHUNKS + 1));
        let error = read_chunk_paths(over_limit)
            .map(|paths| paths.len())
            .expect_err("one more record should exceed the chunk limit");
        assert_eq!(
            error.to_string(),
            format!("clp-s reported more than {MAX_CHUNKS} chunks")
        );
        Ok(())
    }

    #[test]
    fn run_clp_s_extract_preserves_path_order_and_reaps_the_child() -> anyhow::Result<()> {
        const TARGET_CHUNK_SIZE: NonZeroU64 = NonZeroU64::new(1024).expect("1024 is nonzero");

        let clp_home = tempfile::tempdir()?;
        let archive_output = ArchiveOutput {
            storage: ArchiveOutputStorage::S3 {
                staging_directory: "unused".to_owned(),
                s3_config: base_s3_config(),
            },
            ..ArchiveOutput::default()
        };
        let script = format!(
            r#"
printf '%s\n' "$$" > "$0.pid"
test "$AWS_ACCESS_KEY_ID" = the-access-key
test "$AWS_SECRET_ACCESS_KEY" = the-secret-key
while [ "$#" -gt 0 ] && [ "$1" != --target-ordered-chunk-size ]; do
    shift
done
if [ "$#" -lt 2 ] || [ "$2" != "{TARGET_CHUNK_SIZE}" ]; then
    printf '%s\n' 'expected --target-ordered-chunk-size {TARGET_CHUNK_SIZE}' >&2
    exit 1
fi
head -c 131072 /dev/zero >&2
printf '%s\n' '{{"path":"/staging/z"}}' '{{"path":"/staging/a"}}'
"#
        );

        assert_eq!(
            run_stand_in(
                clp_home.path(),
                archive_output,
                Some(TARGET_CHUNK_SIZE),
                &script
            )?,
            [PathBuf::from("/staging/z"), PathBuf::from("/staging/a")]
        );
        assert_child_reaped(clp_home.path());
        Ok(())
    }

    #[test]
    fn run_clp_s_extract_reports_the_bounded_stderr_tail_on_nonzero_exit() -> anyhow::Result<()> {
        let clp_home = tempfile::tempdir()?;
        let script = format!(
            r"printf 'discarded prefix\n' >&2
head -c {MAX_STDERR_TAIL_SIZE} /dev/zero | tr '\000' x >&2
printf 'last error\n' >&2
exit 23"
        );
        let error = run_stand_in(clp_home.path(), ArchiveOutput::default(), None, &script)
            .expect_err("a failed clp-s exit should be reported even with no stats");

        assert_eq!(
            error.root_cause().to_string(),
            "clp-s exited with exit status: 23"
        );
        let stderr_tail = error.to_string();
        let suffix = "last error\n";
        assert_eq!(
            stderr_tail,
            format!(
                "failed to extract with clp-s; stderr tail: {}{suffix}",
                "x".repeat(MAX_STDERR_TAIL_SIZE - suffix.len())
            )
        );
        Ok(())
    }

    #[test]
    fn run_clp_s_extract_kills_and_reaps_the_child_on_an_overlong_line() -> anyhow::Result<()> {
        let clp_home = tempfile::tempdir()?;
        let line_size = MAX_STATS_LINE_SIZE + 1;
        let script = format!(
            r#"printf '%s\n' "$$" > "$0.pid"
head -c {line_size} /dev/zero | tr '\000' x
exec sleep 30"#
        );
        let error = run_stand_in(clp_home.path(), ArchiveOutput::default(), None, &script)
            .expect_err("an overlong stats record should fail extraction");

        assert_eq!(
            error.root_cause().to_string(),
            format!("clp-s chunk statistics line exceeds {MAX_STATS_LINE_SIZE} bytes")
        );
        assert_child_reaped(clp_home.path());
        Ok(())
    }

    /// Executes a stand-in for clp-s and bounds how long the test waits for the runner.
    ///
    /// # Returns
    ///
    /// The chunk paths reported by the stand-in on success, in emission order.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`run_clp_s_extract`]'s return values on failure.
    ///
    /// # Panics
    ///
    /// Panics if the fixture cannot be set up or if the runner panics or fails to return promptly.
    fn run_stand_in(
        clp_home: &Path,
        archive_output: ArchiveOutput,
        target_chunk_size: Option<NonZeroU64>,
        script: &str,
    ) -> anyhow::Result<Vec<PathBuf>> {
        const RETURN_TIMEOUT: Duration = Duration::from_secs(10);
        const RESULT_CHANNEL_CAPACITY: usize = 1;

        install_stand_in(clp_home, script).expect("the stand-in clp-s should be installed");
        let clp_home = clp_home.to_path_buf();
        let (sender, receiver) = std::sync::mpsc::sync_channel(RESULT_CHANNEL_CAPACITY);
        let worker = std::thread::spawn(move || {
            let runtime = tokio::runtime::Builder::new_current_thread()
                .build()
                .expect("a Tokio runtime is needed to resolve archive credentials");
            let config = SpiderTaskExecutorConfig {
                package: Package {
                    storage_engine: StorageEngine::ClpS,
                },
                archive_output,
                ..SpiderTaskExecutorConfig::default()
            };
            let input = ExtractTaskInput {
                job_id: 42,
                dataset: DatasetName::default(),
                archive_id: ARCHIVE_ID
                    .parse::<ArchiveId>()
                    .expect("the fixture has a valid archive ID"),
                target_chunk_size,
            };
            let result = run_clp_s_extract(
                runtime.handle(),
                &clp_home,
                &config,
                &input,
                &clp_home.join("staging"),
            );
            sender
                .send(result)
                .expect("the test should be waiting for the runner");
        });
        let result = receiver
            .recv_timeout(RETURN_TIMEOUT)
            .expect("the clp-s runner should return promptly");
        worker.join().expect("the runner thread should finish");
        result
    }

    /// Installs an executable stand-in for clp-s.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`fs::create_dir`]'s return values on failure.
    /// * Forwards [`fs::write`]'s return values on failure.
    /// * Forwards [`fs::set_permissions`]'s return values on failure.
    fn install_stand_in(clp_home: &Path, script: &str) -> io::Result<()> {
        let bin_directory = clp_home.join("bin");
        let clp_s_path = bin_directory.join("clp-s");
        fs::create_dir(bin_directory)?;
        fs::write(&clp_s_path, format!("#!/bin/sh\nset -eu\n{script}\n"))?;
        fs::set_permissions(clp_s_path, fs::Permissions::from_mode(0o755))
    }

    /// Asserts that the stand-in process recorded by the fixture no longer exists.
    ///
    /// # Panics
    ///
    /// Panics if the fixture did not record its PID, kill cannot run, or the process still exists.
    fn assert_child_reaped(clp_home: &Path) {
        let pid = std::fs::read_to_string(clp_home.join("bin/clp-s.pid"))
            .expect("the child wrote its PID");
        let status = Command::new("kill")
            .args(["-0", pid.trim()])
            .stderr(Stdio::null())
            .status()
            .expect("kill -0 should check for the child's PID");
        assert!(
            !status.success(),
            "clp-s (pid {}) should be reaped",
            pid.trim()
        );
    }
}

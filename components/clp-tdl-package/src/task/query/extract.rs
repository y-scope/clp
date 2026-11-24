//! Ordered chunk extraction through the `clp-s` subprocess.

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
use clp_rust_utils::clp_config::package::config::SpiderTaskExecutorConfig;
use clp_rust_utils::clp_config::package::config::StorageEngine;
use clp_rust_utils::task_io::extraction::ExtractTaskInput;
use serde::Deserialize;

use crate::task::query::archive_input::ArchiveDescriptor;
use crate::task::utils::clp_binary_path;

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
#[cfg_attr(
    not(test),
    expect(
        dead_code,
        reason = "the extraction task will call the runner once registered"
    )
)]
pub(super) fn run_clp_s_extract(
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

    let archive_descriptor = ArchiveDescriptor::resolve(
        runtime,
        clp_home,
        config,
        &input.dataset,
        input.archive_uuid,
    )?;
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
    use clp_rust_utils::clp_config::package::config::SpiderTaskExecutorConfig;
    use clp_rust_utils::clp_config::package::config::StorageEngine;
    use clp_rust_utils::dataset::DatasetName;
    use clp_rust_utils::task_io::extraction::ExtractTaskInput;
    use clp_rust_utils::types::ArchiveUuid;

    use super::MAX_CHUNKS;
    use super::MAX_STATS_LINE_SIZE;
    use super::MAX_STATS_OUTPUT_SIZE;
    use super::MAX_STDERR_TAIL_SIZE;
    use super::build_clp_s_extract_command;
    use super::read_chunk_paths;
    use super::run_clp_s_extract;
    use crate::task::query::archive_input::ArchiveDescriptor;
    use crate::task::query::archive_input::base_s3_config;

    const ARCHIVE_UUID: &str = "018e90e5-8b2a-4a61-a2fc-cac799936caf";

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
    fn extract_args_use_the_filesystem_archive_uuid_without_a_chunk_target() {
        let descriptor = ArchiveDescriptor::Directory {
            path: PathBuf::from("/archives/ds1"),
            archive_uuid: ARCHIVE_UUID
                .parse()
                .expect("the fixture has a valid archive UUID"),
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
                ARCHIVE_UUID,
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

        let bin_directory = clp_home.join("bin");
        let clp_s_path = bin_directory.join("clp-s");
        std::fs::create_dir(bin_directory).expect("the fixture's bin directory should be created");
        std::fs::write(&clp_s_path, format!("#!/bin/sh\nset -eu\n{script}\n"))
            .expect("the stand-in clp-s script should be written");
        std::fs::set_permissions(clp_s_path, std::fs::Permissions::from_mode(0o755))
            .expect("the stand-in clp-s script should be executable");
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
                archive_uuid: ARCHIVE_UUID
                    .parse::<ArchiveUuid>()
                    .expect("the fixture has a valid archive UUID"),
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

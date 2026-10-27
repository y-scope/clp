//! Partitioning of discovered files and S3 objects into compression-task inputs.

use std::collections::VecDeque;
use std::path::Path;

use clp_rust_utils::clp_config::S3Config;
use clp_rust_utils::s3::ObjectMetadata;
use clp_rust_utils::task_io::compression::CompressionInputSource;
use clp_rust_utils::task_io::compression::FsInputSource;
use clp_rust_utils::task_io::compression::S3InputSource;

use crate::Error;

/// Source-specific entry validation and compression-task manifest construction.
pub trait Source {
    /// The entries accepted by this source.
    type Entry;

    /// Validates an entry and prepares its file metadata for partitioning.
    ///
    /// # Returns
    ///
    /// The entry's raw file metadata, or `None` if the source retains it for finalization, on
    /// success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`Error`] if the entry violates the source's input constraints.
    fn prepare_entry(&mut self, entry: Self::Entry) -> Result<Option<InputMetadata>, Error>;

    /// Builds task manifests from file partitions and retained entries, preserving every entry.
    ///
    /// # Parameters
    ///
    /// * `partitions` - File partitions, each containing between 1 and 4,096 paths.
    ///
    /// # Returns
    ///
    /// Manifests containing at most 4,096 entries each. Retained entries are emitted in separate
    /// manifests from files.
    fn build_task_inputs(self, partitions: Vec<Vec<String>>) -> Vec<CompressionInputSource>;
}

/// An input path or object key and its raw size.
pub struct InputMetadata {
    pub path: String,
    pub size: u64,
}

/// Builds inputs for compression tasks.
///
/// The builder buffers input objects internally and groups them into partitions. Each partition
/// contains the inputs for a single compression task.
///
/// # Type Parameters
///
/// * `SourceType` - The source that validates entries and builds compression task manifests.
pub struct CompressionInputBuilder<SourceType: Source> {
    buffer: Vec<FileMetadata>,
    partitioned_paths: Vec<Vec<String>>,
    total_buffered_size: u128,
    target_archive_size: u64,
    source: SourceType,
}

impl<SourceType: Source> CompressionInputBuilder<SourceType> {
    /// Factory function.
    ///
    /// # Returns
    ///
    /// A newly created [`CompressionInputBuilder`] with an empty buffer.
    #[must_use]
    pub const fn new(source: SourceType, target_archive_size: u64) -> Self {
        Self {
            buffer: Vec::new(),
            partitioned_paths: Vec::new(),
            total_buffered_size: 0,
            target_archive_size,
            source,
        }
    }

    /// Adds an input entry to the buffer.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Source::prepare_entry`]'s return values on failure.
    pub fn add(&mut self, entry: SourceType::Entry) -> Result<(), Error> {
        let Some(input) = self.source.prepare_entry(entry)? else {
            return Ok(());
        };
        let file = FileMetadata::new(input.path, input.size);
        self.total_buffered_size += u128::from(file.estimated_size);
        self.buffer.push(file);
        if self.total_buffered_size >= u128::from(self.target_archive_size.saturating_mul(2))
            || self.buffer.len() >= MAX_PARTITION_ENTRIES
        {
            self.partition(self.buffer.len() >= MAX_PARTITION_ENTRIES);
        }
        Ok(())
    }

    /// Partitions buffered objects into target-sized compression task inputs.
    ///
    /// When `flush_buffer` is false, objects that cannot fill another target-sized partition
    /// remain buffered. Otherwise, the final partial partition is also emitted.
    fn partition(&mut self, flush_buffer: bool) {
        if self.buffer.is_empty() {
            return;
        }

        let mut rr_iterator = RoundRobinIterator::new(std::mem::take(&mut self.buffer));

        while flush_buffer || self.total_buffered_size >= u128::from(self.target_archive_size) {
            let mut partition = Vec::new();
            let mut partition_size = 0_u64;

            for file in rr_iterator.by_ref().take(MAX_PARTITION_ENTRIES) {
                partition_size = partition_size.saturating_add(file.estimated_size);
                self.total_buffered_size -= u128::from(file.estimated_size);
                partition.push(file.path);
                if partition_size >= self.target_archive_size {
                    break;
                }
            }

            if partition.is_empty() {
                break;
            }
            self.partitioned_paths.push(partition);
        }

        self.buffer = rr_iterator.into();
    }
}

impl<SourceType: Source> From<CompressionInputBuilder<SourceType>> for Vec<CompressionInputSource> {
    fn from(mut builder: CompressionInputBuilder<SourceType>) -> Self {
        builder.partition(true);
        builder.source.build_task_inputs(builder.partitioned_paths)
    }
}

impl Source for S3Config {
    type Entry = ObjectMetadata;

    fn prepare_entry(&mut self, metadata: Self::Entry) -> Result<Option<InputMetadata>, Error> {
        if metadata.bucket != self.bucket {
            return Err(Error::S3BucketMismatch(
                String::from(self.bucket.clone()),
                String::from(metadata.bucket),
            ));
        }
        if !metadata.key.as_str().starts_with(self.key_prefix.as_str()) {
            return Err(Error::S3KeyPrefixMismatch(
                String::from(self.key_prefix.clone()),
                String::from(metadata.key),
            ));
        }
        Ok(Some(InputMetadata {
            path: String::from(metadata.key),
            size: metadata.size,
        }))
    }

    fn build_task_inputs(self, partitions: Vec<Vec<String>>) -> Vec<CompressionInputSource> {
        partitions
            .into_iter()
            .map(|object_keys| {
                CompressionInputSource::S3(S3InputSource {
                    endpoint_url: self.endpoint_url.clone(),
                    region_code: self.region_code.clone(),
                    bucket: self.bucket.clone(),
                    aws_authentication: self.aws_authentication.clone(),
                    object_keys,
                })
            })
            .collect()
    }
}

/// Filesystem entries supplied to the partition planner.
pub enum FsEntry {
    File { path: String, size: u64 },
    EmptyDirectory { path: String },
}

/// Filesystem source for compression-task planning.
#[derive(Default)]
pub struct FsSource {
    empty_directories: Vec<String>,
}

impl Source for FsSource {
    type Entry = FsEntry;

    fn prepare_entry(&mut self, entry: Self::Entry) -> Result<Option<InputMetadata>, Error> {
        match entry {
            FsEntry::File { path, size } => Ok(Some(InputMetadata { path, size })),
            FsEntry::EmptyDirectory { path } => {
                self.empty_directories.push(path);
                Ok(None)
            }
        }
    }

    fn build_task_inputs(self, partitions: Vec<Vec<String>>) -> Vec<CompressionInputSource> {
        let mut inputs: Vec<_> = partitions
            .into_iter()
            .map(|file_paths| {
                CompressionInputSource::Fs(FsInputSource {
                    file_paths,
                    empty_directories: Vec::new(),
                })
            })
            .collect();
        let mut directories = self.empty_directories.into_iter();
        loop {
            let chunk: Vec<_> = directories.by_ref().take(MAX_PARTITION_ENTRIES).collect();
            if chunk.is_empty() {
                break;
            }
            inputs.push(CompressionInputSource::Fs(FsInputSource {
                file_paths: Vec::new(),
                empty_directories: chunk,
            }));
        }
        inputs
    }
}

const MAX_PARTITION_ENTRIES: usize = 4096;

/// An input path and its estimated uncompressed size.
struct FileMetadata {
    path: String,
    estimated_size: u64,
}

impl FileMetadata {
    /// Creates metadata for the input identified by `path`, estimating its uncompressed size from
    /// its raw `size`.
    ///
    /// # Returns
    ///
    /// A new [`FileMetadata`].
    fn new(path: String, size: u64) -> Self {
        let estimated_size = estimate_uncompressed_size(&path, size);
        Self {
            path,
            estimated_size,
        }
    }
}

/// Owns filename-similarity groups and returns one object from each group in round-robin order.
struct RoundRobinIterator {
    groups: VecDeque<std::vec::IntoIter<FileMetadata>>,
}

impl RoundRobinIterator {
    /// Groups objects by filename similarity and creates a round-robin iterator over the groups.
    ///
    /// # Returns
    ///
    /// A new [`RoundRobinIterator`].
    fn new(files: Vec<FileMetadata>) -> Self {
        let groups = group_files_by_similar_filenames(files)
            .into_iter()
            .map(IntoIterator::into_iter)
            .collect();

        Self { groups }
    }
}

impl From<RoundRobinIterator> for Vec<FileMetadata> {
    fn from(iterator: RoundRobinIterator) -> Self {
        iterator.groups.into_iter().flatten().collect()
    }
}

impl Iterator for RoundRobinIterator {
    type Item = FileMetadata;

    fn next(&mut self) -> Option<Self::Item> {
        let mut group = self.groups.pop_front()?;
        let file = group.next().expect("filename groups must not be empty");
        if !group.as_slice().is_empty() {
            self.groups.push_back(group);
        }
        Some(file)
    }
}

/// Estimates an input's uncompressed size from its compressor suffix.
///
/// # Returns
///
/// The estimated uncompressed size.
fn estimate_uncompressed_size(path: &str, size: u64) -> u64 {
    const GZIP_COMPRESSION_RATIO_ESTIMATE: u64 = 13;
    const GZIP_SUFFIXES: &[&str] = &[".gz", ".gzip", ".tgz"];
    const ZSTD_COMPRESSION_RATIO_ESTIMATE: u64 = 8;

    if GZIP_SUFFIXES
        .iter()
        .any(|suffix| ends_with_ignore_ascii_case(path, suffix))
    {
        size.saturating_mul(GZIP_COMPRESSION_RATIO_ESTIMATE)
    } else if ends_with_ignore_ascii_case(path, ".zst") {
        size.saturating_mul(ZSTD_COMPRESSION_RATIO_ESTIMATE)
    } else {
        size
    }
}

/// # Returns
///
/// Whether `value` ends with `suffix` using ASCII case-insensitive match.
fn ends_with_ignore_ascii_case(value: &str, suffix: &str) -> bool {
    let suffix_start = value.len().saturating_sub(suffix.len());
    let value_suffix = value.get(suffix_start..);
    value_suffix.is_some_and(|value_suffix| value_suffix.eq_ignore_ascii_case(suffix))
}

/// Gets the filename portion of an input path.
///
/// # Returns
///
/// The path's filename, or the complete path if it has no filename component.
fn filename(path: &str) -> &str {
    let Some(file_name) = Path::new(path)
        .file_name()
        .and_then(|file_name| file_name.to_str())
    else {
        return path;
    };
    file_name
}

/// Groups input paths by filename similarity.
///
/// Places files with sufficiently similar filenames into the same group based on their normalized
/// Levenshtein similarity.
///
/// # Returns
///
/// A vector of filename-similarity groups.
fn group_files_by_similar_filenames(mut files: Vec<FileMetadata>) -> Vec<Vec<FileMetadata>> {
    const FILE_GROUPING_MIN_LEVENSHTEIN_RATIO: f64 = 0.6;

    files.sort_by(|a, b| filename(&a.path).cmp(filename(&b.path)));
    files
        .into_iter()
        .fold(Vec::new(), |mut groups: Vec<Vec<FileMetadata>>, curr| {
            match groups.last_mut() {
                Some(group)
                    if group.last().is_some_and(|prev| {
                        strsim::normalized_levenshtein(filename(&prev.path), filename(&curr.path))
                            >= FILE_GROUPING_MIN_LEVENSHTEIN_RATIO
                    }) =>
                {
                    group.push(curr);
                }
                _ => groups.push(vec![curr]),
            }
            groups
        })
}

#[cfg(test)]
mod tests {
    use std::collections::BTreeMap;

    use clp_rust_utils::clp_config::AwsAuthentication;
    use non_empty_string::NonEmptyString;

    use super::*;

    const TEST_BUCKET: &str = "test-bucket";

    /// # Type Parameters
    ///
    /// * `SourceType` - The source used to construct compression task manifests.
    struct Fixture<SourceType: Source> {
        builder: CompressionInputBuilder<SourceType>,
        path_to_size: BTreeMap<String, u64>,
    }

    impl<SourceType: Source> Fixture<SourceType> {
        /// # Returns
        ///
        /// An empty fixture with the supplied source and target archive size.
        fn new(source: SourceType, target_archive_size: u64) -> Self {
            Self {
                builder: CompressionInputBuilder::new(source, target_archive_size),
                path_to_size: BTreeMap::new(),
            }
        }

        /// Checks completed manifests and file partitions against the recorded inputs and target
        /// size.
        ///
        /// # Returns
        ///
        /// The completed task manifests.
        ///
        /// # Panics
        ///
        /// Panics if the file inputs differ from the recorded inputs, a manifest is empty or
        /// exceeds the entry limit, or a partition contains additional files after reaching the
        /// target size.
        fn assert_partitions(self) -> Vec<CompressionInputSource> {
            let target_archive_size = self.builder.target_archive_size;
            let input_sources = Vec::<CompressionInputSource>::from(self.builder);
            let mut path_counts: BTreeMap<&str, usize> = BTreeMap::new();
            for path in input_sources.iter().flat_map(CompressionInputSource::paths) {
                *path_counts.entry(path.as_str()).or_default() += 1;
            }
            for path in self.path_to_size.keys() {
                assert_eq!(path_counts.remove(path.as_str()), Some(1));
            }
            assert_eq!(path_counts, BTreeMap::new());

            for source in &input_sources {
                let paths = source.paths();
                let num_entries = match source {
                    CompressionInputSource::S3(source) => source.object_keys.len(),
                    CompressionInputSource::Fs(source) => {
                        source.file_paths.len() + source.empty_directories.len()
                    }
                };
                assert!((1..=MAX_PARTITION_ENTRIES).contains(&num_entries));
                let Some((_, preceding_paths)) = paths.split_last() else {
                    continue;
                };
                if target_archive_size == 0 {
                    assert_eq!(preceding_paths, [] as [String; 0]);
                } else {
                    let preceding_size: u128 = preceding_paths
                        .iter()
                        .map(|path| {
                            let size = self.path_to_size.get(path).expect("every path was added");
                            u128::from(estimate_uncompressed_size(path, *size))
                        })
                        .sum();
                    assert!(preceding_size < u128::from(target_archive_size));
                }
            }
            input_sources
        }
    }

    impl Fixture<S3Config> {
        /// Adds an S3 object and records its expected size.
        ///
        /// # Panics
        ///
        /// Panics if `key` is empty, does not match the fixture's prefix, or was already added.
        fn add(&mut self, key: String, size: u64) {
            self.builder
                .add(ObjectMetadata {
                    bucket: self.builder.source.bucket.clone(),
                    key: NonEmptyString::new(key.clone()).expect("fixture key must be non-empty"),
                    size,
                })
                .expect("fixture object must match its configured bucket and prefix");
            assert_eq!(self.path_to_size.insert(key, size), None);
        }
    }

    impl Fixture<FsSource> {
        /// Adds a filesystem entry and records a file's expected size.
        ///
        /// # Panics
        ///
        /// Panics if a file path was already added or the source rejects the entry.
        fn add(&mut self, entry: FsEntry) {
            if let FsEntry::File { path, size } = &entry {
                assert_eq!(self.path_to_size.insert(path.clone(), *size), None);
            }
            self.builder
                .add(entry)
                .expect("filesystem fixture entry should be accepted");
        }
    }

    #[test]
    fn s3_partition_on_flush() {
        const TARGET_ARCHIVE_SIZE: u64 = 1000;
        const FILE_SIZE: u64 = 100;
        const NUM_FILES: u64 = 15;

        let source = S3Config {
            bucket: NonEmptyString::try_from(TEST_BUCKET).expect("test bucket is non-empty"),
            region_code: None,
            key_prefix: NonEmptyString::try_from("logs/").expect("test prefix is non-empty"),
            endpoint_url: None,
            aws_authentication: AwsAuthentication::Default,
        };
        let mut fixture = Fixture::new(source, TARGET_ARCHIVE_SIZE);
        for i in 0..NUM_FILES {
            fixture.add(format!("logs/app.log.{i}"), FILE_SIZE);
        }

        assert_eq!(fixture.builder.partitioned_paths, [] as [Vec<String>; 0]);

        fixture.assert_partitions();
    }

    #[test]
    fn s3_partition_on_the_fly() {
        const TARGET_ARCHIVE_SIZE: u64 = 1000;
        const FILE_STEMS: &[&str] = &["app.log", "database.log", "kernel.log", "nginx.log"];
        const NUM_FILES_PER_STEM: u64 = 60;

        let source = S3Config {
            bucket: NonEmptyString::try_from(TEST_BUCKET).expect("test bucket is non-empty"),
            region_code: None,
            key_prefix: NonEmptyString::try_from("logs/").expect("test prefix is non-empty"),
            endpoint_url: None,
            aws_authentication: AwsAuthentication::Default,
        };
        let mut fixture = Fixture::new(source, TARGET_ARCHIVE_SIZE);
        for stem in FILE_STEMS {
            for i in 0..NUM_FILES_PER_STEM {
                fixture.add(format!("logs/{stem}.{i}"), 10 + (i % 7) * 30);
            }
        }

        assert_ne!(fixture.builder.partitioned_paths, [] as [Vec<String>; 0]);

        fixture.assert_partitions();
    }

    #[test]
    fn fs_manifests_bound_zero_size_files_and_empty_directories() {
        let count = MAX_PARTITION_ENTRIES + 17;
        let directories: Vec<_> = (0..count)
            .map(|index| format!("/logs/empty/{index}"))
            .collect();
        let mut fixture = Fixture::new(FsSource::default(), 1024);
        for directory in &directories {
            fixture.add(FsEntry::EmptyDirectory {
                path: directory.clone(),
            });
        }
        for index in 0..count {
            fixture.add(FsEntry::File {
                path: format!("/logs/file/{index}"),
                size: 0,
            });
        }
        let inputs = fixture.assert_partitions();
        let mut all_directories = Vec::new();
        for input in inputs {
            let CompressionInputSource::Fs(source) = input else {
                panic!("expected filesystem manifest")
            };
            assert!(source.file_paths.is_empty() || source.empty_directories.is_empty());
            all_directories.extend(source.empty_directories);
        }
        all_directories.sort();
        let mut expected_directories = directories;
        expected_directories.sort();
        assert_eq!(all_directories, expected_directories);

        let directory = "/logs/directory-only";
        let mut directory_fixture = Fixture::new(FsSource::default(), 1024);
        directory_fixture.add(FsEntry::EmptyDirectory {
            path: directory.to_owned(),
        });
        assert_eq!(
            directory_fixture.assert_partitions(),
            [CompressionInputSource::Fs(FsInputSource {
                file_paths: Vec::new(),
                empty_directories: vec![directory.to_owned()],
            }),]
        );
    }

    #[test]
    fn fs_extreme_sizes_partition_without_overflow_or_lost_inputs() {
        let mut zero_target_fixture = Fixture::new(FsSource::default(), 0);
        zero_target_fixture.add(FsEntry::File {
            path: "/logs/zero".to_owned(),
            size: 0,
        });
        let zero_target_inputs = zero_target_fixture.assert_partitions();
        assert_eq!(zero_target_inputs.len(), 1);
        assert_eq!(zero_target_inputs[0].paths(), ["/logs/zero"]);

        let mut fixture = Fixture::new(FsSource::default(), u64::MAX);
        let files = [
            ("/logs/a.gz", u64::MAX),
            ("/logs/b.zst", u64::MAX),
            ("/logs/c", 7),
        ];
        for (path, size) in files {
            fixture.add(FsEntry::File {
                path: path.to_owned(),
                size,
            });
        }
        let inputs = fixture.assert_partitions();
        let paths: Vec<_> = inputs
            .iter()
            .flat_map(CompressionInputSource::paths)
            .map(String::as_str)
            .collect();
        assert_eq!(paths, files.map(|(path, _)| path));
        assert_eq!(inputs.len(), files.len());
        assert_eq!(estimate_uncompressed_size("input.gz", u64::MAX), u64::MAX);
        assert_eq!(estimate_uncompressed_size("input.zst", u64::MAX), u64::MAX);

        let mut fixture = Fixture::new(FsSource::default(), u64::MAX - 1);
        for path in ["/logs/a", "/logs/b", "/logs/c"] {
            fixture.add(FsEntry::File {
                path: path.to_owned(),
                size: u64::MAX / 2,
            });
        }
        let inputs = fixture.assert_partitions();
        assert_eq!(inputs.len(), 2);
        assert_eq!(
            inputs
                .iter()
                .map(|source| source.paths().len())
                .collect::<Vec<_>>(),
            [2, 1]
        );
    }

    #[test]
    fn fs_estimate_uncompressed_size_for_gzip_suffix() {
        const FILE_SIZE: u64 = 100;

        for path in [
            "logs/app.log.gz",
            "logs/app.log.GZ",
            "logs/app.log.gzip",
            "logs/app.log.GZIP",
            "logs/app.log.tgz",
            "logs/app.log.TGZ",
            "logs/app.log.tar.gz",
            "logs/app.log.TAR.GZ",
        ] {
            assert_eq!(FILE_SIZE * 13, estimate_uncompressed_size(path, FILE_SIZE));
        }
    }

    #[test]
    fn fs_estimate_uncompressed_size_for_zstandard_suffix() {
        const FILE_SIZE: u64 = 100;

        for path in [
            "logs/app.log.zst",
            "logs/app.log.clp.zst",
            "logs/app.log.tar.zst",
            "logs/app.log.ZST",
        ] {
            assert_eq!(FILE_SIZE * 8, estimate_uncompressed_size(path, FILE_SIZE));
        }
    }

    #[test]
    fn fs_round_robin_iterator() {
        let files = ["app.log", "app.log.1", "app.log.2"]
            .into_iter()
            .chain(["nginx.log", "nginx.log.1", "nginx.log.2"])
            .map(|key| FileMetadata::new(String::from(key), 1))
            .collect();

        let mut rr_iterator = RoundRobinIterator::new(files);
        let yielded: Vec<String> = rr_iterator.by_ref().take(3).map(|file| file.path).collect();
        assert_eq!(yielded, vec!["app.log", "nginx.log", "app.log.1"]);

        let mut remaining: Vec<String> = Vec::from(rr_iterator)
            .into_iter()
            .map(|file| file.path)
            .collect();
        remaining.sort();
        assert_eq!(remaining, vec!["app.log.2", "nginx.log.1", "nginx.log.2"]);
    }
}

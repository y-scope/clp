//! Partitioning of discovered files and S3 objects into compression-task inputs.

use std::collections::VecDeque;
use std::path::Path;

use clp_rust_utils::clp_config::S3Config;
use clp_rust_utils::logs_input::LogsInputPath;
use clp_rust_utils::s3::ObjectMetadata;
use clp_rust_utils::task_io::compression::FsInputSource;
use clp_rust_utils::task_io::compression::S3InputSource;

use crate::Error;

/// Validates input metadata and constructs compression-task inputs.
pub trait Source {
    /// The entries accepted by this source.
    type Entry;

    /// The source's file-path or object-key representation.
    type Path: AsRef<str>;

    /// The input payload for one compression task.
    type Manifest;

    /// Checks an entry and extracts its path and size for partitioning.
    ///
    /// # Returns
    ///
    /// The entry's path or object key and its size in bytes on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`Error::S3BucketMismatch`] if an S3 object's bucket differs from the configured bucket.
    /// * [`Error::S3KeyPrefixMismatch`] if an S3 object's key lacks the configured prefix.
    fn prepare_entry(&self, entry: Self::Entry) -> Result<InputMetadata<Self::Path>, Error>;

    /// Converts partitions into compression-task inputs.
    ///
    /// # Parameters
    ///
    /// * `partitions` - Partitions of input paths or object keys.
    ///
    /// # Returns
    ///
    /// One task input per supplied partition, preserving its paths.
    fn build_manifests(self, partitions: Vec<Vec<Self::Path>>) -> Vec<Self::Manifest>;
}

/// An input path or object key and its size in bytes.
///
/// # Type Parameters
///
/// * `PathType` - The source-specific file-path or object-key representation.
pub struct InputMetadata<PathType: AsRef<str>> {
    pub path: PathType,
    pub size: u64,
}

/// Groups files or S3 objects into compression tasks by estimated uncompressed size and entry
/// count.
///
/// # Type Parameters
///
/// * `SourceType` - The source that validates entries and builds compression task manifests.
pub struct CompressionInputBuilder<SourceType: Source> {
    buffer: Vec<FileMetadata<SourceType::Path>>,
    partitioned_paths: Vec<Vec<SourceType::Path>>,
    total_buffered_size: u128,
    target_archive_size: u64,
    source: SourceType,
}

impl<SourceType: Source> CompressionInputBuilder<SourceType> {
    /// Factory function.
    ///
    /// # Returns
    ///
    /// An empty builder using the given source and target archive size.
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

    /// Adds an entry to the compression inputs.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Source::prepare_entry`]'s return values on failure.
    pub fn add(&mut self, entry: SourceType::Entry) -> Result<(), Error> {
        let input = self.source.prepare_entry(entry)?;
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

    /// Finishes partitioning the buffered inputs.
    ///
    /// # Returns
    ///
    /// Task inputs containing every added entry once, or an empty list if no entries were added.
    #[must_use]
    pub fn build(mut self) -> Vec<SourceType::Manifest> {
        self.partition(true);
        self.source.build_manifests(self.partitioned_paths)
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

impl Source for S3Config {
    type Entry = ObjectMetadata;
    type Manifest = S3InputSource;
    type Path = String;

    fn prepare_entry(&self, metadata: Self::Entry) -> Result<InputMetadata<Self::Path>, Error> {
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
        Ok(InputMetadata {
            path: String::from(metadata.key),
            size: metadata.size,
        })
    }

    fn build_manifests(self, partitions: Vec<Vec<Self::Path>>) -> Vec<Self::Manifest> {
        partitions
            .into_iter()
            .map(|object_keys| S3InputSource {
                endpoint_url: self.endpoint_url.clone(),
                region_code: self.region_code.clone(),
                bucket: self.bucket.clone(),
                aws_authentication: self.aws_authentication.clone(),
                object_keys,
            })
            .collect()
    }
}

/// A local file's [`LogsInputPath`] and size in bytes.
pub struct FsEntry {
    pub path: LogsInputPath,
    pub size: u64,
}

/// Constructs [`FsInputSource`] task inputs from local files.
#[derive(Default)]
pub struct FsSource;

impl Source for FsSource {
    type Entry = FsEntry;
    type Manifest = FsInputSource;
    type Path = LogsInputPath;

    fn prepare_entry(&self, entry: Self::Entry) -> Result<InputMetadata<Self::Path>, Error> {
        Ok(InputMetadata {
            path: entry.path,
            size: entry.size,
        })
    }

    fn build_manifests(self, partitions: Vec<Vec<Self::Path>>) -> Vec<Self::Manifest> {
        partitions
            .into_iter()
            .map(|file_paths| FsInputSource { file_paths })
            .collect()
    }
}

const MAX_PARTITION_ENTRIES: usize = 4096;

/// An input path and its estimated uncompressed size.
///
/// # Type Parameters
///
/// * `PathType` - The source-specific file-path or object-key representation.
struct FileMetadata<PathType: AsRef<str>> {
    path: PathType,
    estimated_size: u64,
}

impl<PathType: AsRef<str>> FileMetadata<PathType> {
    /// Creates metadata for the input identified by `path`, estimating its uncompressed size from
    /// its raw `size`.
    ///
    /// # Returns
    ///
    /// A new [`FileMetadata`].
    fn new(path: PathType, size: u64) -> Self {
        let estimated_size = estimate_uncompressed_size(path.as_ref(), size);
        Self {
            path,
            estimated_size,
        }
    }
}

/// Owns filename-similarity groups and returns one object from each group in round-robin order.
///
/// # Type Parameters
///
/// * `PathType` - The source-specific file-path or object-key representation.
struct RoundRobinIterator<PathType: AsRef<str>> {
    groups: VecDeque<std::vec::IntoIter<FileMetadata<PathType>>>,
}

impl<PathType: AsRef<str>> RoundRobinIterator<PathType> {
    /// Groups objects by filename similarity and creates a round-robin iterator over the groups.
    ///
    /// # Returns
    ///
    /// A new [`RoundRobinIterator`].
    fn new(files: Vec<FileMetadata<PathType>>) -> Self {
        let groups = group_files_by_similar_filenames(files)
            .into_iter()
            .map(IntoIterator::into_iter)
            .collect();

        Self { groups }
    }
}

impl<PathType: AsRef<str>> From<RoundRobinIterator<PathType>> for Vec<FileMetadata<PathType>> {
    fn from(iterator: RoundRobinIterator<PathType>) -> Self {
        iterator.groups.into_iter().flatten().collect()
    }
}

impl<PathType: AsRef<str>> Iterator for RoundRobinIterator<PathType> {
    type Item = FileMetadata<PathType>;

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
/// # Type Parameters
///
/// * `PathType` - The source-specific file-path or object-key representation.
///
/// # Returns
///
/// A vector of filename-similarity groups.
fn group_files_by_similar_filenames<PathType: AsRef<str>>(
    mut files: Vec<FileMetadata<PathType>>,
) -> Vec<Vec<FileMetadata<PathType>>> {
    const FILE_GROUPING_MIN_LEVENSHTEIN_RATIO: f64 = 0.6;

    files.sort_by(|a, b| filename(a.path.as_ref()).cmp(filename(b.path.as_ref())));
    files.into_iter().fold(
        Vec::new(),
        |mut groups: Vec<Vec<FileMetadata<PathType>>>, curr| {
            match groups.last_mut() {
                Some(group)
                    if group.last().is_some_and(|prev| {
                        strsim::normalized_levenshtein(
                            filename(prev.path.as_ref()),
                            filename(curr.path.as_ref()),
                        ) >= FILE_GROUPING_MIN_LEVENSHTEIN_RATIO
                    }) =>
                {
                    group.push(curr);
                }
                _ => groups.push(vec![curr]),
            }
            groups
        },
    )
}

#[cfg(test)]
mod tests {
    use std::collections::BTreeMap;

    use clp_rust_utils::clp_config::AwsAuthentication;
    use non_empty_string::NonEmptyString;

    use super::*;

    const TEST_BUCKET: &str = "test-bucket";

    trait ManifestEntries {
        type Path: AsRef<str> + std::fmt::Debug + PartialEq;

        /// # Returns
        ///
        /// The file paths or object keys assigned to the task.
        fn file_paths(&self) -> &[Self::Path];
    }

    impl ManifestEntries for S3InputSource {
        type Path = String;

        fn file_paths(&self) -> &[Self::Path] {
            &self.object_keys
        }
    }

    impl ManifestEntries for FsInputSource {
        type Path = LogsInputPath;

        fn file_paths(&self) -> &[Self::Path] {
            &self.file_paths
        }
    }

    /// # Type Parameters
    ///
    /// * `SourceType` - The source used to construct compression task manifests.
    struct Fixture<SourceType: Source<Manifest: ManifestEntries>> {
        builder: CompressionInputBuilder<SourceType>,
        path_to_size: BTreeMap<String, u64>,
    }

    impl<SourceType: Source<Manifest: ManifestEntries>> Fixture<SourceType> {
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
        fn assert_partitions(self) -> Vec<SourceType::Manifest> {
            let target_archive_size = self.builder.target_archive_size;
            let manifests = self.builder.build();
            let mut path_counts: BTreeMap<&str, usize> = BTreeMap::new();
            for path in manifests.iter().flat_map(ManifestEntries::file_paths) {
                *path_counts.entry(path.as_ref()).or_default() += 1;
            }
            for path in self.path_to_size.keys() {
                assert_eq!(path_counts.remove(path.as_str()), Some(1));
            }
            assert_eq!(path_counts, BTreeMap::new());

            for manifest in &manifests {
                let paths = manifest.file_paths();
                let num_entries = paths.len();
                assert!((1..=MAX_PARTITION_ENTRIES).contains(&num_entries));
                let Some((_, preceding_paths)) = paths.split_last() else {
                    continue;
                };
                if target_archive_size == 0 {
                    assert_eq!(preceding_paths, &[]);
                } else {
                    let preceding_size: u128 = preceding_paths
                        .iter()
                        .map(|path| {
                            let size = self
                                .path_to_size
                                .get(path.as_ref())
                                .expect("every path was added");
                            u128::from(estimate_uncompressed_size(path.as_ref(), *size))
                        })
                        .sum();
                    assert!(preceding_size < u128::from(target_archive_size));
                }
            }
            manifests
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
        /// Adds a local file and records its expected size.
        ///
        /// # Panics
        ///
        /// Panics if a file path was already added or the source rejects the entry.
        fn add(&mut self, entry: FsEntry) {
            assert_eq!(
                self.path_to_size
                    .insert(entry.path.as_ref().to_owned(), entry.size),
                None
            );
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
    fn fs_manifests_bound_zero_size_files() {
        let count = MAX_PARTITION_ENTRIES + 17;
        let mut fixture = Fixture::new(FsSource, 1024);
        for index in 0..count {
            fixture.add(FsEntry {
                path: LogsInputPath::try_from(format!("/file/{index}"))
                    .expect("test path should be valid"),
                size: 0,
            });
        }
        let manifests = fixture.assert_partitions();
        assert_eq!(manifests.len(), count.div_ceil(MAX_PARTITION_ENTRIES));
    }

    #[test]
    fn fs_no_inputs_produce_no_manifests() {
        let fixture = Fixture::new(FsSource, 1024);
        assert_eq!(fixture.assert_partitions(), vec![]);
    }

    #[test]
    fn fs_extreme_sizes_partition_without_overflow_or_lost_inputs() {
        let mut zero_target_fixture = Fixture::new(FsSource, 0);
        zero_target_fixture.add(FsEntry {
            path: LogsInputPath::try_from("/zero".to_owned()).expect("test path should be valid"),
            size: 0,
        });
        zero_target_fixture.assert_partitions();

        let mut fixture = Fixture::new(FsSource, u64::MAX);
        let files = [
            ("/logs/a.gz", u64::MAX),
            ("/logs/b.zst", u64::MAX),
            ("/logs/c", 7),
        ];
        for (path, size) in files {
            fixture.add(FsEntry {
                path: LogsInputPath::try_from(path.to_owned()).expect("test path should be valid"),
                size,
            });
        }
        let manifests = fixture.assert_partitions();
        let paths: Vec<_> = manifests
            .iter()
            .flat_map(ManifestEntries::file_paths)
            .map(AsRef::as_ref)
            .collect();
        assert_eq!(paths, files.map(|(path, _)| path));
        assert_eq!(manifests.len(), files.len());
        assert_eq!(estimate_uncompressed_size("input.gz", u64::MAX), u64::MAX);
        assert_eq!(estimate_uncompressed_size("input.zst", u64::MAX), u64::MAX);

        let mut fixture = Fixture::new(FsSource, u64::MAX - 1);
        for path in ["/logs/a", "/logs/b", "/logs/c"] {
            fixture.add(FsEntry {
                path: LogsInputPath::try_from(path.to_owned()).expect("test path should be valid"),
                size: u64::MAX / 2,
            });
        }
        let manifests = fixture.assert_partitions();
        assert_eq!(
            manifests
                .iter()
                .map(|manifest| manifest.file_paths.len())
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

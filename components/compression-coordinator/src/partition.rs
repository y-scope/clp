//! Partitioning of S3 objects into compression-task inputs.

use std::{collections::VecDeque, path::Path};

use clp_rust_utils::{
    clp_config::S3Config,
    job_config::{ClpIoConfig, InputConfig},
    s3::ObjectMetadata,
    task_io::compression::S3InputSource,
};

use crate::Error;

/// Builds inputs for compression tasks.
///
/// The builder buffers input objects internally and groups them into partitions. Each partition
/// contains the inputs for a single compression task.
pub struct CompressionInputBuilder {
    buffer: Vec<FileMetadata>,
    partitioned_task_inputs: Vec<S3InputSource>,
    total_buffered_size: u64,
    target_archive_size: u64,
    buffer_size_to_trigger_partition: u64,
    s3_config: S3Config,
}

impl CompressionInputBuilder {
    /// Factory function.
    ///
    /// # Returns
    ///
    /// A newly created [`CompressionInputBuilder`] with an empty buffer.
    #[must_use]
    pub fn new(clp_io_config: &ClpIoConfig) -> Self {
        let target_archive_size = clp_io_config.output.target_archive_size;
        let s3_config = match &clp_io_config.input {
            InputConfig::S3InputConfig { config } => config.s3_config.clone(),
            InputConfig::S3ObjectMetadataInputConfig { config } => config.s3_config.clone(),
        };

        Self::from_s3_config(s3_config, target_archive_size)
    }

    /// Creates an empty builder from the S3 input settings and target archive size.
    ///
    /// # Returns
    ///
    /// A newly created [`CompressionInputBuilder`] with an empty buffer.
    #[must_use]
    pub(crate) fn from_s3_config(s3_config: S3Config, target_archive_size: u64) -> Self {
        Self {
            buffer: Vec::new(),
            partitioned_task_inputs: Vec::new(),
            total_buffered_size: 0,
            target_archive_size,
            buffer_size_to_trigger_partition: target_archive_size * 2,
            s3_config,
        }
    }

    /// Adds an S3 object to the buffer.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`Error::S3BucketMismatch`] if `metadata`'s bucket differs from the buffer's configured
    ///   bucket.
    /// * [`Error::S3KeyPrefixMismatch`] if `metadata`'s key doesn't start with the buffer's
    ///   configured key prefix.
    pub fn add(&mut self, metadata: ObjectMetadata) -> Result<(), Error> {
        if metadata.bucket != self.s3_config.bucket {
            return Err(Error::S3BucketMismatch(
                String::from(self.s3_config.bucket.clone()),
                String::from(metadata.bucket),
            ));
        }

        if !metadata
            .key
            .as_str()
            .starts_with(self.s3_config.key_prefix.as_str())
        {
            return Err(Error::S3KeyPrefixMismatch(
                String::from(self.s3_config.key_prefix.clone()),
                String::from(metadata.key),
            ));
        }

        let file = FileMetadata::new(String::from(metadata.key), metadata.size);
        self.total_buffered_size += file.estimated_size;
        self.buffer.push(file);
        if self.total_buffered_size >= self.buffer_size_to_trigger_partition {
            self.partition(false);
        }

        Ok(())
    }

    /// Flushes any remaining buffered objects and consumes the builder, returning the completed
    /// compression task inputs.
    ///
    /// # Returns
    ///
    /// A vector of input sources, where each source contains the inputs for a single compression
    /// task.
    #[must_use]
    pub fn into_task_input_sources(mut self) -> Vec<S3InputSource> {
        self.flush();
        self.partitioned_task_inputs
    }

    /// Partitions all the objects in the current buffer.
    fn flush(&mut self) {
        self.partition(true);
    }

    /// Converts a non-empty partition into a compression task input and pushes it to the underlying
    /// task inputs buffer.
    fn push_input_source(&mut self, object_keys: Vec<String>) {
        if object_keys.is_empty() {
            return;
        }

        self.partitioned_task_inputs.push(S3InputSource {
            endpoint_url: self.s3_config.endpoint_url.clone(),
            region_code: self.s3_config.region_code.clone(),
            bucket: self.s3_config.bucket.clone(),
            aws_authentication: self.s3_config.aws_authentication.clone(),
            object_keys,
        });
    }

    /// Partitions buffered objects into target-sized compression task inputs.
    ///
    /// When `flush_buffer` is false, objects that cannot fill another target-sized partition
    /// remain buffered. Otherwise, the final partial partition is also emitted.
    fn partition(&mut self, flush_buffer: bool) {
        if !flush_buffer && self.total_buffered_size < self.target_archive_size {
            return;
        }
        if self.buffer.is_empty() {
            return;
        }

        let mut rr_iterator = RoundRobinIterator::new(std::mem::take(&mut self.buffer));

        'partitioning: while flush_buffer || self.total_buffered_size >= self.target_archive_size {
            let mut partition = Vec::new();
            let mut partition_size = 0;

            while partition_size < self.target_archive_size {
                let Some(file) = rr_iterator.next() else {
                    self.push_input_source(partition);
                    break 'partitioning;
                };

                partition_size += file.estimated_size;
                self.total_buffered_size -= file.estimated_size;
                partition.push(file.path);
            }

            self.push_input_source(partition);
        }

        self.buffer = rr_iterator.into_flatten();
    }
}

/// An S3 object's key and its estimated uncompressed size.
struct FileMetadata {
    path: String,
    estimated_size: u64,
}

impl FileMetadata {
    /// Creates metadata for the object identified by `path`, estimating its uncompressed size from
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
            .filter(|group| !group.is_empty())
            .map(IntoIterator::into_iter)
            .collect();

        Self { groups }
    }

    /// Consumes the iterator and collects every object that has not yet been returned.
    ///
    /// # Returns
    ///
    /// The remaining objects, flattened according to the iterator's current group order.
    fn into_flatten(self) -> Vec<FileMetadata> {
        self.groups.into_iter().flatten().collect()
    }
}

impl Iterator for RoundRobinIterator {
    type Item = FileMetadata;

    fn next(&mut self) -> Option<Self::Item> {
        let mut group = self.groups.pop_front()?;
        let file = group.next()?;
        if !group.as_slice().is_empty() {
            self.groups.push_back(group);
        }
        Some(file)
    }
}

/// Estimates an S3 object's uncompressed size from its compressor suffix.
///
/// # Returns
///
/// The estimated uncompressed size.
fn estimate_uncompressed_size(key: &str, size: u64) -> u64 {
    const GZIP_COMPRESSION_RATIO_ESTIMATE: u64 = 13;
    const GZIP_SUFFIXES: &[&str] = &[".gz", ".gzip", ".tgz", ".tar.gz"];
    const ZSTD_COMPRESSION_RATIO_ESTIMATE: u64 = 8;
    const ZSTD_SUFFIXES: &[&str] = &[".zstd", ".zstandard", ".tar.zstd", ".tar.zstandard"];

    if GZIP_SUFFIXES.iter().any(|suffix| key.ends_with(suffix)) {
        size * GZIP_COMPRESSION_RATIO_ESTIMATE
    } else if ZSTD_SUFFIXES.iter().any(|suffix| key.ends_with(suffix)) {
        size * ZSTD_COMPRESSION_RATIO_ESTIMATE
    } else {
        size
    }
}

/// Gets the filename portion of an S3 object's key.
///
/// # Returns
///
/// The key's filename, or the complete key if it has no filename component.
fn filename(key: &str) -> &str {
    let Some(file_name) = Path::new(key)
        .file_name()
        .and_then(|file_name| file_name.to_str())
    else {
        return key;
    };
    file_name
}

/// Groups S3 objects by filename similarity.
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

    use clp_rust_utils::{
        clp_config::AwsAuthentication,
        job_config::{OutputConfig, S3InputConfig},
    };
    use non_empty_string::NonEmptyString;

    use super::*;

    const TEST_BUCKET: &str = "test-bucket";

    fn create_non_empty_string(value: &str) -> NonEmptyString {
        NonEmptyString::try_from(value).expect("test string literals are non-empty")
    }

    fn create_builder(target_archive_size: u64) -> CompressionInputBuilder {
        let s3_config = S3Config {
            bucket: create_non_empty_string(TEST_BUCKET),
            region_code: None,
            key_prefix: create_non_empty_string("logs/"),
            endpoint_url: None,
            aws_authentication: AwsAuthentication::Default,
        };
        let clp_io_config = ClpIoConfig {
            input: InputConfig::S3InputConfig {
                config: S3InputConfig {
                    s3_config,
                    keys: None,
                    dataset: None,
                    timestamp_key: None,
                    unstructured: false,
                },
            },
            output: OutputConfig {
                target_archive_size,
                target_dictionaries_size: 1024,
                target_encoded_file_size: 1024,
                target_segment_size: 1024,
                compression_level: 3,
            },
        };

        CompressionInputBuilder::new(&clp_io_config)
    }

    fn add_object(
        builder: &mut CompressionInputBuilder,
        key_to_size: &mut BTreeMap<String, u64>,
        key: &str,
        size: u64,
    ) {
        builder
            .add(ObjectMetadata {
                bucket: create_non_empty_string(TEST_BUCKET),
                key: create_non_empty_string(key),
                size,
            })
            .expect("object's bucket matches the builder's configured bucket");
        key_to_size.insert(String::from(key), size);
    }

    fn assert_partition_invariants(
        input_sources: &[S3InputSource],
        key_to_size: &BTreeMap<String, u64>,
        target_archive_size: u64,
    ) {
        let mut key_counts: BTreeMap<&str, usize> = BTreeMap::new();
        for key in input_sources.iter().flat_map(|source| &source.object_keys) {
            *key_counts.entry(key.as_str()).or_default() += 1;
        }
        for key in key_to_size.keys() {
            assert_eq!(key_counts.remove(key.as_str()), Some(1));
        }
        assert!(key_counts.is_empty());

        for source in input_sources {
            let total_size: u64 = source
                .object_keys
                .iter()
                .map(|key| key_to_size.get(key).expect("every key was added"))
                .sum();
            let last_size = source
                .object_keys
                .last()
                .map(|key| key_to_size.get(key).expect("every key was added"))
                .expect("`push_input_source` never pushes an empty partition");

            // Files are appended only while the partition is still under the target, so removing
            // the last-appended file must drop the partition back below the target.
            assert!(total_size - last_size < target_archive_size);
        }
    }

    #[test]
    fn test_partition_on_flush() {
        const TARGET_ARCHIVE_SIZE: u64 = 1000;
        const FILE_SIZE: u64 = 100;
        const NUM_FILES: u64 = 15;

        let mut builder = create_builder(TARGET_ARCHIVE_SIZE);
        let mut key_to_size = BTreeMap::new();
        for i in 0..NUM_FILES {
            add_object(
                &mut builder,
                &mut key_to_size,
                &format!("logs/app.log.{i}"),
                FILE_SIZE,
            );
        }

        assert!(builder.partitioned_task_inputs.is_empty());

        let input_sources = builder.into_task_input_sources();
        assert!(!input_sources.is_empty());
        assert_partition_invariants(&input_sources, &key_to_size, TARGET_ARCHIVE_SIZE);
    }

    #[test]
    fn test_partition_on_the_fly() {
        const TARGET_ARCHIVE_SIZE: u64 = 1000;
        const FILE_STEMS: &[&str] = &["app.log", "database.log", "kernel.log", "nginx.log"];
        const NUM_FILES_PER_STEM: u64 = 60;

        let mut builder = create_builder(TARGET_ARCHIVE_SIZE);
        let mut key_to_size = BTreeMap::new();
        for stem in FILE_STEMS {
            for i in 0..NUM_FILES_PER_STEM {
                add_object(
                    &mut builder,
                    &mut key_to_size,
                    &format!("logs/{stem}.{i}"),
                    10 + (i % 7) * 30,
                );
            }
        }

        assert!(!builder.partitioned_task_inputs.is_empty());

        let input_sources = builder.into_task_input_sources();
        assert_partition_invariants(&input_sources, &key_to_size, TARGET_ARCHIVE_SIZE);
    }

    #[test]
    fn test_round_robin_iterator() {
        let files = ["app.log", "app.log.1", "app.log.2"]
            .into_iter()
            .chain(["nginx.log", "nginx.log.1", "nginx.log.2"])
            .map(|key| FileMetadata::new(String::from(key), 1))
            .collect();

        let mut rr_iterator = RoundRobinIterator::new(files);
        let yielded: Vec<String> = rr_iterator.by_ref().take(3).map(|file| file.path).collect();
        assert_eq!(yielded, vec!["app.log", "nginx.log", "app.log.1"]);

        let mut remaining: Vec<String> = rr_iterator
            .into_flatten()
            .into_iter()
            .map(|file| file.path)
            .collect();
        remaining.sort();
        assert_eq!(remaining, vec!["app.log.2", "nginx.log.1", "nginx.log.2"]);
    }
}

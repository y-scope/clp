//! Partitioning of S3 objects into compression-task inputs.

use std::{collections::VecDeque, path::Path};

use clp_rust_utils::{
    clp_config::S3Config,
    job_config::{ClpIoConfig, InputConfig},
    s3::ObjectMetadata,
    task_io::compression::S3InputSource,
};

/// Buffers S3 object metadata and partitions the objects into compression-task inputs.
pub struct PathsToCompressBuffer {
    files: Vec<ObjectMetadata>,
    tasks_input_sources: Vec<S3InputSource>,
    total_file_size: u64,
    target_archive_size: u64,
    file_size_to_trigger_compression: u64,
    s3_config: S3Config,
}

impl PathsToCompressBuffer {
    /// Creates an empty buffer using the compression target and S3 settings in `clp_io_config`.
    ///
    /// # Returns
    ///
    /// A new empty [`PathsToCompressBuffer`].
    #[must_use]
    pub fn new(clp_io_config: &ClpIoConfig) -> Self {
        let target_archive_size = clp_io_config.output.target_archive_size;
        let s3_config = match &clp_io_config.input {
            InputConfig::S3InputConfig { config } => config.s3_config.clone(),
            InputConfig::S3ObjectMetadataInputConfig { config } => config.s3_config.clone(),
        };

        Self {
            files: Vec::new(),
            tasks_input_sources: Vec::new(),
            total_file_size: 0,
            target_archive_size,
            file_size_to_trigger_compression: target_archive_size * 2,
            s3_config,
        }
    }

    /// Adds an S3 object to the buffer and partitions buffered objects when the trigger size is
    /// reached.
    pub fn add_file(&mut self, metadata: ObjectMetadata) {
        self.total_file_size += estimate_uncompressed_size(&metadata);
        self.files.push(metadata);
        if self.total_file_size >= self.file_size_to_trigger_compression {
            self.partition_and_compress(false);
        }
    }

    /// Consumes the buffer and returns the compression-task inputs created from completed
    /// partitions.
    ///
    /// # Returns
    ///
    /// One [`S3InputSource`] per completed partition.
    #[must_use]
    pub fn get_tasks_input_sources(self) -> Vec<S3InputSource> {
        self.tasks_input_sources
    }

    /// Partitions every buffered object, including a final partition smaller than the target size.
    pub fn flush(&mut self) {
        self.partition_and_compress(true);
    }

    /// Converts a non-empty partition into a compression-task input and stores it for later
    /// submission.
    fn submit_partition_for_compression(&mut self, files: Vec<ObjectMetadata>) {
        if files.is_empty() {
            return;
        }

        let object_keys = files
            .into_iter()
            .map(|metadata| String::from(metadata.key))
            .collect();

        self.tasks_input_sources.push(S3InputSource {
            endpoint_url: self.s3_config.endpoint_url.clone(),
            region_code: self.s3_config.region_code.clone(),
            bucket: self.s3_config.bucket.clone(),
            aws_authentication: self.s3_config.aws_authentication.clone(),
            object_keys,
        });
    }

    /// Partitions buffered objects into target-sized compression-task inputs.
    ///
    /// When `flush_buffer` is false, objects that cannot fill another target-sized partition
    /// remain buffered. Otherwise, the final partial partition is also emitted.
    fn partition_and_compress(&mut self, flush_buffer: bool) {
        if !flush_buffer && self.total_file_size < self.target_archive_size {
            return;
        }
        if self.files.is_empty() {
            return;
        }

        let mut round_robin_partition = RoundRobinPartition::new(std::mem::take(&mut self.files));

        let mut has_more_files = true;
        while has_more_files && (flush_buffer || self.total_file_size >= self.target_archive_size) {
            let mut partition = Vec::new();
            let mut partition_size = 0;

            while partition_size < self.target_archive_size {
                let Some(file) = round_robin_partition.next() else {
                    has_more_files = false;
                    break;
                };

                partition_size += estimate_uncompressed_size(&file);
                partition.push(file);
            }

            self.submit_partition_for_compression(partition);
            self.total_file_size -= partition_size;
        }

        self.files = round_robin_partition.into_remaining_files();
    }
}

/// Owns filename-similarity groups and returns one object from each group in round-robin order.
struct RoundRobinPartition {
    groups: VecDeque<std::vec::IntoIter<ObjectMetadata>>,
}

impl RoundRobinPartition {
    /// Groups objects by filename similarity and creates a round-robin iterator over the groups.
    ///
    /// # Returns
    ///
    /// A new [`RoundRobinPartition`].
    fn new(files: Vec<ObjectMetadata>) -> Self {
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
    fn into_remaining_files(self) -> Vec<ObjectMetadata> {
        self.groups.into_iter().flatten().collect()
    }
}

impl Iterator for RoundRobinPartition {
    type Item = ObjectMetadata;

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
fn estimate_uncompressed_size(metadata: &ObjectMetadata) -> u64 {
    const GZIP_COMPRESSION_RATIO_ESTIMATE: u64 = 13;
    const GZIP_SUFFIXES: &[&str] = &[".gz", ".gzip", ".tgz", ".tar.gz"];
    const ZSTD_COMPRESSION_RATIO_ESTIMATE: u64 = 8;
    const ZSTD_SUFFIXES: &[&str] = &[".zstd", ".zstandard", ".tar.zstd", ".tar.zstandard"];

    let key = metadata.key.as_str();
    if GZIP_SUFFIXES.iter().any(|suffix| key.ends_with(suffix)) {
        metadata.size * GZIP_COMPRESSION_RATIO_ESTIMATE
    } else if ZSTD_SUFFIXES.iter().any(|suffix| key.ends_with(suffix)) {
        metadata.size * ZSTD_COMPRESSION_RATIO_ESTIMATE
    } else {
        metadata.size
    }
}

/// Gets the filename portion of an S3 object's key.
///
/// # Returns
///
/// The key's filename, or the complete key if it has no filename component.
fn filename(metadata: &ObjectMetadata) -> &str {
    let key = metadata.key.as_str();
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
/// Places files with sufficiently similar filenames into the same group based
/// on their normalized Levenshtein similarity.
///
/// # Returns
///
/// A vector of filename-similarity groups, or an empty vector if files is
/// empty.
fn group_files_by_similar_filenames(mut files: Vec<ObjectMetadata>) -> Vec<Vec<ObjectMetadata>> {
    const FILE_GROUPING_MIN_LEVENSHTEIN_RATIO: f64 = 0.6;

    files.sort_by(|a, b| filename(a).cmp(filename(b)));

    let mut files = files.into_iter();
    let Some(first_file) = files.next() else {
        return Vec::new();
    };

    let mut previous_filename = filename(&first_file).to_owned();
    let mut current_group = vec![first_file];
    let mut groups = Vec::new();

    for file in files {
        let current_filename = filename(&file).to_owned();
        if strsim::normalized_levenshtein(&previous_filename, &current_filename)
            < FILE_GROUPING_MIN_LEVENSHTEIN_RATIO
        {
            groups.push(current_group);
            current_group = Vec::new();
        }
        current_group.push(file);
        previous_filename = current_filename;
    }
    groups.push(current_group);

    groups
}

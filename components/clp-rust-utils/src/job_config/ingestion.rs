pub mod s3 {
    use non_empty_string::NonEmptyString;
    use serde::{Deserialize, Serialize};
    use thiserror::Error;
    use utoipa::ToSchema;

    #[derive(Error, Debug)]
    pub enum ConfigError {
        #[error("Invalid `num_concurrent_listener_tasks`: {0}")]
        InvalidNumConcurrentListenerTasks(u16),

        #[error("Invalid `wait_time_sec`: {0}")]
        InvalidSqsWaitTime(u16),
    }

    /// Enum for all supported types of S3 ingestion job configs.
    #[derive(Clone, Debug, Serialize, Deserialize)]
    pub enum S3IngestionJobConfig {
        SqsListener(SqsListenerConfig),
        S3Scanner(S3ScannerConfig),
    }

    impl S3IngestionJobConfig {
        #[must_use]
        pub const fn as_base_config(&self) -> &BaseConfig {
            match self {
                Self::SqsListener(config) => &config.base,
                Self::S3Scanner(config) => &config.base,
            }
        }

        #[must_use]
        pub const fn as_buffer_config(&self) -> &BufferConfig {
            match self {
                Self::SqsListener(config) => &config.buffer_config,
                Self::S3Scanner(config) => &config.buffer_config,
            }
        }
    }

    /// Base configuration for ingesting logs from S3.
    #[derive(Clone, Debug, Serialize, Deserialize, ToSchema)]
    pub struct BaseConfig {
        /// The S3 bucket to ingest from.
        #[schema(value_type = String, min_length = 1)]
        pub bucket_name: NonEmptyString,

        /// The S3 key prefix to ingest from.
        #[schema(value_type = String, min_length = 1)]
        pub key_prefix: NonEmptyString,

        /// AWS service region. Must be provided if using the default AWS S3 endpoint.
        #[serde(default)]
        #[schema(value_type = String, min_length = 1)]
        pub region: Option<NonEmptyString>,

        /// The endpoint URL for custom S3-compatible object stores (e.g., `MinIO`, `LocalStack`).
        /// Use the default AWS S3 endpoint if not provided.
        #[serde(default)]
        #[schema(value_type = String, min_length = 1)]
        pub endpoint_url: Option<NonEmptyString>,

        /// The dataset to ingest into. Defaults to `None` (which uses the default dataset).
        #[serde(default)]
        #[schema(value_type = String, min_length = 1)]
        pub dataset: Option<NonEmptyString>,

        /// The optional key for extracting timestamps from object metadata. Defaults to `None`.
        #[serde(default)]
        #[schema(value_type = String, min_length = 1)]
        pub timestamp_key: Option<NonEmptyString>,

        /// Whether to treat the ingested objects as unstructured logs. Defaults to `false`.
        #[serde(default = "default_unstructured")]
        pub unstructured: bool,
    }

    /// Configuration for a SQS listener job.
    #[derive(Debug, Clone, Serialize, Deserialize, ToSchema)]
    pub struct SqsListenerConfig {
        #[serde(flatten)]
        pub base: BaseConfig,

        /// Per-job ingestion buffer config.
        #[serde(default)]
        pub buffer_config: BufferConfig,

        /// The SQS queue URL to poll for S3 event notifications. The given queue must be dedicated
        /// to this ingestion job.
        #[schema(value_type = String, min_length = 1)]
        pub queue_url: NonEmptyString,

        /// The number of concurrent listener tasks to run.
        ///
        /// Each listener task is an asynchronous coroutine that continuously polls the configured
        /// SQS queue and processes incoming S3 event notifications.
        ///
        /// Defaults to `1`.
        #[serde(default = "default_num_concurrent_listener_tasks")]
        #[schema(minimum = 1, maximum = 32)]
        pub num_concurrent_listener_tasks: u16,

        /// The long polling wait time, in seconds, for receiving messages from the SQS queue.
        ///
        /// This value controls how long an SQS `ReceiveMessage` call will wait for a message to
        /// arrive before returning a response. Enabling long polling can reduce empty responses
        /// and lower overall request costs.
        ///
        /// AWS SQS enforces a maximum wait time of 20 seconds. Any configured value greater than
        /// 20 seconds will be considered invalid.
        ///
        /// Defaults to `20`.
        #[serde(default = "default_sqs_wait_time_sec")]
        #[schema(minimum = 0, maximum = 20)]
        pub wait_time_sec: u16,
    }

    /// Wrapper around [`SqsListenerConfig`] that guarantees the configuration is valid.
    #[derive(Debug, Clone)]
    pub struct ValidatedSqsListenerConfig {
        config: SqsListenerConfig,
    }

    impl ValidatedSqsListenerConfig {
        /// Validates and creates a new instance of [`ValidatedSqsListenerConfig`] from the given
        /// [`SqsListenerConfig`].
        ///
        /// # Returns
        ///
        /// The validated config wrapper on success.
        ///
        /// # Errors
        ///
        /// Returns an error if:
        ///
        /// * [`ConfigError::InvalidNumConcurrentListenerTasks`] if
        ///   `config.num_concurrent_listener_tasks` is invalid.
        /// * [`ConfigError::InvalidSqsWaitTime`] if `config.wait_time_sec` is invalid.
        pub fn validate_and_create(config: SqsListenerConfig) -> Result<Self, ConfigError> {
            if config.num_concurrent_listener_tasks < 1 || config.num_concurrent_listener_tasks > 32
            {
                return Err(ConfigError::InvalidNumConcurrentListenerTasks(
                    config.num_concurrent_listener_tasks,
                ));
            }
            if config.wait_time_sec > 20 {
                return Err(ConfigError::InvalidSqsWaitTime(config.wait_time_sec));
            }
            Ok(Self { config })
        }

        #[must_use]
        pub const fn get(&self) -> &SqsListenerConfig {
            &self.config
        }
    }

    /// Configuration for a S3 scanner job.
    #[derive(Debug, Clone, Serialize, Deserialize, ToSchema)]
    pub struct S3ScannerConfig {
        #[serde(flatten)]
        pub base: BaseConfig,

        /// Per-job ingestion buffer config.
        #[serde(default)]
        pub buffer_config: BufferConfig,

        /// The scan interval in seconds. Defaults to 30 seconds.
        #[serde(default = "default_scanning_interval_sec")]
        pub scanning_interval_sec: u32,

        /// The key to ingest after. If specified, only objects with keys lexicographically greater
        /// than this value will be ingested. Defaults to `None`.
        #[serde(default)]
        #[schema(value_type = String, min_length = 1)]
        pub start_after: Option<NonEmptyString>,
    }

    /// Configuration for a S3 prefix ingestion job.
    #[derive(Debug, Clone, Serialize, Deserialize, ToSchema)]
    pub struct S3PrefixConfig {
        #[serde(flatten)]
        pub base: BaseConfig,
    }

    /// Configuration for buffer behavior.
    #[derive(Debug, Clone, Serialize, Deserialize, ToSchema)]
    #[serde(default)]
    pub struct BufferConfig {
        /// Size-based flush threshold in bytes.
        ///
        /// The buffer is flushed to create a compression job once the total size of buffered
        /// objects exceeds this threshold.
        ///
        /// Defaults to 4 GiB.
        pub flush_threshold_bytes: u64,

        /// Time-based flush threshold in seconds.
        ///
        /// This is a hard timeout. The buffer is flushed to create a compression job when the
        /// oldest buffered object has remained in the buffer for at least this duration,
        /// regardless of the total buffered size. The timer is not reset by newly ingested
        /// objects.
        ///
        /// Defaults to 300 seconds (5 minutes).
        pub timeout_sec: u64,

        /// Capacity of the internal buffer channel.
        ///
        /// Defines the maximum number of objects that can be queued before being processed by the
        /// buffer. Increasing this value may improve throughput at the cost of higher memory
        /// usage.
        ///
        /// Defaults to 16. Must be a positive integer. The behavior for values less than 1 is
        /// undefined.
        #[schema(minimum = 1)]
        pub channel_capacity: usize,
    }

    impl Default for BufferConfig {
        fn default() -> Self {
            Self {
                flush_threshold_bytes: 4 * 1024 * 1024 * 1024,
                timeout_sec: 300,
                channel_capacity: 16,
            }
        }
    }

    const fn default_unstructured() -> bool {
        false
    }

    const fn default_scanning_interval_sec() -> u32 {
        30
    }

    const fn default_num_concurrent_listener_tasks() -> u16 {
        1
    }

    const fn default_sqs_wait_time_sec() -> u16 {
        20
    }
}

/// Represents the unique identifier for an ingestion job in CLP DB.
pub type JobId = u64;

#[cfg(test)]
mod tests {
    use super::s3::BufferConfig;

    #[test]
    fn test_buffer_config_partial_override() -> Result<(), serde_json::Error> {
        let config: BufferConfig = serde_json::from_str(r#"{"timeout_sec": 60}"#)?;
        let default_config = BufferConfig::default();
        assert_eq!(config.timeout_sec, 60);
        assert_eq!(
            config.flush_threshold_bytes,
            default_config.flush_threshold_bytes
        );
        assert_eq!(config.channel_capacity, default_config.channel_capacity);
        Ok(())
    }

    #[test]
    fn test_buffer_config_empty_object_uses_defaults() -> Result<(), serde_json::Error> {
        let config: BufferConfig = serde_json::from_str("{}")?;
        let default_config = BufferConfig::default();
        assert_eq!(
            config.flush_threshold_bytes,
            default_config.flush_threshold_bytes
        );
        assert_eq!(config.timeout_sec, default_config.timeout_sec);
        assert_eq!(config.channel_capacity, default_config.channel_capacity);
        Ok(())
    }
}

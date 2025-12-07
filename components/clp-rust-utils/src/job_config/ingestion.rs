pub mod s3 {
    use serde::{Deserialize, Serialize};

    /// Base configuration for ingesting logs from S3.
    #[derive(Clone, Debug, Serialize, Deserialize)]
    pub struct BaseConfig {
        /// AWS service region.
        pub region: String,

        /// The S3 bucket to ingest from.
        pub bucket_name: String,

        /// The S3 key prefix to ingest from.
        pub key_prefix: String,

        /// The dataset to ingest into. Defaults to `None` (which uses the default dataset).
        #[serde(default)]
        pub dataset: Option<String>,

        /// The optional key for extracting timestamps from object metadata. Defaults to `None`.
        #[serde(default)]
        pub timestamp_key: Option<String>,

        /// Whether to treat the ingested objects as unstructured logs. Defaults to `false`.
        #[serde(default = "default_unstructured")]
        pub unstructured: bool,

        /// Tags to apply on the compressed archives. Defaults to `None`.
        #[serde(default)]
        pub tags: Option<Vec<String>>,
    }

    /// Configuration for a SQS listener job.
    #[derive(Debug, Clone, Serialize, Deserialize)]
    pub struct SqsListenerConfig {
        #[serde(flatten)]
        pub base: BaseConfig,

        /// The SQS queue URL to poll for S3 event notifications. The given queue must be dedicated
        /// to this ingestion job.
        pub queue_url: String,
    }

    /// Configuration for a S3 scanner job.
    #[derive(Debug, Clone, Serialize, Deserialize)]
    pub struct S3ScannerConfig {
        #[serde(flatten)]
        pub base: BaseConfig,

        /// The scan interval in seconds. Defaults to 30 seconds.
        #[serde(default = "default_scanning_interval_sec")]
        pub scanning_interval_sec: u32,

        /// The key to ingest after. If specified, only objects with keys lexicographically greater
        /// than this value will be ingested. Defaults to `None`.
        #[serde(default)]
        pub start_after: Option<String>,
    }

    const fn default_unstructured() -> bool {
        false
    }

    const fn default_scanning_interval_sec() -> u32 {
        30
    }
}

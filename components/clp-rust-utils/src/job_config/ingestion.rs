/// Base configuration for ingesting logs from S3.
#[derive(Clone, Debug)]
pub struct S3IngestionBaseConfig {
    /// AWS service region.
    pub region: String,

    /// The S3 bucket to ingest from.
    pub bucket_name: String,

    /// The S3 key prefix to ingest from.
    pub key_prefix: String,

    /// The dataset to ingest into. Using `default` if not specified.
    pub dataset: Option<String>,

    /// The optional key for extracting timestamps from object metadata.
    pub timestamp_key: Option<String>,

    /// Whether to treat the ingested objects as unstructured logs.
    pub unstructured: bool,

    /// Tags to apply on the compressed archives.
    pub tags: Option<Vec<String>>,
}

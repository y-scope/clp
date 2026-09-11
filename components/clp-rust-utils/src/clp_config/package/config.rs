use std::num::NonZeroU32;
use std::num::NonZeroU64;
use std::num::NonZeroUsize;
use std::path::Path;
use std::path::PathBuf;

use non_empty_string::NonEmptyString;
use serde::Deserialize;

use crate::clp_config::AwsAuthentication;
use crate::clp_config::S3Config;
use crate::dataset::resolve_dataset_name;

/// Mirror of `clp_py_utils.clp_config.ClpConfig`.
///
/// # NOTE
///
/// * This type is partially defined: unused fields are omitted and discarded through
///   deserialization.
/// * The default values must be kept in sync with the Python definition.
#[derive(Clone, Debug, Deserialize, Eq, PartialEq)]
#[serde(default)]
pub struct Config {
    pub package: Package,
    pub database: Database,
    pub results_cache: ResultsCache,
    pub api_server: Option<ApiServer>,
    pub log_ingestor: Option<LogIngestor>,
    pub logs_directory: String,
    pub stream_output: StreamOutput,
    pub logs_input: LogsInput,
    pub archive_output: ArchiveOutput,
    pub telemetry: Telemetry,
    pub spider: Option<Spider>,
    pub compression_coordinator: Option<CompressionCoordinator>,
}

impl Default for Config {
    fn default() -> Self {
        Self {
            package: Package::default(),
            database: Database::default(),
            results_cache: ResultsCache::default(),
            api_server: None,
            log_ingestor: None,
            logs_directory: "var/log".to_owned(),
            stream_output: StreamOutput::default(),
            logs_input: LogsInput::Fs {
                config: FsIngestion::default(),
            },
            archive_output: ArchiveOutput::default(),
            telemetry: Telemetry::default(),
            spider: None,
            compression_coordinator: None,
        }
    }
}

/// Configuration for the Spider task executor.
#[derive(Clone, Debug, Deserialize, Eq, PartialEq)]
#[serde(default)]
pub struct SpiderTaskExecutorConfig {
    pub package: Package,
    pub archive_output: ArchiveOutput,
    pub tmp_directory: String,
    pub database: Database,
}

impl SpiderTaskExecutorConfig {
    /// Resolves `tmp_directory` against `clp_home`.
    ///
    /// # Returns
    ///
    /// `tmp_directory` unchanged if it is already absolute; otherwise, it is joined with
    /// `clp_home`.
    #[must_use]
    pub fn abs_tmp_directory(&self, clp_home: &Path) -> PathBuf {
        make_config_path_absolute(clp_home, &self.tmp_directory)
    }

    /// Resolves the archive-output storage's local directory against `clp_home`.
    ///
    /// # Returns
    ///
    /// The storage's local directory (`directory` for `Fs`, `staging_directory` for `S3`) unchanged
    /// if it is already absolute; otherwise, it is joined with `clp_home`.
    #[must_use]
    pub fn abs_archive_output_staging(&self, clp_home: &Path) -> PathBuf {
        let directory = match &self.archive_output.storage {
            ArchiveOutputStorage::Fs { directory } => directory,
            ArchiveOutputStorage::S3 {
                staging_directory, ..
            } => staging_directory,
        };
        make_config_path_absolute(clp_home, directory)
    }
}

impl Default for SpiderTaskExecutorConfig {
    fn default() -> Self {
        Self {
            package: Package::default(),
            database: Database::default(),
            archive_output: ArchiveOutput::default(),
            tmp_directory: "var/tmp".to_owned(),
        }
    }
}

/// Database names for CLP components.
///
/// # NOTE
///
///
/// This struct mirrors all allowed DB names from `clp_py_utils.clp_config.ClpDbNameType`. Instead
/// of storing them in a map, we use a struct to ensure all expected names are always present and
/// reject all unknown fields.
#[derive(Clone, Debug, Deserialize, Eq, PartialEq)]
#[serde(deny_unknown_fields)]
pub struct ClpDbNames {
    pub clp: String,
}

impl Default for ClpDbNames {
    fn default() -> Self {
        Self {
            clp: "clp-db".to_owned(),
        }
    }
}

/// Mirror of `clp_py_utils.clp_config.Database`.
///
/// # NOTE
///
/// * This type is partially defined: unused fields are omitted and discarded through
///   deserialization.
/// * The default values must be kept in sync with the Python definition.
/// * `table_prefix` is a fixed constant mirroring `CLP_METADATA_TABLE_PREFIX` (`"clp_"`) and is
///   excluded from (de)serialization.
#[derive(Clone, Debug, Deserialize, Eq, PartialEq)]
#[serde(default)]
pub struct Database {
    pub host: String,
    pub port: u16,
    pub names: ClpDbNames,

    #[serde(skip)]
    pub table_prefix: String,
}

impl Default for Database {
    fn default() -> Self {
        /// Mirror of `clp_py_utils.clp_config.CLP_METADATA_TABLE_PREFIX`.
        const CLP_METADATA_TABLE_PREFIX: &str = "clp_";
        Self {
            host: "localhost".to_owned(),
            port: 3306,
            names: ClpDbNames::default(),
            table_prefix: CLP_METADATA_TABLE_PREFIX.to_owned(),
        }
    }
}

impl Database {
    /// # Returns
    ///
    /// The archives table name (`<prefix><dataset>_archives`).
    #[must_use]
    pub fn archives_table_name(&self, dataset: Option<&str>) -> String {
        format!(
            "{}{}_archives",
            self.table_prefix,
            resolve_dataset_name(dataset)
        )
    }

    /// # Returns
    ///
    /// The column-metadata table name (`<prefix><dataset>_column_metadata`).
    #[must_use]
    pub fn column_metadata_table_name(&self, dataset: Option<&str>) -> String {
        format!(
            "{}{}_column_metadata",
            self.table_prefix,
            resolve_dataset_name(dataset)
        )
    }

    /// # Returns
    ///
    /// The datasets table name `<prefix>datasets`.
    #[must_use]
    pub fn datasets_table_name(&self) -> String {
        format!("{}datasets", self.table_prefix)
    }
}

#[derive(Clone, Debug, Deserialize, Eq, PartialEq)]
#[serde(default)]
pub struct ApiServer {
    pub host: String,
    pub port: u16,
    pub query_job_polling: QueryJobPollingConfig,
    pub default_max_num_query_results: u32,
}

impl Default for ApiServer {
    fn default() -> Self {
        Self {
            host: "localhost".to_owned(),
            port: 3001,
            query_job_polling: QueryJobPollingConfig::default(),
            default_max_num_query_results: 1000,
        }
    }
}

#[derive(Clone, Debug, Deserialize, Eq, PartialEq)]
#[serde(default)]
pub struct QueryJobPollingConfig {
    #[serde(rename = "initial_backoff")]
    pub initial_backoff_ms: u64,

    #[serde(rename = "max_backoff")]
    pub max_backoff_ms: u64,
}

impl Default for QueryJobPollingConfig {
    fn default() -> Self {
        Self {
            initial_backoff_ms: 100,
            max_backoff_ms: 5000,
        }
    }
}

/// Mirror of `clp_py_utils.clp_config.Package`.
///
/// # NOTE
///
/// * This type is partially defined: unused fields are omitted and discarded through
///   deserialization.
/// * The default values must be kept in sync with the Python definition.
#[derive(Clone, Debug, Deserialize, Eq, PartialEq)]
#[serde(default)]
pub struct Package {
    pub storage_engine: StorageEngine,
}

impl Default for Package {
    fn default() -> Self {
        Self {
            storage_engine: StorageEngine::Clp,
        }
    }
}

/// Mirror of `clp_py_utils.clp_config.StorageEngine`.
#[derive(Clone, Debug, Deserialize, Eq, PartialEq)]
pub enum StorageEngine {
    #[serde(rename = "clp")]
    Clp,
    #[serde(rename = "clp-s")]
    ClpS,
}

/// Mirror of `clp_py_utils.clp_config.ResultsCache`.
///
/// # NOTE
///
/// * This type is partially defined: unused fields are omitted and discarded through
///   deserialization.
/// * The default values must be kept in sync with the Python definition.
#[derive(Clone, Debug, Deserialize, Eq, PartialEq)]
#[serde(default)]
pub struct ResultsCache {
    pub host: String,
    pub port: u16,
    pub db_name: String,
}

impl Default for ResultsCache {
    fn default() -> Self {
        Self {
            host: "localhost".to_owned(),
            port: 27017,
            db_name: "clp-query-results".to_owned(),
        }
    }
}

/// Mirror of `clp_py_utils.clp_config.StreamOutput`.
///
/// # NOTE
///
/// * The default values must be kept in sync with the Python definition.
#[derive(Clone, Default, Debug, Deserialize, Eq, PartialEq)]
#[serde(default)]
pub struct StreamOutput {
    pub storage: StreamOutputStorage,
}

#[derive(Clone, Debug, Deserialize, Eq, PartialEq)]
#[serde(tag = "type")]
pub enum StreamOutputStorage {
    #[serde(rename = "fs")]
    Fs { directory: String },

    #[serde(rename = "s3")]
    S3 {
        staging_directory: String,
        s3_config: S3Config,
    },
}

impl Default for StreamOutputStorage {
    fn default() -> Self {
        Self::Fs {
            directory: "var/data/streams".to_owned(),
        }
    }
}

/// Mirror of `clp_py_utils.clp_config.LogIngestor`.
///
/// # NOTE
///
/// * The default values must be kept in sync with the Python definition.
#[derive(Clone, Debug, Deserialize, Eq, PartialEq)]
#[serde(default)]
pub struct LogIngestor {
    pub host: String,
    pub port: u16,
    pub logging_level: String,
}

impl Default for LogIngestor {
    fn default() -> Self {
        Self {
            host: "localhost".to_owned(),
            port: 3002,
            logging_level: "INFO".to_owned(),
        }
    }
}

/// Mirror of `clp_py_utils.clp_config.ArchiveOutput`.
///
/// # NOTE
///
/// * This type is partially defined: unused fields are omitted and discarded through
///   deserialization.
/// * The default values must be kept in sync with the Python definition.
#[derive(Clone, Debug, Deserialize, Eq, PartialEq)]
#[serde(default)]
pub struct ArchiveOutput {
    pub storage: ArchiveOutputStorage,
    pub target_archive_size: u64,
    pub target_dictionaries_size: u64,
    pub target_encoded_file_size: u64,
    pub target_segment_size: u64,
    pub compression_level: u8,
}

impl ArchiveOutput {
    /// Derives the archive storage directory for a dataset.
    ///
    /// # Returns
    ///
    /// The dataset's storage base (`s3_config.key_prefix` for S3, `directory` for `Fs`) joined with
    /// `dataset`, where a `None` dataset resolves to `default`.
    #[must_use]
    pub fn dataset_archive_storage_directory(&self, dataset: Option<&str>) -> String {
        let base = match &self.storage {
            ArchiveOutputStorage::Fs { directory } => directory.as_str(),
            ArchiveOutputStorage::S3 { s3_config, .. } => s3_config.key_prefix.as_str(),
        };
        Path::new(base)
            .join(resolve_dataset_name(dataset))
            .to_string_lossy()
            .into_owned()
    }

    /// Derives the S3 object key of an archive in a dataset.
    ///
    /// # Returns
    ///
    /// The dataset's archive storage directory joined with `archive_id`, where a `None` dataset
    /// resolves to `default`.
    #[must_use]
    pub fn dataset_archive_object_key(&self, dataset: Option<&str>, archive_id: &str) -> String {
        format!(
            "{}/{archive_id}",
            self.dataset_archive_storage_directory(dataset)
        )
    }
}

impl Default for ArchiveOutput {
    fn default() -> Self {
        Self {
            storage: ArchiveOutputStorage::default(),
            target_archive_size: 256 * 1024 * 1024,
            target_dictionaries_size: 32 * 1024 * 1024,
            target_encoded_file_size: 256 * 1024 * 1024,
            target_segment_size: 256 * 1024 * 1024,
            compression_level: 3,
        }
    }
}

/// Mirror of `clp_py_utils.clp_config.ArchiveFsStorage` | `ArchiveS3Storage`.
#[derive(Clone, Debug, Deserialize, Eq, PartialEq)]
#[serde(tag = "type")]
pub enum ArchiveOutputStorage {
    #[serde(rename = "fs")]
    Fs { directory: String },

    #[serde(rename = "s3")]
    S3 {
        #[serde(default = "default_archive_staging_directory")]
        staging_directory: String,
        s3_config: S3Config,
    },
}

impl Default for ArchiveOutputStorage {
    fn default() -> Self {
        Self::Fs {
            directory: "var/data/archives".to_owned(),
        }
    }
}

/// Mirror of `clp_py_utils.clp_config.S3IngestionConfig`.
#[derive(Clone, Debug, Deserialize, Eq, PartialEq)]
pub struct S3Ingestion {
    pub aws_authentication: AwsAuthentication,
}

/// Mirror of `clp_py_utils.clp_config.FsIngestionConfig`.
///
/// # NOTE
///
/// * The default values must be kept in sync with the Python definition.
#[derive(Clone, Debug, Deserialize, Eq, PartialEq)]
pub struct FsIngestion {
    pub directory: String,
}

impl Default for FsIngestion {
    fn default() -> Self {
        Self {
            directory: "/".to_owned(),
        }
    }
}

/// Mirror of `clp_py_utils.clp_config.ClpConfig.logs_input`.
#[derive(Debug, Clone, PartialEq, Eq, Deserialize)]
#[serde(tag = "type")]
pub enum LogsInput {
    #[serde(rename = "fs")]
    Fs {
        #[serde(flatten)]
        config: FsIngestion,
    },

    #[serde(rename = "s3")]
    S3 {
        #[serde(flatten)]
        config: S3Ingestion,
    },
}

/// Mirror of `clp_py_utils.clp_config.Telemetry`.
#[derive(Clone, Debug, Deserialize, Eq, PartialEq)]
#[serde(default)]
pub struct Telemetry {
    pub disable: bool,
    pub endpoint: String,
}

impl Default for Telemetry {
    fn default() -> Self {
        Self {
            disable: false,
            endpoint: "https://telemetry.yscope.io".to_owned(),
        }
    }
}

/// Compression coordinator configuration.
#[derive(Clone, Debug, Deserialize, Eq, PartialEq)]
#[serde(default)]
pub struct CompressionCoordinator {
    pub resource_group: SpiderResourceGroup,
    pub job_polling_interval_millisecs: NonZeroU64,
    pub max_concurrent_jobs: NonZeroUsize,
    pub result_polling: PollingBackoff,
    pub compression_task_max_retry: u32,
    pub commit_task_max_retry: u32,
    pub database_connection_pool_size: NonZeroU32,
    pub termination_timeout_secs: NonZeroU64,
    pub commit_task_soft_timeout_secs: NonZeroU64,
    pub commit_task_hard_timeout_secs: NonZeroU64,
}

impl Default for CompressionCoordinator {
    fn default() -> Self {
        Self {
            resource_group: SpiderResourceGroup {
                name: NonEmptyString::new("compression-coordinator".to_owned())
                    .expect("default resource group name should not be empty"),
            },
            job_polling_interval_millisecs: NonZeroU64::new(100)
                .expect("default jobs poll delay should not be zero"),
            max_concurrent_jobs: NonZeroUsize::new(1000)
                .expect("default maximum number of concurrent jobs should not be zero"),
            result_polling: PollingBackoff {
                init_backoff_millisecs: NonZeroU64::new(100)
                    .expect("default result polling init backoff should not be zero"),
                max_backoff_millisecs: NonZeroU64::new(1000)
                    .expect("default result polling max backoff should not be zero"),
            },
            compression_task_max_retry: 1,
            commit_task_max_retry: 1,
            database_connection_pool_size: NonZeroU32::new(10)
                .expect("default database connection pool size should not be zero"),
            termination_timeout_secs: NonZeroU64::new(30)
                .expect("default termination timeout should not be zero"),
            commit_task_soft_timeout_secs: NonZeroU64::new(45)
                .expect("default commit task soft timeout should not be zero"),
            commit_task_hard_timeout_secs: NonZeroU64::new(60)
                .expect("default commit task hard timeout should not be zero"),
        }
    }
}

/// Spider configuration.
#[derive(Clone, Debug, Deserialize, Eq, PartialEq)]
pub struct Spider {
    pub host: NonEmptyString,
    pub port: u16,
}

/// Spider resource group configuration.
#[derive(Clone, Debug, Deserialize, Eq, PartialEq)]
pub struct SpiderResourceGroup {
    pub name: NonEmptyString,
}

/// Polling backoff configuration.
#[derive(Clone, Debug, Deserialize, Eq, PartialEq)]
pub struct PollingBackoff {
    pub init_backoff_millisecs: NonZeroU64,
    pub max_backoff_millisecs: NonZeroU64,
}

/// # Returns
///
/// `path` unchanged if it is already absolute, otherwise joined with `root`.
fn make_config_path_absolute(root: &Path, path: &str) -> PathBuf {
    if Path::new(path).is_absolute() {
        PathBuf::from(path)
    } else {
        root.join(path)
    }
}

fn default_archive_staging_directory() -> String {
    "var/data/staged-archives".to_owned()
}

#[cfg(test)]
mod tests {
    use std::path::Path;

    use super::ArchiveOutput;
    use super::ArchiveOutputStorage;
    use super::Database;
    use super::LogsInput;
    use super::SpiderTaskExecutorConfig;

    #[test]
    fn deserialize_logs_input_s3_config() {
        const ACCESS_KEY_ID: &str = "YSCOPE";
        const SECRET_ACCESS_KEY: &str = "IamSecret";
        let logs_input_config_json = serde_json::json!({
            "type": "s3",
            "aws_authentication": {
                "type": "credentials",
                "credentials": {
                    "access_key_id": ACCESS_KEY_ID,
                    "secret_access_key": SECRET_ACCESS_KEY,
                }
            }
        });

        let deserialized =
            serde_json::from_str::<LogsInput>(logs_input_config_json.to_string().as_str())
                .expect("failed to deserialize `LogsInput` from JSON");

        match deserialized {
            LogsInput::S3 { config } => match config.aws_authentication {
                crate::clp_config::AwsAuthentication::Credentials { credentials } => {
                    assert_eq!(credentials.access_key_id, ACCESS_KEY_ID);
                    assert_eq!(credentials.secret_access_key, SECRET_ACCESS_KEY);
                }
                crate::clp_config::AwsAuthentication::Default => {
                    panic!("Expected credentials, got `default`")
                }
            },
            LogsInput::Fs { .. } => panic!("Expected S3"),
        }
    }

    #[test]
    fn deserialize_logs_input_s3_default_config() {
        let logs_input_config_json = serde_json::json!({
            "type": "s3",
            "aws_authentication": {
                "type": "default",
            }
        });

        let deserialized =
            serde_json::from_str::<LogsInput>(logs_input_config_json.to_string().as_str())
                .expect("failed to deserialize `LogsInput` from JSON");

        match deserialized {
            LogsInput::S3 { config } => {
                assert_eq!(
                    config.aws_authentication,
                    crate::clp_config::AwsAuthentication::Default
                );
            }
            LogsInput::Fs { .. } => panic!("Expected S3"),
        }
    }

    #[test]
    fn deserialize_logs_input_fs_config() {
        const DIRECTORY: &str = "/var/logs";

        let logs_input_config_json = serde_json::json!({
            "type": "fs",
            "directory": DIRECTORY,
        });

        let deserialized =
            serde_json::from_str::<LogsInput>(logs_input_config_json.to_string().as_str())
                .expect("failed to deserialize `LogsInput` from JSON");

        match deserialized {
            LogsInput::Fs { config } => {
                assert_eq!(config.directory, DIRECTORY);
            }
            LogsInput::S3 { .. } => panic!("Expected Fs"),
        }
    }

    #[test]
    fn dataset_archive_storage_directory_fs() {
        let archive_output = ArchiveOutput {
            storage: ArchiveOutputStorage::Fs {
                directory: "/var/data/archives".to_owned(),
            },
            ..ArchiveOutput::default()
        };

        assert_eq!(
            archive_output.dataset_archive_storage_directory(Some("mydataset")),
            "/var/data/archives/mydataset"
        );
        assert_eq!(
            archive_output.dataset_archive_storage_directory(None),
            "/var/data/archives/default"
        );
    }

    #[test]
    fn dataset_archive_storage_directory_s3() {
        use non_empty_string::NonEmptyString;

        use crate::clp_config::AwsAuthentication;
        use crate::clp_config::S3Config;

        let archive_output = ArchiveOutput {
            storage: ArchiveOutputStorage::S3 {
                staging_directory: "var/data/staged-archives".to_owned(),
                s3_config: S3Config {
                    bucket: NonEmptyString::try_from("bucket".to_string())
                        .expect("bucket is non-empty"),
                    region_code: None,
                    key_prefix: NonEmptyString::try_from("prefix".to_string())
                        .expect("key prefix is non-empty"),
                    endpoint_url: None,
                    aws_authentication: AwsAuthentication::Default,
                },
            },
            ..ArchiveOutput::default()
        };

        assert_eq!(
            archive_output.dataset_archive_storage_directory(Some("mydataset")),
            "prefix/mydataset"
        );
        assert_eq!(
            archive_output.dataset_archive_storage_directory(None),
            "prefix/default"
        );
    }

    #[test]
    fn dataset_archive_object_key_joins_prefix_dataset_and_id() {
        use non_empty_string::NonEmptyString;

        use crate::clp_config::AwsAuthentication;
        use crate::clp_config::S3Config;
        use crate::types::non_empty_string::ExpectedNonEmpty;

        let archive_output = ArchiveOutput {
            storage: ArchiveOutputStorage::S3 {
                staging_directory: "var/data/staged-archives".to_owned(),
                s3_config: S3Config {
                    bucket: NonEmptyString::from_static_str("bucket"),
                    region_code: None,
                    key_prefix: NonEmptyString::from_static_str("LIB1/"),
                    endpoint_url: None,
                    aws_authentication: AwsAuthentication::Default,
                },
            },
            ..ArchiveOutput::default()
        };

        assert_eq!(
            archive_output.dataset_archive_object_key(None, "abc"),
            "LIB1/default/abc"
        );
        assert_eq!(
            archive_output.dataset_archive_object_key(Some("mydataset"), "abc"),
            "LIB1/mydataset/abc"
        );
    }

    #[test]
    fn deserialize_database_ignores_provided_table_prefix() {
        let database_json = serde_json::json!({
            "host": "h",
            "port": 3306,
            "names": {
                "clp": "clp-db",
            },
            "table_prefix": "custom_"
        });

        let db = serde_json::from_str::<Database>(database_json.to_string().as_str())
            .expect("failed to deserialize `Database` from JSON");

        assert_eq!(db.table_prefix, "clp_");
    }

    #[test]
    fn abs_tmp_directory_joins_relative_path() {
        let config = SpiderTaskExecutorConfig {
            tmp_directory: "var/tmp".to_owned(),
            ..SpiderTaskExecutorConfig::default()
        };

        assert_eq!(
            config.abs_tmp_directory(Path::new("/opt/clp")),
            Path::new("/opt/clp/var/tmp")
        );
    }

    #[test]
    fn abs_tmp_directory_leaves_absolute_path_unchanged() {
        let config = SpiderTaskExecutorConfig {
            tmp_directory: "/abs/tmp".to_owned(),
            ..SpiderTaskExecutorConfig::default()
        };

        assert_eq!(
            config.abs_tmp_directory(Path::new("/opt/clp")),
            Path::new("/abs/tmp")
        );
    }

    #[test]
    fn abs_archive_output_staging_joins_relative_s3_path() {
        let config = s3_config_with_staging_directory("var/staged-archives");

        assert_eq!(
            config.abs_archive_output_staging(Path::new("/opt/clp")),
            Path::new("/opt/clp/var/staged-archives")
        );
    }

    #[test]
    fn abs_archive_output_staging_leaves_absolute_s3_path_unchanged() {
        let config = s3_config_with_staging_directory("/abs/staged-archives");

        assert_eq!(
            config.abs_archive_output_staging(Path::new("/opt/clp")),
            Path::new("/abs/staged-archives")
        );
    }

    /// # Returns
    ///
    /// A [`SpiderTaskExecutorConfig`] whose archive output is S3-backed with `staging_directory`.
    fn s3_config_with_staging_directory(staging_directory: &str) -> SpiderTaskExecutorConfig {
        use non_empty_string::NonEmptyString;

        use crate::clp_config::AwsAuthentication;
        use crate::clp_config::S3Config;

        SpiderTaskExecutorConfig {
            archive_output: ArchiveOutput {
                storage: ArchiveOutputStorage::S3 {
                    staging_directory: staging_directory.to_owned(),
                    s3_config: S3Config {
                        bucket: NonEmptyString::try_from("bucket".to_string())
                            .expect("bucket is non-empty"),
                        region_code: None,
                        key_prefix: NonEmptyString::try_from("prefix/".to_string())
                            .expect("key prefix is non-empty"),
                        endpoint_url: None,
                        aws_authentication: AwsAuthentication::Default,
                    },
                },
                ..ArchiveOutput::default()
            },
            ..SpiderTaskExecutorConfig::default()
        }
    }
}

use serde::Deserialize;

use crate::clp_config::{AwsAuthentication, S3Config};

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
    pub spider: String,
}

impl Default for ClpDbNames {
    fn default() -> Self {
        Self {
            clp: "clp-db".to_owned(),
            spider: "spider-db".to_owned(),
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
#[derive(Clone, Debug, Deserialize, Eq, PartialEq)]
#[serde(default)]
pub struct Database {
    pub host: String,
    pub port: u16,
    pub names: ClpDbNames,
}

impl Default for Database {
    fn default() -> Self {
        Self {
            host: "localhost".to_owned(),
            port: 3306,
            names: ClpDbNames::default(),
        }
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
    #[serde(rename = "buffer_flush_timeout")]
    pub buffer_flush_timeout_sec: u64,
    pub buffer_flush_threshold: u64,
    pub channel_capacity: usize,
    pub logging_level: String,
}

impl Default for LogIngestor {
    fn default() -> Self {
        Self {
            host: "localhost".to_owned(),
            port: 3002,
            buffer_flush_timeout_sec: 300,
            buffer_flush_threshold: 4096 * 1024 * 1024, // 4 GiB
            channel_capacity: 10,
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
    pub target_archive_size: u64,
    pub target_dictionaries_size: u64,
    pub target_encoded_file_size: u64,
    pub target_segment_size: u64,
    pub compression_level: u8,
}

impl Default for ArchiveOutput {
    fn default() -> Self {
        Self {
            target_archive_size: 256 * 1024 * 1024,
            target_dictionaries_size: 32 * 1024 * 1024,
            target_encoded_file_size: 256 * 1024 * 1024,
            target_segment_size: 256 * 1024 * 1024,
            compression_level: 3,
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

#[cfg(test)]
mod tests {
    use super::LogsInput;

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
            },
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
}

use serde::Deserialize;

/// Mirror of `clp_py_utils.clp_config.CLPConfig`.
///
/// # NOTE
///
/// * This type is partially defined: unused fields are omitted and discarded through
///   deserialization.
/// * The default values must be kept in sync with the Python definition.
#[derive(Deserialize, Clone)]
pub struct Config {
    #[serde(default)]
    pub package: Package,
    #[serde(default)]
    pub database: Database,
    #[serde(default)]
    pub results_cache: ResultsCache,
    #[serde(default)]
    pub api_server: ApiServer,
}

/// Mirror of `clp_py_utils.clp_config.Database`.
///
/// # NOTE
///
/// * This type is partially defined: unused fields are omitted and discarded through
///   deserialization.
/// * The default values must be kept in sync with the Python definition.
#[derive(Deserialize, Clone)]
#[serde(default)]
pub struct Database {
    pub host: String,
    pub port: u16,
    pub name: String,
}

impl Default for Database {
    fn default() -> Self {
        Self {
            host: "localhost".to_owned(),
            port: 3306,
            name: "clp-db".to_owned(),
        }
    }
}

#[derive(Deserialize, Clone)]
#[serde(default)]
pub struct ApiServer {
    pub host: String,
    pub port: u16,
}

impl Default for ApiServer {
    fn default() -> Self {
        Self {
            host: "localhost".to_owned(),
            port: 3001,
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
#[derive(Deserialize, Clone)]
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
#[derive(Deserialize, Clone)]
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
#[derive(Deserialize, Clone)]
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

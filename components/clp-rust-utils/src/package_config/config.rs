use serde::Deserialize;

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

#[derive(Deserialize, Clone)]
pub enum StorageEngine {
    #[serde(rename = "clp")]
    Clp,
    #[serde(rename = "clp-s")]
    ClpS,
}

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

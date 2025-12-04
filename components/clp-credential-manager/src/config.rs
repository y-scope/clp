use std::path::Path;

use clp_rust_utils::serde::yaml;
use secrecy::SecretString;
use serde::Deserialize;

use crate::error::ServiceError;

const DEFAULT_MAX_CONNECTIONS: u32 = 5;
const DEFAULT_SERVER_BIND_ADDRESS: &str = "0.0.0.0";
const DEFAULT_SERVER_PORT: u16 = 8080;

/// Root configuration deserialized from `credential-manager-config.yml`.
#[derive(Debug, Clone, Deserialize)]
pub struct CredentialManagerConfig {
    #[serde(default)]
    pub server: ServerConfig,
    pub database: DatabaseConfig,
}

impl CredentialManagerConfig {
    /// Loads configuration from disk and validates YAML syntax.
    ///
    /// # Parameters:
    ///
    /// * `path`: Filesystem location of the YAML configuration file.
    ///
    /// # Returns
    ///
    /// A parsed [`CredentialManagerConfig`] instance when the file can be read and deserialized
    /// successfully.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`yaml::from_path`] surfaces any [`clp_rust_utils::Error`], which maps to
    ///   [`ServiceError::ConfigFile`].
    pub fn from_file(path: &Path) -> Result<Self, ServiceError> {
        let config: Self = yaml::from_path(path)?;
        Ok(config)
    }
}

/// Network settings for the Axum server.
#[derive(Debug, Clone, Deserialize)]
#[serde(default)]
pub struct ServerConfig {
    pub bind_address: String,
    pub port: u16,
}

impl Default for ServerConfig {
    /// Provides sensible defaults that bind the HTTP server to all interfaces on port 8080.
    ///
    /// # Returns
    ///
    /// A [`ServerConfig`] with bind address `0.0.0.0` and port `8080`.
    fn default() -> Self {
        Self {
            bind_address: default_bind_address(),
            port: default_bind_port(),
        }
    }
}

/// Returns the default bind address of `0.0.0.0` so the service listens on every interface.
fn default_bind_address() -> String {
    DEFAULT_SERVER_BIND_ADDRESS.to_owned()
}

/// Returns the default bind port of `8080` that matches other CLP services.
const fn default_bind_port() -> u16 {
    DEFAULT_SERVER_PORT
}

/// Connection options for the `MySQL` backend that stores credentials.
#[derive(Debug, Clone, Deserialize)]
pub struct DatabaseConfig {
    pub host: String,
    #[serde(default = "default_mysql_port")]
    pub port: u16,
    pub name: String,
    pub user: String,
    pub password: SecretString,
    #[serde(default = "default_max_connections")]
    pub max_connections: u32,
}

/// Returns the default `MySQL` server port (`3306`).
const fn default_mysql_port() -> u16 {
    3306
}

/// Returns the default maximum connection count for the `MySQL` pool.
const fn default_max_connections() -> u32 {
    DEFAULT_MAX_CONNECTIONS
}

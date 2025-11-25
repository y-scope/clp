use std::{
    fs,
    net::SocketAddr,
    path::{Path},
};

use secrecy::SecretString;
use serde::Deserialize;

use crate::error::{ServiceError, ServiceResult};

const DEFAULT_MAX_CONNECTIONS: u32 = 5;
const DEFAULT_SERVER_BIND_ADDRESS: &str = "0.0.0.0";
const DEFAULT_SERVER_PORT: u16 = 8080;

/// Root configuration deserialized from `credential-manager-config.yml`.
#[derive(Debug, Clone, Deserialize)]
pub struct AppConfig {
    #[serde(default)]
    pub server: ServerConfig,
    pub database: DatabaseConfig,
}

impl AppConfig {
    /// Loads configuration from disk and validates YAML syntax.
    ///
    /// # Parameters:
    ///
    /// * `path`: Filesystem location of the YAML configuration file.
    ///
    /// # Returns:
    ///
    /// A parsed [`AppConfig`] instance when the file can be read and deserialized successfully.
    ///
    /// # Errors:
    ///
    /// * Returns [`ServiceError::Io`] if the file cannot be read.
    /// * Returns [`ServiceError::Yaml`] if parsing fails.
    pub fn from_file(path: &Path) -> ServiceResult<Self> {
        let contents = fs::read_to_string(path)?;
        let config: Self = serde_yaml::from_str(&contents)?;
        Ok(config)
    }
}

/// Network settings for the Axum server.
#[derive(Debug, Clone, Deserialize)]
pub struct ServerConfig {
    #[serde(default = "default_bind_address")]
    pub bind_address: String,
    #[serde(default = "default_bind_port")]
    pub port: u16,
}

impl ServerConfig {
    /// Renders the configured address pair into a [`SocketAddr`].
    ///
    /// # Returns:
    ///
    /// A [`SocketAddr`] that Axum can bind to when starting the HTTP server.
    ///
    /// # Errors:
    ///
    /// Returns [`ServiceError::Config`] if the address string cannot be parsed.
    pub fn socket_addr(&self) -> ServiceResult<SocketAddr> {
        let addr = format!("{}:{}", self.bind_address, self.port);
        addr.parse().map_err(|err| {
            ServiceError::Config(format!(
                "invalid bind address `{}`:{} ({err})",
                self.bind_address, self.port
            ))
        })
    }
}

impl Default for ServerConfig {
    /// Provides sensible defaults that bind the HTTP server to all interfaces on port 8080.
    fn default() -> Self {
        Self {
            bind_address: default_bind_address(),
            port: default_bind_port(),
        }
    }
}

/// Supplies the default bind address of `0.0.0.0` so the service listens on every interface.
fn default_bind_address() -> String {
    DEFAULT_SERVER_BIND_ADDRESS.to_owned()
}

/// Supplies the default bind port of `8080` that matches other CLP services.
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

/// Supplies the default MySQL server port (`3306`).
const fn default_mysql_port() -> u16 {
    3306
}

/// Supplies the default maximum connection count for the MySQL pool.
const fn default_max_connections() -> u32 {
    DEFAULT_MAX_CONNECTIONS
}

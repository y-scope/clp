#![allow(dead_code)]

use std::{
    fs,
    net::SocketAddr,
    path::{Path, PathBuf},
};

use secrecy::SecretString;
use serde::Deserialize;

use crate::error::{ServiceError, ServiceResult};

const DEFAULT_MAX_CONNECTIONS: u32 = 5;
const DEFAULT_SERVER_BIND_ADDRESS: &str = "0.0.0.0";
const DEFAULT_SERVER_PORT: u16 = 8080;
const DEFAULT_JWT_TTL_SECONDS: u64 = 3600;

/// Root configuration deserialized from `credential-manager-config.yml`.
#[derive(Debug, Clone, Deserialize)]
pub struct AppConfig {
    #[serde(default)]
    pub server: ServerConfig,
    pub database: DatabaseConfig,
    pub jwt: JwtConfig,
    #[serde(default)]
    pub credentials_file: Option<PathBuf>,
    #[serde(default)]
    pub default_credential: Option<serde_yaml::Value>,
}

impl AppConfig {
    /// Loads configuration from disk and validates YAML syntax.
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
    fn default() -> Self {
        Self {
            bind_address: default_bind_address(),
            port: default_bind_port(),
        }
    }
}

fn default_bind_address() -> String {
    DEFAULT_SERVER_BIND_ADDRESS.to_owned()
}

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

const fn default_mysql_port() -> u16 {
    3306
}

const fn default_max_connections() -> u32 {
    DEFAULT_MAX_CONNECTIONS
}

/// JWT-related settings shared across token issuance endpoints.
#[derive(Debug, Clone, Deserialize)]
pub struct JwtConfig {
    pub secret: SecretString,
    #[serde(default = "default_jwt_ttl")]
    pub token_ttl_seconds: u64,
}

const fn default_jwt_ttl() -> u64 {
    DEFAULT_JWT_TTL_SECONDS
}

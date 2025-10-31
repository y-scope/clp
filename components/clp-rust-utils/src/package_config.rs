pub mod config;
pub mod credentials;
pub use config::Config;
pub use credentials::Credentials;

pub const DEFAULT_CONFIG_FILE_PATH: &str = "etc/clp-config.yml";
pub const DEFAULT_CREDENTIALS_FILE_PATH: &str = "etc/credentials.yml";

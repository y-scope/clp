pub mod config;
pub mod credentials;
pub use config::Config;
pub use credentials::Credentials;

/// Mirror of configuration paths in `clp_py_utils.clp_config`.
pub const DEFAULT_CONFIG_FILE_PATH: &str = "etc/clp-config.yml";
pub const DEFAULT_CREDENTIALS_FILE_PATH: &str = "etc/credentials.yml";

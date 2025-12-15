use std::sync::LazyLock;

use non_empty_string::NonEmptyString;

use crate::types::non_empty_string::ExpectedNonEmpty;

pub mod config;
pub mod credentials;

/// Mirror of configuration paths in `clp_py_utils.clp_config`.
pub const DEFAULT_CONFIG_FILE_PATH: &str = "etc/clp-config.yaml";
pub const DEFAULT_CREDENTIALS_FILE_PATH: &str = "etc/credentials.yaml";

pub static DEFAULT_DATASET_NAME: LazyLock<NonEmptyString> =
    LazyLock::new(|| NonEmptyString::from_static_str("default"));

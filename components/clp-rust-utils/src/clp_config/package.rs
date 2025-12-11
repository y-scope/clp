use non_empty_string::NonEmptyString;

pub mod config;
pub mod credentials;

/// Mirror of configuration paths in `clp_py_utils.clp_config`.
pub const DEFAULT_CONFIG_FILE_PATH: &str = "etc/clp-config.yaml";
pub const DEFAULT_CREDENTIALS_FILE_PATH: &str = "etc/credentials.yaml";

const DEFAULT_DATASET_NAME: &str = "default";

/// Mirror of constants in `clp_py_utils.clp_config`.
///
/// # Returns
///
/// A [`NonEmptyString`] representing the default dataset name.
///
/// # Panics
///
/// Panics if the default dataset name (configured at compile time) is empty.
#[must_use]
pub fn default_dataset() -> NonEmptyString {
    NonEmptyString::new(DEFAULT_DATASET_NAME.to_string())
        .unwrap_or_else(|_| panic!("Default dataset name should be a valid NonEmptyString"))
}

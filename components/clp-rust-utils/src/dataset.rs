use std::sync::LazyLock;

use regex::Regex;

/// The default dataset name (mirror of `clp_py_utils.clp_config.CLP_DEFAULT_DATASET_NAME`).
pub const CLP_DEFAULT_DATASET_NAME: &str = "default";

pub static VALID_DATASET_NAME_REGEX: LazyLock<Regex> =
    LazyLock::new(|| Regex::new(r"^[a-zA-Z0-9_]+$").unwrap());

/// # Returns
///
/// `dataset` when set, otherwise the `CLP_S` default dataset name [`CLP_DEFAULT_DATASET_NAME`].
#[must_use]
pub fn resolve_dataset_name(dataset: Option<&str>) -> &str {
    dataset.unwrap_or(CLP_DEFAULT_DATASET_NAME)
}

#[cfg(test)]
mod tests {
    use super::{CLP_DEFAULT_DATASET_NAME, resolve_dataset_name};

    #[test]
    fn resolve_dataset_name_passes_through_some() {
        assert_eq!(resolve_dataset_name(Some("mydataset")), "mydataset");
    }

    #[test]
    fn resolve_dataset_name_defaults_none_to_default() {
        assert_eq!(resolve_dataset_name(None), CLP_DEFAULT_DATASET_NAME);
    }
}

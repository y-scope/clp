/// The default dataset name (mirror of `clp_py_utils.clp_config.CLP_DEFAULT_DATASET_NAME`).
pub const CLP_DEFAULT_DATASET_NAME: &str = "default";

/// Mirror of the bound in `clp_package_utils.general.validate_dataset_name`, using the fixed
/// `clp_` table prefix and the longest metadata-table suffix, `column_metadata`.
pub const DATASET_NAME_MAX_LEN: usize = 64 - "clp_".len() - 1 - "column_metadata".len();

/// Checks the dataset-name contract enforced by the Python compression and dataset tools.
///
/// Python's `\w` accepts Unicode letters and numbers as well as underscores. Count characters,
/// not UTF-8 bytes, when applying the MySQL table-name limit.
#[must_use]
pub fn is_valid_dataset_name(dataset: &str) -> bool {
    !dataset.is_empty()
        && dataset.chars().count() <= DATASET_NAME_MAX_LEN
        && dataset.chars().all(|ch| ch == '_' || ch.is_alphanumeric())
}

/// # Returns
///
/// `dataset` when set, otherwise the `CLP_S` default dataset name [`CLP_DEFAULT_DATASET_NAME`].
#[must_use]
pub fn resolve_dataset_name(dataset: Option<&str>) -> &str {
    dataset.unwrap_or(CLP_DEFAULT_DATASET_NAME)
}

#[cfg(test)]
mod tests {
    use super::CLP_DEFAULT_DATASET_NAME;
    use super::DATASET_NAME_MAX_LEN;
    use super::is_valid_dataset_name;
    use super::resolve_dataset_name;

    #[test]
    fn accepts_dataset_names_allowed_by_the_python_cli() {
        assert!(is_valid_dataset_name("journaux_été"));
        assert!(is_valid_dataset_name(&"é".repeat(DATASET_NAME_MAX_LEN)));
    }

    #[test]
    fn rejects_names_the_python_cli_rejects() {
        assert!(!is_valid_dataset_name(""));
        assert!(!is_valid_dataset_name("bad-name"));
        assert!(!is_valid_dataset_name("e\u{301}"));
        assert!(!is_valid_dataset_name(
            &"é".repeat(DATASET_NAME_MAX_LEN + 1)
        ));
    }

    #[test]
    fn resolve_dataset_name_passes_through_some() {
        assert_eq!(resolve_dataset_name(Some("mydataset")), "mydataset");
    }

    #[test]
    fn resolve_dataset_name_defaults_none_to_default() {
        assert_eq!(resolve_dataset_name(None), CLP_DEFAULT_DATASET_NAME);
    }
}

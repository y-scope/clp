use std::sync::LazyLock;

use regex::Regex;

/// The default dataset name (mirror of `clp_py_utils.clp_config.CLP_DEFAULT_DATASET_NAME`).
pub const CLP_DEFAULT_DATASET_NAME: &str = "default";

pub static VALID_DATASET_NAME_REGEX: LazyLock<Regex> =
    LazyLock::new(|| Regex::new(r"^[a-zA-Z0-9_]+$").unwrap());

/// `MySQL`'s maximum table-name length (mirror of
/// `clp_py_utils.clp_metadata_db_utils.MYSQL_TABLE_NAME_MAX_LEN`).
const MYSQL_TABLE_NAME_MAX_LEN: usize = 64;

/// Length of the longest metadata-table suffix, `column_metadata` (mirror of
/// `clp_py_utils.clp_metadata_db_utils.TABLE_SUFFIX_MAX_LEN`).
const TABLE_SUFFIX_MAX_LEN: usize = "column_metadata".len();

/// Length of the metadata-table prefix (mirror of
/// `clp_py_utils.clp_config.CLP_METADATA_TABLE_PREFIX`). The prefix is a fixed constant in Rust;
/// see `clp_config::package::config::Database`.
const CLP_METADATA_TABLE_PREFIX_LEN: usize = "clp_".len();

/// Maximum dataset-name length that keeps every derived metadata table name within
/// [`MYSQL_TABLE_NAME_MAX_LEN`]. Mirror of the bound
/// `clp_package_utils.general.validate_dataset_name` enforces.
///
/// The subtracted `1` accounts for the separator between the dataset name and the table suffix.
pub const DATASET_NAME_MAX_LEN: usize =
    MYSQL_TABLE_NAME_MAX_LEN - CLP_METADATA_TABLE_PREFIX_LEN - 1 - TABLE_SUFFIX_MAX_LEN;

/// # Returns
///
/// Whether `dataset` is usable as a dataset name: non-empty, containing only alphanumeric
/// characters and underscores, and short enough that every metadata table name derived from it
/// fits within `MySQL`'s table-name limit.
#[must_use]
pub fn is_valid_dataset_name(dataset: &str) -> bool {
    dataset.len() <= DATASET_NAME_MAX_LEN && VALID_DATASET_NAME_REGEX.is_match(dataset)
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
    fn dataset_name_max_len_matches_the_python_bound() {
        assert_eq!(DATASET_NAME_MAX_LEN, 44);
    }

    #[test]
    fn is_valid_dataset_name_accepts_ordinary_names() {
        assert!(is_valid_dataset_name("default"));
        assert!(is_valid_dataset_name("my_dataset_1"));
        assert!(is_valid_dataset_name(&"a".repeat(DATASET_NAME_MAX_LEN)));
    }

    #[test]
    fn is_valid_dataset_name_rejects_empty_illegal_and_overlong_names() {
        assert!(!is_valid_dataset_name(""));
        assert!(!is_valid_dataset_name("has-a-dash"));
        assert!(!is_valid_dataset_name("has space"));
        assert!(!is_valid_dataset_name("../escape"));
        assert!(!is_valid_dataset_name(
            &"a".repeat(DATASET_NAME_MAX_LEN + 1)
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

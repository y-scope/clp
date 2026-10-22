//! Dataset names and the validation rules that the CLP package applies to them.

use std::fmt::Display;
use std::fmt::Formatter;
use std::str::FromStr;

use non_empty_string::NonEmptyString;
use unicode_properties::GeneralCategoryGroup;
use unicode_properties::UnicodeGeneralCategory;

use crate::clp_config::package::config::CLP_METADATA_TABLE_PREFIX;

/// The default dataset name (mirror of `clp_py_utils.clp_config.CLP_DEFAULT_DATASET_NAME`).
pub const CLP_DEFAULT_DATASET_NAME: &str = "default";

/// The maximum number of characters in a dataset name, as derived in
/// `clp_package_utils.general.validate_dataset_name`.
pub const DATASET_NAME_MAX_LEN: usize =
    MYSQL_TABLE_NAME_MAX_LEN - CLP_METADATA_TABLE_PREFIX.len() - 1 - TABLE_SUFFIX_MAX_LEN;

/// A dataset name accepted by `clp_package_utils.general.validate_dataset_name`.
#[derive(Clone, Debug, Eq, Hash, PartialEq)]
pub struct DatasetName {
    value: NonEmptyString,
}

impl DatasetName {
    /// # Returns
    ///
    /// The dataset name as a string slice.
    #[must_use]
    pub fn as_str(&self) -> &str {
        self.value.as_str()
    }
}

impl FromStr for DatasetName {
    type Err = ParseDatasetNameError;

    /// Creates a dataset name from `value` if `clp_package_utils.general.validate_dataset_name`
    /// accepts it.
    ///
    /// # Returns
    ///
    /// The parsed dataset name on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ParseDatasetNameError::InvalidCharacters`] if `value` is empty or contains an invalid
    ///   character.
    /// * [`ParseDatasetNameError::TooLong`] if `value` has more than [`DATASET_NAME_MAX_LEN`]
    ///   characters.
    fn from_str(value: &str) -> Result<Self, Self::Err> {
        let Ok(non_empty_value) = NonEmptyString::from_str(value) else {
            return Err(ParseDatasetNameError::InvalidCharacters(value.to_owned()));
        };
        if !value.chars().all(|ch| {
            ch == '_'
                || matches!(
                    ch.general_category_group(),
                    GeneralCategoryGroup::Letter | GeneralCategoryGroup::Number
                )
        }) {
            return Err(ParseDatasetNameError::InvalidCharacters(value.to_owned()));
        }
        if value.chars().count() > DATASET_NAME_MAX_LEN {
            return Err(ParseDatasetNameError::TooLong(value.to_owned()));
        }
        Ok(Self {
            value: non_empty_value,
        })
    }
}

impl Display for DatasetName {
    fn fmt(&self, formatter: &mut Formatter<'_>) -> std::fmt::Result {
        self.value.fmt(formatter)
    }
}

impl From<DatasetName> for NonEmptyString {
    fn from(dataset: DatasetName) -> Self {
        dataset.value
    }
}

/// An error returned when parsing an invalid dataset name.
#[derive(Debug, Eq, PartialEq, thiserror::Error)]
pub enum ParseDatasetNameError {
    #[error(
        "invalid dataset name {0:?}: names can only contain alphanumeric characters and \
         underscores"
    )]
    InvalidCharacters(String),

    #[error(
        "invalid dataset name {0:?}: names can only be a maximum of {DATASET_NAME_MAX_LEN} \
         characters long"
    )]
    TooLong(String),
}

/// # Returns
///
/// `dataset` when set, otherwise the `CLP_S` default dataset name [`CLP_DEFAULT_DATASET_NAME`].
#[must_use]
pub fn resolve_dataset_name(dataset: Option<&str>) -> &str {
    dataset.unwrap_or(CLP_DEFAULT_DATASET_NAME)
}

/// Mirror of `clp_py_utils.clp_metadata_db_utils.MYSQL_TABLE_NAME_MAX_LEN`. Must be kept in sync.
const MYSQL_TABLE_NAME_MAX_LEN: usize = 64;

/// Mirror of `clp_py_utils.clp_metadata_db_utils.TABLE_SUFFIX_MAX_LEN`. Must be kept in sync.
const TABLE_SUFFIX_MAX_LEN: usize = "column_metadata".len();

#[cfg(test)]
mod tests {
    use super::CLP_DEFAULT_DATASET_NAME;
    use super::DATASET_NAME_MAX_LEN;
    use super::DatasetName;
    use super::MYSQL_TABLE_NAME_MAX_LEN;
    use super::ParseDatasetNameError;
    use super::resolve_dataset_name;
    use crate::clp_config::package::config::Database;

    #[test]
    fn dataset_name_accepts_names_allowed_by_the_python_cli() {
        for name in [
            "default",
            "my_dataset_01",
            "_",
            "journaux_été",
            "日志_数据",
            "данные",
            "logs_٣٤",
            "logs_१२",
            "Ⅻ_logs",
            "logs_²",
            "½_logs",
        ] {
            let dataset: DatasetName = name.parse().expect("name should be a valid dataset name");
            assert_eq!(dataset.as_str(), name);
            assert_eq!(dataset.to_string(), name);
        }
    }

    #[test]
    fn dataset_name_rejects_characters_rejected_by_the_python_cli() {
        for name in [
            "", "-", " ", "bad-name", "bad name", "bad.name", "bad`name", "e\u{301}", "a\u{345}",
        ] {
            assert_eq!(
                name.parse::<DatasetName>(),
                Err(ParseDatasetNameError::InvalidCharacters(name.to_owned()))
            );
        }
    }

    #[test]
    fn dataset_name_length_is_bounded_in_characters() {
        let longest_name = "é".repeat(DATASET_NAME_MAX_LEN);
        let dataset: DatasetName = longest_name
            .parse()
            .expect("a name of the maximum length should be valid");
        assert_eq!(dataset.as_str(), longest_name);

        let too_long_name = "é".repeat(DATASET_NAME_MAX_LEN + 1);
        assert_eq!(
            too_long_name.parse::<DatasetName>(),
            Err(ParseDatasetNameError::TooLong(too_long_name))
        );
    }

    #[test]
    fn dataset_name_characters_are_checked_before_length() {
        let name = "-".repeat(DATASET_NAME_MAX_LEN + 1);
        assert_eq!(
            name.parse::<DatasetName>(),
            Err(ParseDatasetNameError::InvalidCharacters(name))
        );
    }

    #[test]
    fn longest_dataset_name_fills_the_longest_table_name() {
        let dataset: DatasetName = "a"
            .repeat(DATASET_NAME_MAX_LEN)
            .parse()
            .expect("a name of the maximum length should be valid");
        let table_name = Database::default().column_metadata_table_name(Some(&dataset));
        assert_eq!(table_name.chars().count(), MYSQL_TABLE_NAME_MAX_LEN);
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

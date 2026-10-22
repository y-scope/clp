//! Dataset names and the validation rules that the CLP package applies to them.

use std::fmt::Display;
use std::fmt::Formatter;
use std::str::FromStr;

use serde::Deserialize;
use serde::Serialize;
use unicode_properties::GeneralCategoryGroup;
use unicode_properties::UnicodeGeneralCategory;

use crate::clp_config::package::config::CLP_METADATA_TABLE_PREFIX;

/// The name of the dataset used when none is specified.
pub const CLP_DEFAULT_DATASET_NAME: &str = "default";

/// The maximum number of characters in a dataset name.
///
/// The bound ensures that the longest metadata table name, `<table prefix><dataset>_<longest
/// suffix>` with the table prefix [`CLP_METADATA_TABLE_PREFIX`], fits MySQL's 64-character
/// table-name limit.
pub const DATASET_NAME_MAX_LEN: usize =
    MYSQL_TABLE_NAME_MAX_LEN - CLP_METADATA_TABLE_PREFIX.len() - 1 - TABLE_SUFFIX_MAX_LEN;

/// A validated dataset name.
///
/// A dataset name is non-empty, only contains `_` or Unicode letters and numbers (general
/// categories L and N), and is at most [`DATASET_NAME_MAX_LEN`] characters long.
#[derive(Clone, Debug, Deserialize, Eq, Hash, PartialEq, Serialize)]
#[serde(try_from = "String", into = "String")]
pub struct DatasetName {
    value: String,
}

impl DatasetName {
    /// # Returns
    ///
    /// The dataset name as a string slice.
    #[must_use]
    pub const fn as_str(&self) -> &str {
        self.value.as_str()
    }
}

impl FromStr for DatasetName {
    type Err = ParseDatasetNameError;

    /// Creates a dataset name from `value` if it is a valid [`DatasetName`].
    ///
    /// # Returns
    ///
    /// The parsed dataset name on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`DatasetName::try_from`]'s return values on failure.
    fn from_str(value: &str) -> Result<Self, Self::Err> {
        Self::try_from(value.to_owned())
    }
}

impl TryFrom<String> for DatasetName {
    type Error = ParseDatasetNameError;

    /// Creates a dataset name from `value` if it is a valid [`DatasetName`].
    ///
    /// # Returns
    ///
    /// The dataset name on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ParseDatasetNameError::InvalidCharacters`] if `value` is empty or contains an invalid
    ///   character.
    /// * [`ParseDatasetNameError::TooLong`] if `value` has more than [`DATASET_NAME_MAX_LEN`]
    ///   characters.
    fn try_from(value: String) -> Result<Self, Self::Error> {
        if value.is_empty()
            || !value.chars().all(|ch| {
                ch == '_'
                    || matches!(
                        ch.general_category_group(),
                        GeneralCategoryGroup::Letter | GeneralCategoryGroup::Number
                    )
            })
        {
            return Err(ParseDatasetNameError::InvalidCharacters(value));
        }
        if value.chars().count() > DATASET_NAME_MAX_LEN {
            return Err(ParseDatasetNameError::TooLong(value));
        }
        Ok(Self { value })
    }
}

impl Display for DatasetName {
    fn fmt(&self, formatter: &mut Formatter<'_>) -> std::fmt::Result {
        self.value.fmt(formatter)
    }
}

impl From<DatasetName> for String {
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

/// The maximum number of characters in a MySQL table name.
const MYSQL_TABLE_NAME_MAX_LEN: usize = 64;

/// The length of the longest metadata table name suffix.
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
    fn dataset_name_accepts_underscores_letters_and_numbers() {
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
    fn dataset_name_rejects_empty_names_and_other_characters() {
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
    fn dataset_name_encodes_in_msgpack_as_a_string() {
        const NAME: &str = "my_dataset";
        let dataset: DatasetName = NAME.parse().expect("name should be a valid dataset name");

        assert_eq!(
            rmp_serde::to_vec(&dataset).expect("dataset name should serialize"),
            rmp_serde::to_vec(NAME).expect("string should serialize")
        );
        assert_eq!(
            rmp_serde::to_vec(&Some(dataset.clone())).expect("dataset name should serialize"),
            rmp_serde::to_vec(&Some(NAME.to_owned())).expect("string should serialize")
        );
        assert_eq!(
            rmp_serde::to_vec(&None::<DatasetName>).expect("`None` should serialize"),
            rmp_serde::to_vec(&None::<String>).expect("`None` should serialize")
        );

        let serialized = rmp_serde::to_vec(NAME).expect("string should serialize");
        let deserialized: DatasetName =
            rmp_serde::from_slice(&serialized).expect("dataset name should deserialize");
        assert_eq!(deserialized, dataset);
    }

    #[test]
    fn dataset_name_msgpack_deserialization_rejects_invalid_names() {
        for name in [
            String::new(),
            "bad-name".to_owned(),
            "a".repeat(DATASET_NAME_MAX_LEN + 1),
        ] {
            let serialized = rmp_serde::to_vec(&name).expect("string should serialize");
            let error = rmp_serde::from_slice::<DatasetName>(&serialized)
                .expect_err("an invalid dataset name should fail to deserialize");
            let parse_error = name
                .parse::<DatasetName>()
                .expect_err("name should be an invalid dataset name");
            assert!(
                error.to_string().contains(&parse_error.to_string()),
                "unexpected error: {error}"
            );
        }
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

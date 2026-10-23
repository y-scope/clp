//! Dataset names and the validation rules that the CLP package applies to them.

use std::fmt::Display;
use std::fmt::Formatter;
use std::str::FromStr;
use std::sync::LazyLock;

use regex::Regex;
use serde::Deserialize;
use serde::Serialize;
use sqlx::MySql;
use sqlx::encode::IsNull;
use sqlx::error::BoxDynError;
use sqlx::mysql::MySqlTypeInfo;
use sqlx::mysql::MySqlValueRef;

use crate::clp_config::package::config::CLP_METADATA_TABLE_PREFIX;

/// A validated dataset name.
///
/// A dataset name is non-empty, only contains `_` or Unicode letters and numbers (general
/// categories L and N) in the Basic Multilingual Plane, and is short enough for its longest
/// metadata table name to fit MySQL's 64-character table-name limit.
///
/// A dataset name serializes as a plain string. It deserializes from a string, validated by the
/// same rules as its [`FromStr`] implementation, or from a `null` or missing value, which gives the
/// default dataset name ([`DatasetName::default_name`]).
///
/// A dataset name binds to and decodes from MySQL as a string, validated on decode by the same
/// rules.
#[derive(Clone, Debug, Deserialize, Eq, Hash, PartialEq, Serialize)]
#[serde(try_from = "Option<String>", into = "String")]
pub struct DatasetName {
    value: String,
}

impl DatasetName {
    /// # Returns
    ///
    /// The name of the dataset used when none is specified.
    #[must_use]
    pub const fn default_name() -> &'static str {
        CLP_DEFAULT_DATASET_NAME
    }

    /// # Returns
    ///
    /// The dataset name as a string slice.
    #[must_use]
    pub const fn as_str(&self) -> &str {
        self.value.as_str()
    }

    /// # Returns
    ///
    /// Whether the dataset name is [`DatasetName::default_name`].
    #[must_use]
    pub fn is_default(&self) -> bool {
        self.value == Self::default_name()
    }
}

impl Default for DatasetName {
    fn default() -> Self {
        Self {
            value: Self::default_name().to_owned(),
        }
    }
}

impl FromStr for DatasetName {
    type Err = ParseDatasetNameError;

    fn from_str(value: &str) -> Result<Self, Self::Err> {
        Self::try_from(value.to_owned())
    }
}

impl TryFrom<String> for DatasetName {
    type Error = ParseDatasetNameError;

    fn try_from(value: String) -> Result<Self, Self::Error> {
        // MySQL identifiers can only contain characters in the Basic Multilingual Plane.
        const VALID_CHARACTERS_PATTERN: &str = r"\A[\p{L}\p{N}_&&\x{0000}-\x{FFFF}]+\z";

        static VALID_CHARACTERS_REGEX: LazyLock<Regex> =
            LazyLock::new(|| Regex::new(VALID_CHARACTERS_PATTERN).expect("invalid regex pattern"));

        if value.is_empty() {
            return Err(ParseDatasetNameError::Empty);
        }
        if !VALID_CHARACTERS_REGEX.is_match(&value) {
            return Err(ParseDatasetNameError::InvalidCharacters(value));
        }
        if value.chars().count() > DATASET_NAME_MAX_LEN {
            return Err(ParseDatasetNameError::TooLong(value));
        }
        Ok(Self { value })
    }
}

impl TryFrom<Option<String>> for DatasetName {
    type Error = ParseDatasetNameError;

    fn try_from(value: Option<String>) -> Result<Self, Self::Error> {
        value.map_or_else(|| Ok(Self::default()), Self::try_from)
    }
}

impl Display for DatasetName {
    fn fmt(&self, formatter: &mut Formatter<'_>) -> std::fmt::Result {
        self.value.fmt(formatter)
    }
}

impl sqlx::Type<MySql> for DatasetName {
    fn type_info() -> MySqlTypeInfo {
        <str as sqlx::Type<MySql>>::type_info()
    }

    fn compatible(ty: &MySqlTypeInfo) -> bool {
        <str as sqlx::Type<MySql>>::compatible(ty)
    }
}

impl sqlx::Encode<'_, MySql> for DatasetName {
    fn encode_by_ref(&self, buf: &mut Vec<u8>) -> Result<IsNull, BoxDynError> {
        <&str as sqlx::Encode<MySql>>::encode(self.as_str(), buf)
    }
}

impl<'decode> sqlx::Decode<'decode, MySql> for DatasetName {
    fn decode(value: MySqlValueRef<'decode>) -> Result<Self, BoxDynError> {
        let value = <&str as sqlx::Decode<MySql>>::decode(value)?;
        value.parse::<Self>().map_err(Into::into)
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
    #[error("dataset name must not be empty")]
    Empty,

    #[error(
        "invalid dataset name {0:?}: names can only contain underscores and alphanumeric \
         characters in the Basic Multilingual Plane"
    )]
    InvalidCharacters(String),

    #[error(
        "invalid dataset name {0:?}: names can only be a maximum of {DATASET_NAME_MAX_LEN} \
         characters long"
    )]
    TooLong(String),
}

/// The name of the dataset used when none is specified.
const CLP_DEFAULT_DATASET_NAME: &str = "default";

/// The maximum number of characters in a dataset name.
///
/// The bound ensures that the longest metadata table name, `<table prefix><dataset>_<longest
/// suffix>` with the table prefix [`CLP_METADATA_TABLE_PREFIX`], fits MySQL's 64-character
/// table-name limit.
const DATASET_NAME_MAX_LEN: usize =
    MYSQL_TABLE_NAME_MAX_LEN - CLP_METADATA_TABLE_PREFIX.len() - 1 - TABLE_SUFFIX_MAX_LEN;

/// The maximum number of characters in a MySQL table name.
const MYSQL_TABLE_NAME_MAX_LEN: usize = 64;

/// The length of the longest metadata table name suffix.
const TABLE_SUFFIX_MAX_LEN: usize = "column_metadata".len();

#[cfg(test)]
mod tests {
    use serde::Deserialize;
    use sqlx::MySql;
    use sqlx::encode::IsNull;

    use super::DATASET_NAME_MAX_LEN;
    use super::DatasetName;
    use super::ParseDatasetNameError;

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
        assert_eq!("".parse::<DatasetName>(), Err(ParseDatasetNameError::Empty));
        for name in [
            "-",
            " ",
            "bad-name",
            "bad name",
            "bad.name",
            "bad`name",
            "e\u{301}",
            "a\u{345}",
            "𝒜_logs",
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
    fn dataset_name_encodes_in_msgpack_as_a_string() {
        for (dataset, name) in [
            (
                "my_dataset"
                    .parse()
                    .expect("name should be a valid dataset name"),
                "my_dataset",
            ),
            (DatasetName::default(), "default"),
        ] {
            let serialized = rmp_serde::to_vec(name).expect("string should serialize");
            assert_eq!(
                rmp_serde::to_vec(&dataset).expect("dataset name should serialize"),
                serialized
            );

            let deserialized: DatasetName =
                rmp_serde::from_slice(&serialized).expect("dataset name should deserialize");
            assert_eq!(deserialized, dataset);
        }
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
            assert_eq!(error.to_string(), parse_error.to_string());
        }
    }

    #[test]
    fn dataset_name_encodes_in_mysql_as_a_string() {
        for (dataset, name) in [
            (
                "my_dataset"
                    .parse()
                    .expect("name should be a valid dataset name"),
                "my_dataset",
            ),
            (DatasetName::default(), "default"),
        ] {
            let mut encoded = Vec::new();
            let is_null =
                <DatasetName as sqlx::Encode<MySql>>::encode_by_ref(&dataset, &mut encoded)
                    .expect("dataset name should encode");
            assert!(matches!(is_null, IsNull::No));

            let mut expected = Vec::new();
            let expected_is_null =
                <&str as sqlx::Encode<MySql>>::encode_by_ref(&name, &mut expected)
                    .expect("string should encode");
            assert!(matches!(expected_is_null, IsNull::No));
            assert_eq!(encoded, expected);
        }
    }

    #[test]
    fn null_dataset_name_deserializes_as_the_default() {
        let nil = rmp_serde::to_vec(&None::<String>).expect("nil should serialize");
        assert_eq!(
            rmp_serde::from_slice::<DatasetName>(&nil).expect("msgpack nil should deserialize"),
            DatasetName::default()
        );
        assert_eq!(
            serde_json::from_str::<DatasetName>("null").expect("JSON `null` should deserialize"),
            DatasetName::default()
        );
    }

    #[test]
    fn absent_dataset_name_field_deserializes_as_the_default() {
        #[derive(Debug, Deserialize, PartialEq)]
        struct DefaultedConfig {
            #[serde(default)]
            dataset: DatasetName,
        }

        #[derive(Debug, Deserialize, PartialEq)]
        struct Config {
            dataset: DatasetName,
        }

        assert_eq!(
            serde_json::from_str::<DefaultedConfig>("{}")
                .expect("an empty defaulted config should deserialize"),
            DefaultedConfig {
                dataset: DatasetName::default()
            }
        );
        assert_eq!(
            serde_json::from_str::<Config>("{}").expect("an empty config should deserialize"),
            Config {
                dataset: DatasetName::default()
            }
        );
    }

    #[test]
    fn dataset_name_from_option_defaults_none_and_validates_some() {
        assert_eq!(
            DatasetName::try_from(None).map(String::from),
            Ok(DatasetName::default_name().to_owned())
        );
        assert_eq!(
            DatasetName::try_from(Some("my_dataset".to_owned())).map(String::from),
            Ok("my_dataset".to_owned())
        );
        assert_eq!(
            DatasetName::try_from(Some("bad-name".to_owned())),
            Err(ParseDatasetNameError::InvalidCharacters(
                "bad-name".to_owned()
            ))
        );
    }

    #[test]
    fn is_default_is_true_only_for_the_default_dataset_name() {
        assert!(DatasetName::default().is_default());
        assert!(
            DatasetName::default_name()
                .parse::<DatasetName>()
                .expect("the default name should be a valid dataset name")
                .is_default()
        );
        assert!(
            !"my_dataset"
                .parse::<DatasetName>()
                .expect("name should be a valid dataset name")
                .is_default()
        );
    }
}

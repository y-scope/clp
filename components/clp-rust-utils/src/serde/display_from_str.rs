//! Serde adapters that encode a value as its [`Display`] string and decode it with [`FromStr`],
//! for use with `#[serde(with = "...")]`.

use std::fmt::Display;
use std::str::FromStr;

use serde::Deserialize;
use serde::Deserializer;
use serde::Serializer;
use serde::de::Error as _;

/// Serializes `value` as its [`Display`] string.
///
/// # Type Parameters
///
/// * `ValueType` - The type of the value to serialize.
/// * `SerializerType` - The serializer to write the string to.
///
/// # Returns
///
/// The serializer's output on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`Serializer::collect_str`]'s return values on failure.
pub fn serialize<ValueType: Display, SerializerType: Serializer>(
    value: &ValueType,
    serializer: SerializerType,
) -> Result<SerializerType::Ok, SerializerType::Error> {
    serializer.collect_str(value)
}

/// Deserializes a string and parses it with [`FromStr`].
///
/// # Type Parameters
///
/// * `DeserializerType` - The deserializer to read the string from.
/// * `ValueType` - The type to parse the string into.
///
/// # Returns
///
/// The parsed value on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * `DeserializerType::Error` if the string fails to parse as `ValueType`.
/// * Forwards [`String::deserialize`]'s return values on failure.
pub fn deserialize<
    'deserializer,
    DeserializerType: Deserializer<'deserializer>,
    ValueType: FromStr<Err: Display>,
>(
    deserializer: DeserializerType,
) -> Result<ValueType, DeserializerType::Error> {
    String::deserialize(deserializer)?
        .parse()
        .map_err(DeserializerType::Error::custom)
}

#[cfg(test)]
mod tests {
    use serde::Deserialize;
    use serde::Serialize;
    use serde_json::Value;
    use serde_json::json;

    use crate::types::ArchiveId;

    #[derive(Debug, Deserialize, Eq, PartialEq, Serialize)]
    struct Wrapper {
        #[serde(with = "crate::serde::display_from_str")]
        archive_id: ArchiveId,
    }

    #[test]
    fn display_from_str_round_trips_value_as_its_display_string() {
        const ARCHIVE_ID: &str = "018e90e5-8b2a-4a61-a2fc-cac799936caf";
        let wrapper = Wrapper {
            archive_id: ARCHIVE_ID.parse().expect("valid UUID should parse"),
        };

        let serialized = rmp_serde::to_vec_named(&wrapper).expect("wrapper should serialize");
        assert_eq!(
            rmp_serde::from_slice::<Value>(&serialized)
                .expect("serialized wrapper should decode as a map"),
            json!({ "archive_id": ARCHIVE_ID })
        );
        assert_eq!(
            rmp_serde::from_slice::<Wrapper>(&serialized).expect("wrapper should deserialize"),
            wrapper
        );
    }

    #[test]
    fn display_from_str_rejects_string_that_fails_from_str_with_its_error_message() {
        const INVALID_ARCHIVE_ID: &str = "not-a-uuid";
        let serialized = rmp_serde::to_vec_named(&json!({ "archive_id": INVALID_ARCHIVE_ID }))
            .expect("map should serialize");

        let error = rmp_serde::from_slice::<Wrapper>(&serialized)
            .expect_err("invalid archive ID should fail to deserialize");
        let parse_error = INVALID_ARCHIVE_ID
            .parse::<ArchiveId>()
            .expect_err("invalid UUID should be rejected");
        assert!(
            error.to_string().contains(&parse_error.to_string()),
            "deserialization error `{error}` should contain the parse error `{parse_error}`"
        );
    }
}

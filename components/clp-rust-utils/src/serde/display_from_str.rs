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

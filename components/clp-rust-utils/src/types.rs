//! Shared types and extensions used across CLP components.

use std::fmt::Display;
use std::fmt::Formatter;

use serde::Deserialize;
use serde::Serialize;

/// An archive's UUID. External storage keys and database values use its canonical text form.
#[derive(Clone, Copy, Debug, Deserialize, Eq, Hash, PartialEq, Serialize)]
#[serde(transparent)]
pub struct ArchiveId {
    value: uuid::Uuid,
}

impl ArchiveId {
    /// Factory function.
    ///
    /// Creates an archive ID from a UUID string accepted by [`uuid::Uuid::parse_str`].
    ///
    /// # Returns
    ///
    /// The parsed archive ID on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ParseArchiveIdError`] if `value` is not a valid UUID.
    pub fn parse_str(value: &str) -> Result<Self, ParseArchiveIdError> {
        let id = uuid::Uuid::parse_str(value).map_err(|source| ParseArchiveIdError {
            value: value.to_owned(),
            source,
        })?;
        Ok(Self { value: id })
    }
}

impl Display for ArchiveId {
    fn fmt(&self, formatter: &mut Formatter<'_>) -> std::fmt::Result {
        self.value.fmt(formatter)
    }
}

/// An error returned when parsing an invalid external archive ID string.
#[derive(Debug, thiserror::Error)]
#[error("invalid archive ID {value:?}: {source}")]
pub struct ParseArchiveIdError {
    value: String,
    source: uuid::Error,
}

pub mod non_empty_string {
    use non_empty_string::NonEmptyString;

    /// Trait for creating [`NonEmptyString`] instances from an expected non-empty string.
    pub trait ExpectedNonEmpty {
        /// # Returns
        ///
        /// A [`NonEmptyString`] of the provided static string slice.
        ///
        /// # Panics
        ///
        /// Panics if the provided static string slice is empty.
        #[must_use]
        fn from_static_str(str: &'static str) -> NonEmptyString {
            NonEmptyString::new(str.to_owned()).expect("static &str shouldn't be empty")
        }

        /// # Returns
        ///
        /// A [`NonEmptyString`] of the provided string.
        ///
        /// # Panics
        ///
        /// Panics if the provided string is empty.
        #[must_use]
        fn from_string(str: String) -> NonEmptyString {
            NonEmptyString::new(str).expect("string shouldn't be empty")
        }
    }

    impl ExpectedNonEmpty for NonEmptyString {}
}

#[cfg(test)]
mod tests {
    //! Tests for parsing external archive IDs.

    use super::ArchiveId;

    #[test]
    fn parse_archive_id_accepts_valid_uuid_representations() {
        const ARCHIVE_ID: &str = "018e90e5-8b2a-4a61-a2fc-cac799936caf";
        for value in [
            ARCHIVE_ID.to_owned(),
            ARCHIVE_ID.to_ascii_uppercase(),
            ARCHIVE_ID.replace('-', ""),
            format!("{{{ARCHIVE_ID}}}"),
            format!("urn:uuid:{ARCHIVE_ID}"),
        ] {
            assert_eq!(
                ArchiveId::parse_str(&value)
                    .expect("valid UUID should parse")
                    .to_string(),
                ARCHIVE_ID
            );
        }
    }

    #[test]
    fn parse_archive_id_rejects_invalid_uuids() {
        for value in ["", "not-a-uuid", "018e90e5-8b2a-4a61-a2fc-cac799936cag"] {
            let error = ArchiveId::parse_str(value).expect_err("invalid UUID should be rejected");
            assert_eq!(error.value, value);
            assert!(
                error.to_string().contains(&format!("{value:?}")),
                "parse error should include the invalid archive ID"
            );
        }
    }
}

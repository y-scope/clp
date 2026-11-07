//! Timeline bucket validation errors.

use mongodb::bson::spec::ElementType;

/// Failures when validating aggregated timeline buckets.
#[derive(Debug, thiserror::Error)]
pub enum Error {
    #[error("timeline bucket is missing field `{field}`")]
    MissingField { field: &'static str },

    #[error("timeline field `{field}` must be a BSON integer, got {actual:?}")]
    NonIntegerField {
        field: &'static str,
        actual: ElementType,
    },

    #[error("timeline count must be nonnegative, got {count}")]
    NegativeCount { count: i64 },

    #[error("timeline aggregate contains invalid source contributions")]
    InvalidSourceContribution,

    #[error("combined timeline count exceeds the signed 64-bit integer range")]
    CountOverflow,
}

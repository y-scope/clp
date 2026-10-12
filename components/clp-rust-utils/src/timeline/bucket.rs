//! Validation of timeline buckets.

use mongodb::bson::Bson;
use mongodb::bson::Document;

use super::Error;

/// A validated timeline bucket with an integral timestamp and nonnegative count.
#[derive(Debug, Eq, PartialEq)]
pub struct Bucket {
    timestamp: i64,
    count: i64,
}

impl Bucket {
    /// Constructs a bucket from a document returned by [`super::aggregate_source_contributions`].
    ///
    /// # Returns
    ///
    /// The bucket on success.
    ///
    /// # Errors
    ///
    /// * [`Error::InvalidSourceContribution`] if the aggregate flags invalid source contributions.
    /// * [`Error::CountOverflow`] if the sum of valid source counts overflows a BSON integer.
    /// * [`Error::NegativeCount`] if the count is negative.
    /// * Forwards [`get_integer_field`]'s return values on failure.
    pub fn try_from_aggregation_document(document: &Document) -> Result<Self, Error> {
        if get_integer_field(document, "invalid")? != 0 {
            return Err(Error::InvalidSourceContribution);
        }
        // MongoDB promotes an overflowing integer sum to a double. Source validation above
        // ensures that floating-point operands cannot be mistaken for overflow.
        if matches!(document.get("count"), Some(Bson::Double(_))) {
            return Err(Error::CountOverflow);
        }
        let bucket = Self {
            timestamp: get_integer_field(document, "_id")?,
            count: get_integer_field(document, "count")?,
        };
        if bucket.count < 0 {
            return Err(Error::NegativeCount {
                count: bucket.count,
            });
        }
        Ok(bucket)
    }

    /// # Returns
    ///
    /// The bucket timestamp in Unix epoch milliseconds.
    #[must_use]
    pub const fn timestamp(&self) -> i64 {
        self.timestamp
    }

    /// # Returns
    ///
    /// The number of matching events in the bucket.
    #[must_use]
    pub const fn count(&self) -> i64 {
        self.count
    }
}

/// Reads a signed integer field from a BSON document without floating-point coercion.
///
/// # Returns
///
/// The integer on success.
///
/// # Errors
///
/// * [`Error::MissingField`] if the field is absent.
/// * [`Error::NonIntegerField`] if the field does not contain a BSON integer.
fn get_integer_field(document: &Document, field: &'static str) -> Result<i64, Error> {
    match document.get(field) {
        Some(Bson::Int32(value)) => Ok(i64::from(*value)),
        Some(Bson::Int64(value)) => Ok(*value),
        Some(value) => Err(Error::NonIntegerField {
            field,
            actual: value.element_type(),
        }),
        None => Err(Error::MissingField { field }),
    }
}

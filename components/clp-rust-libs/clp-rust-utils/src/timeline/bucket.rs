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
    /// Returns an error if:
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
/// Returns an error if:
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

#[cfg(test)]
mod tests {
    use mongodb::bson::Bson;
    use mongodb::bson::doc;

    use super::Bucket;
    use super::Error;

    #[test]
    fn accepts_integer_types_and_boundaries() -> anyhow::Result<()> {
        struct Case {
            document: mongodb::bson::Document,
            timestamp: i64,
            count: i64,
        }
        for case in [
            Case {
                document: doc! { "_id": i32::MIN, "count": i32::MAX, "invalid": 0_i32 },
                timestamp: i64::from(i32::MIN),
                count: i64::from(i32::MAX),
            },
            Case {
                document: doc! { "_id": i32::MAX, "count": 0_i32, "invalid": 0_i64 },
                timestamp: i64::from(i32::MAX),
                count: 0,
            },
            Case {
                document: doc! { "_id": i64::MIN, "count": i64::MAX, "invalid": 0_i32 },
                timestamp: i64::MIN,
                count: i64::MAX,
            },
            Case {
                document: doc! { "_id": i64::MAX, "count": 0_i64, "invalid": 0_i64 },
                timestamp: i64::MAX,
                count: 0,
            },
        ] {
            let bucket = Bucket::try_from_aggregation_document(&case.document)?;
            assert_eq!(bucket.timestamp(), case.timestamp);
            assert_eq!(bucket.count(), case.count);
        }
        Ok(())
    }

    #[test]
    fn rejects_missing_fields() -> anyhow::Result<()> {
        for missing in ["invalid", "_id", "count"] {
            let mut document = doc! { "_id": 0, "count": 1, "invalid": 0 };
            document.remove(missing);
            let error = Bucket::try_from_aggregation_document(&document)
                .expect_err("missing aggregation field must fail validation");
            anyhow::ensure!(
                matches!(error, Error::MissingField { field } if field == missing),
                "unexpected error for missing {missing}: {error}"
            );
        }
        Ok(())
    }

    #[test]
    fn rejects_noninteger_fields() -> anyhow::Result<()> {
        for field in ["invalid", "_id", "count"] {
            for value in [
                Bson::Null,
                Bson::String("1".to_owned()),
                Bson::Boolean(true),
                Bson::Array(vec![Bson::Int32(1)]),
                Bson::Document(doc! { "value": 1 }),
            ] {
                let expected_type = value.element_type();
                let mut document = doc! { "_id": 0, "count": 1, "invalid": 0 };
                document.insert(field, value);
                let error = Bucket::try_from_aggregation_document(&document)
                    .expect_err("noninteger aggregation field must fail validation");
                anyhow::ensure!(
                    matches!(error, Error::NonIntegerField { field: actual_field, actual }
                        if actual_field == field && actual == expected_type),
                    "unexpected error for {document}: {error}"
                );
            }
        }
        for field in ["invalid", "_id"] {
            let mut document = doc! { "_id": 0, "count": 1, "invalid": 0 };
            document.insert(field, 0.0);
            anyhow::ensure!(
                matches!(
                    Bucket::try_from_aggregation_document(&document),
                    Err(Error::NonIntegerField { field: actual_field, .. }) if actual_field == field
                ),
                "floating-point {field} must not be coerced to an integer"
            );
        }
        Ok(())
    }

    #[test]
    fn rejects_negative_counts() -> anyhow::Result<()> {
        for count in [-1_i64, i64::MIN] {
            let error = Bucket::try_from_aggregation_document(
                &doc! { "_id": 0, "count": count, "invalid": 0 },
            )
            .expect_err("negative count must fail validation");
            anyhow::ensure!(
                matches!(error, Error::NegativeCount { count: actual } if actual == count),
                "unexpected error for count {count}: {error}"
            );
        }
        Ok(())
    }

    /// Distinguishes MongoDB's promoted integer sum from malformed source operands.
    #[test]
    fn rejects_overflow_and_prioritizes_invalid_sources() -> anyhow::Result<()> {
        let mut document = doc! { "_id": 0, "count": 9_223_372_036_854_775_808.0, "invalid": 0 };
        anyhow::ensure!(
            matches!(
                Bucket::try_from_aggregation_document(&document),
                Err(Error::CountOverflow)
            ),
            "a promoted integer sum must be reported as overflow"
        );
        for invalid in [-1_i64, 1, i64::MAX] {
            for count in [Bson::Int32(1), Bson::Double(9_223_372_036_854_775_808.0)] {
                document.insert("invalid", invalid);
                document.insert("count", count);
                anyhow::ensure!(
                    matches!(
                        Bucket::try_from_aggregation_document(&document),
                        Err(Error::InvalidSourceContribution)
                    ),
                    "invalid sources must take precedence over count validation: {document}"
                );
            }
        }
        Ok(())
    }
}

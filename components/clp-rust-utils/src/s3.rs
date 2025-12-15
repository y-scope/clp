mod client;

use crate::s3_event::Record;
pub use client::create_new_client;

#[derive(Debug, Clone, PartialEq, Eq, PartialOrd, Ord)]
pub struct ObjectMetadata {
    pub bucket: String,
    pub key: String,
    pub size: u64,
}

/// Extracts S3 object metadata from the given event notification if it corresponds to a relevant
/// object.
///
/// # Parameters
///
/// * `record`: The record to extract object metadata from.
/// * `bucket_name`: The name of the bucket to extract object metadata from.
/// * `key_prefix`: The prefix of the key to extract object metadata from.
///
/// # Returns
///
/// * `Some(ObjectMetadata)` if the record corresponds to a relevant object.
/// * `None` if:
///   * The event is not an object creation event.
///   * The bucket name does not match the listener's configuration.
///   * [`Self::is_relevant_object`] evaluates to `false`.
pub fn extract_object_metadata(
    record: Record,
    bucket_name: &str,
    key_prefix: &str,
) -> Option<ObjectMetadata> {
    if !record.event_name.starts_with("ObjectCreated:")
        || bucket_name != record.s3.bucket.name.as_str()
        || !is_relevant_object(record.s3.object.key.as_str(), key_prefix)
    {
        return None;
    }
    Some(ObjectMetadata {
        bucket: record.s3.bucket.name,
        key: record.s3.object.key,
        size: record.s3.object.size,
    })
}

/// # Returns:
///
/// Whether the object key corresponds to a relevant object based on the listener's prefix.
fn is_relevant_object(object_key: &str, key_prefix: &str) -> bool {
    !object_key.ends_with('/') && object_key.starts_with(key_prefix)
}

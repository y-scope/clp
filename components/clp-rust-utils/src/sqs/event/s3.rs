use non_empty_string::NonEmptyString;
use serde::Deserialize;

/// Represents an Amazon S3 event notification message.
///
/// This struct models the essential fields required to process a single record from an S3 event
/// notification. Its schema follows the AWS S3 event notification format version 2.1.
///
/// For more information, see:
/// <https://docs.aws.amazon.com/AmazonS3/latest/userguide/notification-content-structure.html>
#[derive(Clone, Debug, Deserialize, Eq, PartialEq)]
pub struct S3 {
    #[serde(rename = "Records")]
    pub records: Vec<Record>,
}

#[derive(Clone, Debug, Deserialize, Eq, PartialEq)]
pub struct Record {
    pub s3: Entity,
    #[serde(rename = "eventName")]
    pub event_name: String,
}

#[derive(Clone, Debug, Deserialize, Eq, PartialEq)]
pub struct Entity {
    pub bucket: Bucket,
    pub object: Object,
}

#[derive(Clone, Debug, Deserialize, Eq, PartialEq)]
pub struct Bucket {
    pub name: NonEmptyString,
}

#[derive(Clone, Debug, Deserialize, Eq, PartialEq)]
pub struct Object {
    pub key: NonEmptyString,
    pub size: u64,
}

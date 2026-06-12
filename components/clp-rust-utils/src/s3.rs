mod client;
mod url;

pub use client::create_new_client;
use non_empty_string::NonEmptyString;
pub use url::{NormalizedS3ObjectUrls, ParsedS3Url, normalize_s3_object_urls, parse_s3_url};

/// Represents the unique identifier for an S3 object metadata entry in CLP DB.
pub type S3ObjectMetadataId = u64;

#[derive(Debug, Clone, PartialEq, Eq, PartialOrd, Ord)]
pub struct ObjectMetadata {
    pub bucket: NonEmptyString,
    pub key: NonEmptyString,
    pub size: u64,
}

mod client;
mod scan;

pub use client::create_new_client;
pub use scan::scan_prefix;

use non_empty_string::NonEmptyString;

/// Represents the unique identifier for an S3 object metadata entry in CLP DB.
pub type S3ObjectMetadataId = u64;

#[derive(Debug, Clone, PartialEq, Eq, PartialOrd, Ord)]
pub struct ObjectMetadata {
    pub bucket: NonEmptyString,
    pub key: NonEmptyString,
    pub size: u64,
}

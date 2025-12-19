mod client;

pub use client::create_new_client;
use non_empty_string::NonEmptyString;

#[derive(Debug, Clone, PartialEq, Eq, PartialOrd, Ord)]
pub struct ObjectMetadata {
    pub bucket: NonEmptyString,
    pub key: NonEmptyString,
    pub size: u64,
}

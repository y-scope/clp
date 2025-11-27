mod client;

pub use client::create_new_client;

#[derive(Debug, Clone, PartialEq, Eq, PartialOrd, Ord)]
pub struct ObjectMetadata {
    pub bucket: String,
    pub key: String,
    pub size: u64,
}

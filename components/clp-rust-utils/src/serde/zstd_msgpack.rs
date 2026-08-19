//! Zstandard-compressed `MessagePack` serialization utilities.

use serde::Serialize;
use serde::de::DeserializeOwned;

use crate::Error;

const ZSTD_COMPRESSION_LEVEL: i32 = 3;

/// Namespace for Zstandard-compressed `MessagePack` utilities.
pub enum ZstdMsgpack {}

impl ZstdMsgpack {
    /// Serialize a value to a Zstandard-compressed `MessagePack` byte sequence.
    ///
    /// # Type Parameters
    ///
    /// * `ValueType` - The type of value to serialize.
    ///
    /// # Returns
    ///
    /// The serialized byte sequence on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`rmp_serde::to_vec_named`]'s return values on failure.
    /// * Forwards [`zstd::stream::encode_all`]'s return values on failure.
    pub fn serialize<ValueType: Serialize + ?Sized>(val: &ValueType) -> Result<Vec<u8>, Error> {
        let msgpack_data = rmp_serde::to_vec_named(val)?;
        zstd::stream::encode_all(msgpack_data.as_slice(), ZSTD_COMPRESSION_LEVEL)
            .map_err(Error::Zstd)
    }

    /// Deserialize an owned value from a Zstandard-compressed `MessagePack` byte sequence.
    ///
    /// # Type Parameters
    ///
    /// * `ValueType` - The type of value to deserialize.
    ///
    /// # Returns
    ///
    /// The deserialized value on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`zstd::stream::decode_all`]'s return values on failure.
    /// * Forwards [`rmp_serde::from_slice`]'s return values on failure.
    pub fn deserialize<ValueType: DeserializeOwned>(data: &[u8]) -> Result<ValueType, Error> {
        let msgpack_data = zstd::stream::decode_all(data).map_err(Error::Zstd)?;
        rmp_serde::from_slice(&msgpack_data).map_err(Into::into)
    }
}

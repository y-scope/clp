//! Zstandard-compressed `MessagePack` serialization utilities.

use std::ffi::{CStr, c_void};

use libzstd_rs_sys::{ZSTD_compress, ZSTD_compressBound, ZSTD_getErrorName, ZSTD_isError};
use serde::Serialize;

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
    /// * Forwards [`zstd_compress`]'s return values on failure.
    pub fn serialize<ValueType: Serialize + ?Sized>(val: &ValueType) -> Result<Vec<u8>, Error> {
        let msgpack_data = rmp_serde::to_vec_named(val)?;
        zstd_compress(msgpack_data.as_slice())
    }
}

/// Creates an error from a zstd result code.
///
/// # Returns
///
/// A zstd error containing the human-readable zstd error name.
fn zstd_error(result: usize) -> Error {
    let error_name = ZSTD_getErrorName(result);
    // SAFETY: `ZSTD_getErrorName` returns a pointer to a static, null-terminated error string for
    // every result code and the pointer remains valid after this call.
    let error_name = unsafe { CStr::from_ptr(error_name) };
    Error::Zstd(error_name.to_string_lossy().into_owned())
}

/// Compresses bytes with zstd.
///
/// # Returns
///
/// The zstd-compressed bytes on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`Error::Zstd`] if the zstd compression bound or compression operation fails.
fn zstd_compress(src: &[u8]) -> Result<Vec<u8>, Error> {
    let dst_capacity = ZSTD_compressBound(src.len());
    if 0 != ZSTD_isError(dst_capacity) {
        return Err(zstd_error(dst_capacity));
    }

    let mut dst = vec![0_u8; dst_capacity];
    // SAFETY: `dst` is valid for `dst.len()` writable bytes, `src` is valid for `src.len()`
    // readable bytes (including when empty), and `ZSTD_compress` does not retain either
    // pointer.
    let compressed_size = unsafe {
        ZSTD_compress(
            dst.as_mut_ptr().cast::<c_void>(),
            dst.len(),
            src.as_ptr().cast::<c_void>(),
            src.len(),
            ZSTD_COMPRESSION_LEVEL,
        )
    };
    if 0 != ZSTD_isError(compressed_size) {
        return Err(zstd_error(compressed_size));
    }

    dst.truncate(compressed_size);
    Ok(dst)
}

#[cfg(test)]
mod tests {
    use std::error::Error as StdError;

    use libzstd_rs_sys::{
        ZSTD_CONTENTSIZE_ERROR,
        ZSTD_CONTENTSIZE_UNKNOWN,
        ZSTD_decompress,
        ZSTD_getFrameContentSize,
    };
    use serde_json::Value;

    use super::*;

    #[derive(Serialize)]
    struct TestValue {
        message: &'static str,
        count: u32,
    }

    #[test]
    fn serialize_produces_decompressible_named_messagepack() -> Result<(), Box<dyn StdError>> {
        let compressed = ZstdMsgpack::serialize(&TestValue {
            message: "hello",
            count: 3,
        })?;
        let msgpack = decompress_zstd(&compressed);
        let actual: Value = rmp_serde::from_slice(&msgpack)?;
        assert_eq!(serde_json::json!({"message": "hello", "count": 3}), actual);
        Ok(())
    }

    #[test]
    fn zstd_error_includes_human_readable_name() {
        let result = ZSTD_compressBound(usize::MAX);
        assert_ne!(0, ZSTD_isError(result));

        let Error::Zstd(error_name) = zstd_error(result) else {
            panic!("expected a zstd error");
        };
        assert_eq!("Src size is incorrect", error_name);
    }

    fn decompress_zstd(src: &[u8]) -> Vec<u8> {
        // SAFETY: `src` is valid for `src.len()` readable bytes and the function does not retain
        // the pointer.
        let content_size =
            unsafe { ZSTD_getFrameContentSize(src.as_ptr().cast::<c_void>(), src.len()) };
        assert_ne!(ZSTD_CONTENTSIZE_ERROR, content_size);
        assert_ne!(ZSTD_CONTENTSIZE_UNKNOWN, content_size);

        let mut dst = vec![
            0_u8;
            usize::try_from(content_size)
                .expect("zstd frame content should fit in memory")
        ];
        // SAFETY: `dst` is valid for `dst.len()` writable bytes, `src` is valid for `src.len()`
        // readable bytes, and `ZSTD_decompress` does not retain either pointer.
        let decompressed_size = unsafe {
            ZSTD_decompress(
                dst.as_mut_ptr().cast::<c_void>(),
                dst.len(),
                src.as_ptr().cast::<c_void>(),
                src.len(),
            )
        };
        assert_eq!(0, ZSTD_isError(decompressed_size));
        assert_eq!(dst.len(), decompressed_size);
        dst
    }
}

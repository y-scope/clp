//! Zstandard-compressed `MessagePack` serialization utilities.

use std::{ffi::c_void, ptr};

use libzstd_rs_sys::{ZSTD_compress, ZSTD_compressBound, ZSTD_isError};
use serde::Serialize;

use crate::Error;

const ZSTD_COMPRESSION_LEVEL: i32 = 3;

/// Namespace for Zstandard-compressed `MessagePack` utilities.
pub struct ZstdMsgpack;

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
/// A zstd error containing the zstd error code.
fn zstd_error(result: usize) -> Error {
    Error::Zstd(format!("error code {result}"))
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
    // SAFETY: `dst` is valid for `dst.len()` writable bytes. `src` is either null with length zero
    // or valid for `src.len()` readable bytes. `ZSTD_compress` does not retain either pointer.
    let compressed_size = unsafe {
        ZSTD_compress(
            dst.as_mut_ptr().cast::<c_void>(),
            dst.len(),
            if src.is_empty() {
                ptr::null()
            } else {
                src.as_ptr().cast::<c_void>()
            },
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

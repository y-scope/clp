//! Zstandard-compressed `MessagePack` serialization utilities.

use std::ffi::CStr;
use std::ffi::c_void;

use libzstd_rs_sys::ZSTD_CONTENTSIZE_ERROR;
use libzstd_rs_sys::ZSTD_CONTENTSIZE_UNKNOWN;
use libzstd_rs_sys::ZSTD_compress;
use libzstd_rs_sys::ZSTD_compressBound;
use libzstd_rs_sys::ZSTD_decompress;
use libzstd_rs_sys::ZSTD_getErrorName;
use libzstd_rs_sys::ZSTD_getFrameContentSize;
use libzstd_rs_sys::ZSTD_isError;
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
    /// * Forwards [`zstd_compress`]'s return values on failure.
    pub fn serialize<ValueType: Serialize + ?Sized>(val: &ValueType) -> Result<Vec<u8>, Error> {
        let msgpack_data = rmp_serde::to_vec_named(val)?;
        zstd_compress(msgpack_data.as_slice())
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
    /// * [`Error::Zstd`] if the input is not a single-pass zstd frame or decompression fails.
    /// * Forwards [`rmp_serde::from_slice`]'s return values on failure.
    pub fn deserialize<ValueType: DeserializeOwned>(data: &[u8]) -> Result<ValueType, Error> {
        let msgpack_data = zstd_decompress(data)?;
        rmp_serde::from_slice(&msgpack_data).map_err(Into::into)
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

/// Decompresses a single-pass zstd frame.
///
/// # Returns
///
/// The decompressed bytes on success.
///
/// # Errors
///
/// Returns [`Error::Zstd`] if the frame is invalid, does not declare its decompressed size, cannot
/// fit in memory, or decompression fails.
fn zstd_decompress(src: &[u8]) -> Result<Vec<u8>, Error> {
    // SAFETY: `src` is valid for `src.len()` readable bytes (including when empty), and
    // `ZSTD_getFrameContentSize` does not retain the pointer.
    let content_size =
        unsafe { ZSTD_getFrameContentSize(src.as_ptr().cast::<c_void>(), src.len()) };
    if ZSTD_CONTENTSIZE_ERROR == content_size {
        return Err(Error::Zstd("invalid zstd frame".to_string()));
    }
    if ZSTD_CONTENTSIZE_UNKNOWN == content_size {
        return Err(Error::Zstd(
            "zstd frame does not declare its decompressed size".to_string(),
        ));
    }

    let content_size = usize::try_from(content_size)
        .map_err(|_| Error::Zstd("zstd frame is too large for this platform".to_string()))?;
    let mut dst = Vec::new();
    dst.try_reserve_exact(content_size)
        .map_err(|error| Error::Zstd(format!("cannot allocate zstd output buffer: {error}")))?;
    dst.resize(content_size, 0);

    // SAFETY: `dst` is valid for `dst.len()` writable bytes, `src` is valid for `src.len()`
    // readable bytes (including when empty), and `ZSTD_decompress` does not retain either pointer.
    let decompressed_size = unsafe {
        ZSTD_decompress(
            dst.as_mut_ptr().cast::<c_void>(),
            dst.len(),
            src.as_ptr().cast::<c_void>(),
            src.len(),
        )
    };
    if 0 != ZSTD_isError(decompressed_size) {
        return Err(zstd_error(decompressed_size));
    }

    dst.truncate(decompressed_size);
    Ok(dst)
}

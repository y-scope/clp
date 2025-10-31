use std::io::Write;

use brotli::CompressorWriter;
use serde::Serialize;

use crate::Error;

/// Namespace for brotli compressed msgpack utils.
pub struct BrotliMsgpack {}

impl BrotliMsgpack {
    /// Serialize a value to a Brotli-compressed `MessagePack` byte sequence.
    ///
    /// # Return
    /// A vector of bytes containing the serialized byte sequence.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`rmp_serde::to_vec_named`]'s errors on failure.
    /// * Forwards [`std::io::Write::write_all`]'s errors on failure.
    pub fn serialize<T: Serialize + ?Sized>(val: &T) -> Result<Vec<u8>, Error> {
        let msgpack_data = rmp_serde::to_vec_named(val)?;
        let mut brotli_compressor = CompressorWriter::new(Vec::new(), 4096, 5, 22);
        brotli_compressor.write_all(&msgpack_data)?;
        Ok(brotli_compressor.into_inner())
    }
}

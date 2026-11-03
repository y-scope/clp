//! Encoders for the frames of the wire protocol, for tests.

/// # Returns
///
/// The bytes of a handshake with the given fields.
#[must_use]
pub fn encode_handshake(
    version: u64,
    session_token: &str,
    task_index: u64,
    archive_id: &str,
) -> Vec<u8> {
    let mut bytes = rmp::encode::ByteBuf::new();
    rmp::encode::write_array_len(&mut bytes, 4);
    rmp::encode::write_uint(&mut bytes, version);
    rmp::encode::write_str(&mut bytes, session_token);
    rmp::encode::write_uint(&mut bytes, task_index);
    rmp::encode::write_str(&mut bytes, archive_id);
    bytes.into_vec()
}

/// # Returns
///
/// The bytes of a result frame with the given fields.
#[must_use]
pub fn encode_result(result_index: u64, timestamp_millisecs: i64, message: &str) -> Vec<u8> {
    let mut bytes = rmp::encode::ByteBuf::new();
    rmp::encode::write_array_len(&mut bytes, 3);
    rmp::encode::write_uint(&mut bytes, result_index);
    rmp::encode::write_sint(&mut bytes, timestamp_millisecs);
    rmp::encode::write_str(&mut bytes, message);
    bytes.into_vec()
}

/// # Returns
///
/// The bytes of a trailer counting `num_results` results.
#[must_use]
pub fn encode_trailer(num_results: u64) -> Vec<u8> {
    let mut bytes = rmp::encode::ByteBuf::new();
    rmp::encode::write_array_len(&mut bytes, 1);
    rmp::encode::write_uint(&mut bytes, num_results);
    bytes.into_vec()
}

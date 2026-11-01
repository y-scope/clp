//! Encoders for the messages of the wire protocol, for tests.

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
    let Ok(_) = rmp::encode::write_array_len(&mut bytes, 4);
    let Ok(_) = rmp::encode::write_uint(&mut bytes, version);
    let Ok(()) = rmp::encode::write_str(&mut bytes, session_token);
    let Ok(_) = rmp::encode::write_uint(&mut bytes, task_index);
    let Ok(()) = rmp::encode::write_str(&mut bytes, archive_id);
    bytes.into_vec()
}

/// # Returns
///
/// The bytes of a result frame with the given fields.
#[must_use]
pub fn encode_result(result_index: u64, timestamp_millisecs: i64, message: &str) -> Vec<u8> {
    let mut bytes = rmp::encode::ByteBuf::new();
    let Ok(_) = rmp::encode::write_array_len(&mut bytes, 3);
    let Ok(_) = rmp::encode::write_uint(&mut bytes, result_index);
    let Ok(_) = rmp::encode::write_sint(&mut bytes, timestamp_millisecs);
    let Ok(()) = rmp::encode::write_str(&mut bytes, message);
    bytes.into_vec()
}

/// # Returns
///
/// The bytes of a trailer counting `num_results` results.
#[must_use]
pub fn encode_trailer(num_results: u64) -> Vec<u8> {
    let mut bytes = rmp::encode::ByteBuf::new();
    let Ok(_) = rmp::encode::write_array_len(&mut bytes, 1);
    let Ok(_) = rmp::encode::write_uint(&mut bytes, num_results);
    bytes.into_vec()
}

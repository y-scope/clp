//! Encoders for the messages of the wire protocol, for tests.

/// # Returns
///
/// The bytes of a handshake with the given fields.
///
/// # Panics
///
/// Panics if writing to the `Vec` fails. This should never happen since writing to a `Vec` can't
/// fail.
#[must_use]
pub fn encode_handshake(
    version: u64,
    session_token: &str,
    task_index: u64,
    archive_id: &str,
) -> Vec<u8> {
    let mut bytes = Vec::new();
    rmp::encode::write_array_len(&mut bytes, 4).expect("writing to a `Vec` shouldn't fail");
    rmp::encode::write_uint(&mut bytes, version).expect("writing to a `Vec` shouldn't fail");
    rmp::encode::write_str(&mut bytes, session_token).expect("writing to a `Vec` shouldn't fail");
    rmp::encode::write_uint(&mut bytes, task_index).expect("writing to a `Vec` shouldn't fail");
    rmp::encode::write_str(&mut bytes, archive_id).expect("writing to a `Vec` shouldn't fail");
    bytes
}

/// # Returns
///
/// The bytes of a result frame with the given fields.
///
/// # Panics
///
/// Panics if writing to the `Vec` fails. This should never happen since writing to a `Vec` can't
/// fail.
#[must_use]
pub fn encode_result(result_index: u64, timestamp_millisecs: i64, message: &str) -> Vec<u8> {
    let mut bytes = Vec::new();
    rmp::encode::write_array_len(&mut bytes, 3).expect("writing to a `Vec` shouldn't fail");
    rmp::encode::write_uint(&mut bytes, result_index).expect("writing to a `Vec` shouldn't fail");
    rmp::encode::write_sint(&mut bytes, timestamp_millisecs)
        .expect("writing to a `Vec` shouldn't fail");
    rmp::encode::write_str(&mut bytes, message).expect("writing to a `Vec` shouldn't fail");
    bytes
}

/// # Returns
///
/// The bytes of a trailer counting `num_results` results.
///
/// # Panics
///
/// Panics if writing to the `Vec` fails. This should never happen since writing to a `Vec` can't
/// fail.
#[must_use]
pub fn encode_trailer(num_results: u64) -> Vec<u8> {
    let mut bytes = Vec::new();
    rmp::encode::write_array_len(&mut bytes, 1).expect("writing to a `Vec` shouldn't fail");
    rmp::encode::write_uint(&mut bytes, num_results).expect("writing to a `Vec` shouldn't fail");
    bytes
}

#ifndef GLT_FFI_IR_STREAM_ENCODING_METHODS_HPP
#define GLT_FFI_IR_STREAM_ENCODING_METHODS_HPP

#include <string_view>
#include <vector>

#include "../../ir/types.hpp"
#include "../encoding_methods.hpp"

namespace glt::ffi::ir_stream {
namespace eight_byte_encoding {
/**
 * Serializes the preamble for the eight-byte encoding IR stream
 * @param timestamp_pattern
 * @param timestamp_pattern_syntax
 * @param time_zone_id
 * @param ir_buf
 * @return true on success, false otherwise
 */
bool serialize_preamble(
        std::string_view timestamp_pattern,
        std::string_view timestamp_pattern_syntax,
        std::string_view time_zone_id,
        std::vector<int8_t>& ir_buf
);

/**
 * Serializes the given log event into the eight-byte encoding IR stream
 * @param timestamp
 * @param message
 * @param logtype
 * @param ir_buf
 * @return true on success, false otherwise
 */
bool serialize_log_event(
        ir::epoch_time_ms_t timestamp,
        std::string_view message,
        std::string& logtype,
        std::vector<int8_t>& ir_buf
);
}  // namespace eight_byte_encoding

namespace four_byte_encoding {
/**
 * Serializes the preamble for the four-byte encoding IR stream
 * @param timestamp_pattern
 * @param timestamp_pattern_syntax
 * @param time_zone_id
 * @param reference_timestamp
 * @param ir_buf
 * @return true on success, false otherwise
 */
bool serialize_preamble(
        std::string_view timestamp_pattern,
        std::string_view timestamp_pattern_syntax,
        std::string_view time_zone_id,
        ir::epoch_time_ms_t reference_timestamp,
        std::vector<int8_t>& ir_buf
);

/**
 * Serializes the given log event into the four-byte encoding IR stream
 * @param timestamp_delta
 * @param message
 * @param logtype
 * @param ir_buf
 * @return true on success, false otherwise
 */
bool serialize_log_event(
        ir::epoch_time_ms_t timestamp_delta,
        std::string_view message,
        std::string& logtype,
        std::vector<int8_t>& ir_buf
);

/**
 * Serializes the given message into the four-byte encoding IR stream
 * delta
 * @param message
 * @param logtype
 * @param ir_buf
 * @return true on success, false otherwise
 */
bool serialize_message(std::string_view message, std::string& logtype, std::vector<int8_t>& ir_buf);

/**
 * Serializes the given timestamp delta into the four-byte encoding IR stream
 * @param timestamp_delta
 * @param ir_buf
 * @return true on success, false otherwise
 */
bool serialize_timestamp(ir::epoch_time_ms_t timestamp_delta, std::vector<int8_t>& ir_buf);
}  // namespace four_byte_encoding
}  // namespace glt::ffi::ir_stream

#endif  // GLT_FFI_IR_STREAM_ENCODING_METHODS_HPP

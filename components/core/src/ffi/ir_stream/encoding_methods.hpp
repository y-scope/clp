#ifndef FFI_IR_STREAM_ENCODING_METHODS_HPP
#define FFI_IR_STREAM_ENCODING_METHODS_HPP

// C++ standard libraries
#include <string_view>
#include <vector>

// Project headers
#include "../encoding_methods.hpp"

namespace ffi::ir_stream {
    namespace eight_byte_encoding {
        /**
         * Encodes the preamble for the eight-byte encoding IR stream
         * @param timestamp_pattern
         * @param timestamp_pattern_syntax
         * @param time_zone_id
         * @param ir_buf
         * @return true on success, false otherwise
         */
        bool encode_preamble (std::string_view timestamp_pattern,
                              std::string_view timestamp_pattern_syntax,
                              std::string_view time_zone_id, std::vector<int8_t>& ir_buf);

        /**
         * Encodes the given message into the eight-byte encoding IR stream
         * @param timestamp
         * @param message
         * @param logtype
         * @param ir_buf
         * @return true on success, false otherwise
         */
        bool encode_message (epoch_time_ms_t timestamp, std::string_view message,
                             std::string& logtype, std::vector<int8_t>& ir_buf);
    }

    namespace four_byte_encoding {
        /**
         * Encodes the preamble for the four-byte encoding IR stream
         * @param timestamp_pattern
         * @param timestamp_pattern_syntax
         * @param time_zone_id
         * @param reference_timestamp
         * @param ir_buf
         * @return true on success, false otherwise
         */
        bool encode_preamble (std::string_view timestamp_pattern,
                              std::string_view timestamp_pattern_syntax,
                              std::string_view time_zone_id, epoch_time_ms_t reference_timestamp,
                              std::vector<int8_t>& ir_buf);

        /**
         * Encodes the given message into the four-byte encoding IR stream
         * @param timestamp_delta
         * @param message
         * @param logtype
         * @param ir_buf
         * @return true on success, false otherwise
         */
        bool encode_message (epoch_time_ms_t timestamp_delta, std::string_view message,
                             std::string& logtype, std::vector<int8_t>& ir_buf);

        /**
         * Encodes the given message into the four-byte encoding IR stream
         * without encoding timestamp delta
         * @param message
         * @param logtype
         * @param ir_buf
         * @return true on success, false otherwise
         */
        bool encode_message (std::string_view message, std::string& logtype,
                             std::vector<int8_t>& ir_buf);

        /**
         * Encodes the given timestamp delta into the four-byte encoding IR
         * stream
         * @param timestamp_delta
         * @param ir_buf
         * @return true on success, false otherwise
         */
        bool encode_timestamp (epoch_time_ms_t timestamp_delta, std::vector<int8_t>& ir_buf);
    }
}

#endif //FFI_IR_STREAM_ENCODING_METHODS_HPP

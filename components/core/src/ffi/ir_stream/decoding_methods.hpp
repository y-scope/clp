#ifndef FFI_IR_STREAM_DECODING_METHODS_HPP
#define FFI_IR_STREAM_DECODING_METHODS_HPP

// C++ standard libraries
#include <string_view>
#include <vector>

// Project headers
#include "../encoding_methods.hpp"


namespace ffi::ir_stream {
    using encoded_tag_t = uint8_t;

    typedef enum {
        ErrorCode_Success,
        ErrorCode_Corrupted_IR,
        ErrorCode_InComplete_IR,
        ErrorCode_Unsupported_Version,
        ErrorCode_End_of_IR
    } IR_ErrorCode;

    IR_ErrorCode get_encoding_type(const std::vector<int8_t>& ir_buf, size_t& cursor_pos, bool& is_compact);

    namespace eight_byte_encoding {
        typedef struct {
            std::string timestamp_pattern;
            std::string timestamp_pattern_syntax;
            std::string time_zone_id;
        } TimestampInfo;

        /**
         * decodes the preamble for the eight-byte encoding IR stream
         * @param timestamp_pattern
         * @param timestamp_pattern_syntax
         * @param time_zone_id
         * @param ir_buf
         * @return true on success, false otherwise
         * Also return the ending position of preamble
         */
        IR_ErrorCode decode_preamble (std::vector<int8_t>& ir_buf,
                                      TimestampInfo& ts_info,
                                      size_t& ending_pos);

        /**
         * decodes the first message in the given eight-byte encoding IR stream.
         * if the IR stream is incomplete, return false.
         * else, return the ending position of the IR stream.
         * @param ts_info
         * @param ir_buf
         * @param message
         * @param timestamp
         * @param ending_pos
         * @return true on success, false otherwise
         */
        IR_ErrorCode decode_next_message (const TimestampInfo& ts_info,
                                          const std::vector<int8_t>& ir_buf,
                                          std::string& message,
                                          epoch_time_ms_t& timestamp,
                                          size_t& ending_pos);

    }

    namespace four_byte_encoding {

        typedef struct {
            std::string timestamp_pattern;
            std::string timestamp_pattern_syntax;
            std::string time_zone_id;
        } TimestampInfo;

        /**
         * decodes the preamble for the eight-byte encoding IR stream
         * @param timestamp_pattern
         * @param timestamp_pattern_syntax
         * @param time_zone_id
         * @param ir_buf
         * @return true on success, false otherwise
         * Also return the ending position of preamble
         */
        bool decode_preamble (std::vector<int8_t>& ir_buf,
                              TimestampInfo& ts_info,
                              epoch_time_ms_t& reference_ts,
                              size_t& ending_pos);

        /**
         * decodes the first message in the given eight-byte encoding IR stream.
         * if the IR stream is incomplete, return false.
         * else, return the ending position of the IR stream.
         * @param timestamp
         * @param message
         * @param logtype
         * @param ir_buf
         * @return true on success, false otherwise
         */
        bool decode_message (const TimestampInfo& ts_info,
                             const std::vector<int8_t>& ir_buf,
                             epoch_time_ms_t& reference_ts,
                             std::string& message,
                             epoch_time_ms_t& timestamp,
                             size_t& ending_pos);
    }
}

#endif //DECODING_METHODS_HPP

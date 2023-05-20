#ifndef FFI_IR_STREAM_DECODING_METHODS_HPP
#define FFI_IR_STREAM_DECODING_METHODS_HPP

// C++ standard libraries
#include <string_view>
#include <vector>

// Project headers
#include "../encoding_methods.hpp"
#include "../../BufferReader.hpp"
namespace ffi::ir_stream {
    using encoded_tag_t = uint8_t;
    /**
     * Struct to hold the timestamp info from the IR stream's metadata
     */
    struct TimestampInfo {
        std::string timestamp_pattern;
        std::string timestamp_pattern_syntax;
        std::string time_zone_id;
    };

    typedef enum {
        IRErrorCode_Success,
        IRErrorCode_Decode_Error,
        IRErrorCode_Eof,
        IRErrorCode_Corrupted_IR,
        IRErrorCode_Corrupted_Metadata,
        IRErrorCode_Incomplete_IR,
        IRErrorCode_Unsupported_Version,
    } IRErrorCode;

    /**
     * Decodes the encoding type for the encoded IR stream
     * @param ir_buf
     * @param is_four_bytes_encoding Returns the encoding type
     * @return ErrorCode_Success on success
     * @return ErrorCode_Corrupted_IR if ir_buf contains invalid IR
     * @return ErrorCode_Incomplete_IR if ir_buf doesn't contain enough data to
     * decode
     */
    IRErrorCode get_encoding_type (BufferReader& ir_buf, bool& is_four_bytes_encoding);

    namespace eight_byte_encoding {
        /**
         * Decodes the preamble for the eight-byte encoding IR stream.
         * @param ir_buf
         * @param ts_info Returns the timestamp info on success
         * @return IRErrorCode_Success on success
         * @return IRErrorCode_Corrupted_IR if ir_buf contains invalid IR
         * @return IRErrorCode_Incomplete_IR if ir_buf doesn't contain enough
         * data to decode
         * @return IRErrorCode_Unsupported_Version if the IR uses an unsupported
         * version
         * @return IRErrorCode_Corrupted_Metadata if the metadata cannot be
         * decoded
         */
        IRErrorCode decode_preamble (BufferReader& ir_buf, TimestampInfo& ts_info);

        /**
         * Decodes the next message for the eight-byte encoding IR stream.
         * @param ir_buf
         * @param message Returns the decoded message
         * @param timestamp Returns the decoded timestamp
         * @return ErrorCode_Success on success
         * @return ErrorCode_Corrupted_IR if ir_buf contains invalid IR
         * @return ErrorCode_Decode_Error if the encoded message cannot be
         * properly decoded
         * @return ErrorCode_Incomplete_IR if ir_buf doesn't contain enough data
         * to decode
         * @return ErrorCode_End_of_IR if the IR ends
         */
        IRErrorCode decode_next_message (BufferReader& ir_buf, std::string& message,
                                         epoch_time_ms_t& timestamp);
    }

    namespace four_byte_encoding {
        /**
         * Decodes the preamble for the four-byte encoding IR stream.
         * @param ir_buf
         * @param ts_info Returns the decoded timestamp info
         * @param reference_ts Returns the decoded reference timestamp
         * @return IRErrorCode_Success on success
         * @return IRErrorCode_Corrupted_IR if ir_buf contains invalid IR
         * @return IRErrorCode_Incomplete_IR if ir_buf doesn't contain enough
         * data to decode
         * @return IRErrorCode_Unsupported_Version if the IR has a version that
         * is not supported
         * @return IRErrorCode_Corrupted_Metadata if the metadata cannot be
         * decoded
         */
        IRErrorCode decode_preamble (BufferReader& ir_buf, TimestampInfo& ts_info,
                                     epoch_time_ms_t& reference_ts);

        /**
         * Decodes the next message for the four-byte encoding IR stream.
         * @param ir_buf
         * @param message Returns the decoded message
         * @param timestamp_delta Returns the decoded timestamp delta
         * @return ErrorCode_Success on success
         * @return ErrorCode_Corrupted_IR if ir_buf contains invalid IR
         * @return ErrorCode_Decode_Error if the encoded message cannot be
         * properly decoded
         * @return ErrorCode_Incomplete_IR if ir_buf doesn't contain enough data
         * to decode
         * @return ErrorCode_End_of_IR if the IR ends
         */
        IRErrorCode decode_next_message (BufferReader& ir_buf, std::string& message,
                                         epoch_time_ms_t& timestamp_delta);
    }
}

#endif //FFI_IR_STREAM_DECODING_METHODS_HPP

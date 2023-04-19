#ifndef FFI_IR_STREAM_DECODING_METHODS_HPP
#define FFI_IR_STREAM_DECODING_METHODS_HPP

// C++ standard libraries
#include <string_view>
#include <vector>

// Project headers
#include "../encoding_methods.hpp"

namespace ffi::ir_stream {
    using encoded_tag_t = uint8_t;

    // Class representing the IR buffer passed in by users
    class IRBuffer {
    public:

        IRBuffer (const int8_t* data, size_t size) : m_data(data),
                  m_size(size),
                  m_cursor_pos(0),
                  m_internal_cursor_pos(0)
        {}

        size_t cursor_pos () const { return m_cursor_pos; }
        void set_cursor_pos (size_t cursor_pos) { m_cursor_pos = cursor_pos; }

        // the following functions are only supposed to be used by the decoder
        void increment_internal_pos (size_t val) { m_internal_cursor_pos += val; }
        void init_internal_pos () { m_internal_cursor_pos = m_cursor_pos; }
        void commit_internal_pos () { m_cursor_pos = m_internal_cursor_pos; }
        const int8_t* internal_head() { return m_data + m_internal_cursor_pos; }

        bool size_overflow (size_t read_size) {
            return (m_internal_cursor_pos + read_size) > m_size;
        }


    private:
        const int8_t* const m_data;
        const size_t m_size;
        size_t m_cursor_pos;
        // Internal cursor position to help keeping
        // cursor pos on a decoding failure.
        size_t m_internal_cursor_pos;
    };

    typedef struct {
        std::string timestamp_pattern;
        std::string timestamp_pattern_syntax;
        std::string time_zone_id;
    } TimestampInfo;

    typedef enum {
        ErrorCode_Success,
        ErrorCode_Corrupted_IR,
        ErrorCode_InComplete_IR,
        ErrorCode_Unsupported_Version,
        ErrorCode_End_of_IR
    } IR_ErrorCode;

    /**
     * Decodes the encoding type for the encoded IR stream.
     * On success, return the encoding type by reference
     * @param ir_buf
     * @param is_four_bytes_encoding
     * @return ErrorCode_Success on success
     * @return ErrorCode_Corrupted_IR if ir_buf contains invalid IR
     * @return ErrorCode_InComplete_IR if input buffer doesn't contain enough data for decoding
     */
    IR_ErrorCode get_encoding_type(IRBuffer& ir_buf, bool& is_four_bytes_encoding);

    namespace eight_byte_encoding {

        /**
         * decodes the preamble for the eight-byte encoding IR stream.
         * On success, returns ts_info by reference and sets the cursor_pos
         * in ir_buf to be the end of preamble
         * @param ir_buf
         * @param ts_info
         * @return ErrorCode_Success on success
         * @return ErrorCode_Corrupted_IR if ir_buf contains invalid IR
         * @return ErrorCode_InComplete_IR if input buffer doesn't contain enough data for decoding
         * @return ErrorCode_Unsupported_Version is the IR has a version that is not supported
         */
        IR_ErrorCode decode_preamble (IRBuffer& ir_buf,
                                      TimestampInfo& ts_info);

        /**
         * decodes the next message for the eight-byte encoding IR stream.
         * On success, returns message and timestamp by reference and sets the
         * cursor_pos in ir_buf to be end of decoded message
         * @param ir_buf
         * @param message
         * @param timestamp
         * @return ErrorCode_Success on success
         * @return ErrorCode_Corrupted_IR if ir_buf contains invalid IR
         * @return ErrorCode_InComplete_IR if input buffer doesn't contain enough data for decoding
         * @return ErrorCode_End_of_IR if the IR ends
         */
        IR_ErrorCode decode_next_message (IRBuffer& ir_buf,
                                          std::string& message,
                                          epoch_time_ms_t& timestamp);

    }

    namespace four_byte_encoding {

        /**
         * decodes the preamble for the four-byte encoding IR stream.
         * On success, returns ts_info and reference_ts by reference
         * and sets cursor_pos in ir_buf to the be end of preamble
         * @param ir_buf
         * @param ts_info
         * @return ErrorCode_Success on success
         * @return ErrorCode_Corrupted_IR if ir_buf contains invalid IR
         * @return ErrorCode_InComplete_IR if input buffer doesn't contain enough data for decoding
         * @return ErrorCode_Unsupported_Version is the IR has a version that is not supported
         */
        IR_ErrorCode decode_preamble (IRBuffer& ir_buf,
                                      TimestampInfo& ts_info,
                                      epoch_time_ms_t& reference_ts);

        /**
         * decodes the next message for the four-byte encoding IR stream.
         * On success, returns message and ts_delta by reference and sets
         * cursor_pos in ir_buf to be the end of decoded message
         * @param ir_buf
         * @param message
         * @param ts_delta
         * @return ErrorCode_Success on success
         * @return ErrorCode_Corrupted_IR if ir_buf contains invalid IR
         * @return ErrorCode_InComplete_IR if input buffer doesn't contain enough data for decoding
         * @return ErrorCode_End_of_IR if the IR ends
         */
        IR_ErrorCode decode_next_message (IRBuffer& ir_buf,
                                          std::string& message,
                                          epoch_time_ms_t& ts_delta);
    }
}

#endif //FFI_IR_STREAM_DECODING_METHODS_HPP

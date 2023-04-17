#ifndef FFI_IR_STREAM_DECODING_METHODS_HPP
#define FFI_IR_STREAM_DECODING_METHODS_HPP

// C++ standard libraries
#include <string_view>
#include <vector>

// Project headers
#include "../encoding_methods.hpp"


namespace ffi::ir_stream {
    using encoded_tag_t = uint8_t;

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
        void commit_internal_pos () { m_cursor_pos = m_internal_cursor_pos; }
        void increment_internal_pos (size_t val) { m_internal_cursor_pos += val; }
        void init_internal_pos () { m_internal_cursor_pos = m_cursor_pos; }

        bool size_overflow (size_t read_size) {
            return (m_internal_cursor_pos + read_size) > m_size;
        }

        const int8_t* internal_head() { return m_data + m_internal_cursor_pos; }


    private:
        const int8_t* const m_data;
        const size_t m_size;
        size_t m_cursor_pos;
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

    IR_ErrorCode get_encoding_type(IRBuffer& ir_buf, bool& is_four_bytes_encoding);

    namespace eight_byte_encoding {

        /**
         * decodes the preamble for the eight-byte encoding IR stream
         * @param timestamp_pattern
         * @param timestamp_pattern_syntax
         * @param time_zone_id
         * @param ir_buf
         * @return true on success, false otherwise
         * Also return the ending position of preamble
         */
        IR_ErrorCode decode_preamble (IRBuffer& ir_buf,
                                      TimestampInfo& ts_info);

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
        IR_ErrorCode decode_next_message (IRBuffer& ir_buf,
                                          std::string& message,
                                          epoch_time_ms_t& timestamp);

    }

    namespace four_byte_encoding {

        /**
         * decodes the preamble for the eight-byte encoding IR stream
         * @param timestamp_pattern
         * @param timestamp_pattern_syntax
         * @param time_zone_id
         * @param ir_buf
         * @return true on success, false otherwise
         * Also return the ending position of preamble
         */
        IR_ErrorCode decode_preamble (IRBuffer& ir_buf,
                                      TimestampInfo& ts_info,
                                      epoch_time_ms_t& reference_ts);

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
        IR_ErrorCode decode_next_message (IRBuffer& ir_buf,
                                          std::string& message,
                                          epoch_time_ms_t& ts_delta);
    }
}

#endif //FFI_IR_STREAM_DECODING_METHODS_HPP

#include "decoding_methods.hpp"

// json
#include "../../../submodules/json/single_include/nlohmann/json.hpp"

// Project headers
#include "byteswap.hpp"
#include "protocol_constants.hpp"

using std::is_same_v;
using std::string;
using std::string_view;
using std::vector;

namespace ffi::ir_stream {
    /**
     * @tparam encoded_variable_t Type of the encoded variable
     * @param tag
     * @param is_encoded_var Returns true if tag is for an encoded variable (as
     * opposed to a dictionary variable)
     * @return Whether the tag is a variable tag
     */
    template <typename encoded_variable_t>
    static bool is_variable_tag (encoded_tag_t tag, bool& is_encoded_var);

    /**
     * Decodes an integer from ir_buf
     * @tparam integer_t Type of the integer to decode
     * @param ir_buf
     * @param value Returns the decoded integer
     * @return true on success, false if the ir_buf doesn't contain enough data
     * to decode
     */
    template <typename integer_t>
    static bool decode_int (IrBuffer& ir_buf, integer_t& value);

    /**
     * Decodes the next logtype string from ir_buf
     * @param ir_buf
     * @param encoded_tag
     * @param logtype Returns the logtype string
     * @return IRErrorCode_Success on success
     * @return IRErrorCode_Corrupted_IR if ir_buf contains invalid IR
     * @return IRErrorCode_Incomplete_IR if ir_buf doesn't contain enough data
     * to decode
     */
    static IRErrorCode parse_logtype (IrBuffer& ir_buf, encoded_tag_t encoded_tag,
                                      string_view& logtype);

    /**
     * Decodes the next dictionary-type variable string from ir_buf
     * @param ir_buf
     * @param encoded_tag
     * @param dict_var Returns the dictionary variable
     * @return IRErrorCode_Success on success
     * @return IRErrorCode_Corrupted_IR if ir_buf contains invalid IR
     * @return IRErrorCode_Incomplete_IR if input buffer doesn't contain enough
     * data to decode
     */
    static IRErrorCode parse_dictionary_var (IrBuffer& ir_buf, encoded_tag_t encoded_tag,
                                             string_view& dict_var);

    /**
     * Parses the next timestamp from ir_buf
     * @tparam encoded_variable_t Type of the encoded variable
     * @param ir_buf
     * @param encoded_tag
     * @param ts Returns the timestamp delta if
     * encoded_variable_t == four_byte_encoded_variable_t or the actual
     * timestamp if encoded_variable_t == eight_byte_encoded_variable_t
     * @return IRErrorCode_Success on success
     * @return IRErrorCode_Corrupted_IR if ir_buf contains invalid IR
     * @return IRErrorCode_Incomplete_IR if ir_buf doesn't contain enough data
     * to decode
     */
    template <typename encoded_variable_t>
    IRErrorCode parse_timestamp (IrBuffer& ir_buf, encoded_tag_t encoded_tag, epoch_time_ms_t& ts);

    /**
     * Extracts timestamp info from the JSON metadata and stores it into ts_info
     * @param metadata_json The JSON metadata
     * @param ts_info Returns the timestamp info
     */
    static void set_timestamp_info (const nlohmann::json& metadata_json, TimestampInfo& ts_info);

    /**
     * Decodes the next encoded message from ir_buf
     * @tparam encoded_variable_t Type of the encoded variable
     * @param ir_buf
     * @param message Returns the decoded message
     * @param timestamp Returns the timestamp delta if
     * encoded_variable_t == four_byte_encoded_variable_t or the actual
     * timestamp if encoded_variable_t == eight_byte_encoded_variable_t
     * @return IRErrorCode_Success on success
     * @return IRErrorCode_Corrupted_IR if ir_buf contains invalid IR
     * @return IRErrorCode_Decode_Error if the encoded message cannot be
     * properly decoded
     * @return IRErrorCode_Incomplete_IR if ir_buf doesn't contain enough data
     * to decode
     */
    template <typename encoded_variable_t>
    static IRErrorCode generic_decode_next_message (IrBuffer& ir_buf, string& message,
                                                    epoch_time_ms_t& timestamp);

    /**
     * Decodes the JSON metadata from the ir_buf
     * @param ir_buf
     * @param json_metadata Returns the JSON metadata
     * @return IRErrorCode_Success on success
     * @return IRErrorCode_Corrupted_IR if ir_buf contains invalid IR
     * @return IRErrorCode_Incomplete_IR if ir_buf doesn't contain enough data
     * to decode
     */
    static IRErrorCode read_json_metadata (IrBuffer& ir_buf, string_view& json_metadata);

    /**
     * Decodes the message from the given logtype, encoded variables, and
     * dictionary variables. This function properly handles the escaped
     * placeholder in the logtype, as opposed to the decode_message defined in
     * encoding_methods that doesn't handle escaped placeholder for simplicity.
     * @tparam encoded_variable_t Type of the encoded variable
     * @param logtype
     * @param encoded_vars
     * @param dictionary_vars
     * @return The decoded message
     * @throw ErrorCode_Corrupt if the message can't be decoded
     */
    template <typename encoded_variable_t>
    static std::string decode_message (
        string_view logtype,
        vector<encoded_variable_t> encoded_vars,
        vector<string_view> dictionary_vars
    );

    bool IrBuffer::try_read (string_view& str_view, size_t read_size) {
        if (read_will_overflow(read_size)) {
            return false;
        }
        str_view = string_view(reinterpret_cast<const char*>(m_data + m_internal_cursor_pos),
                               read_size);
        m_internal_cursor_pos += read_size;
        return true;
    }

    template <typename integer_t>
    bool IrBuffer::try_read (integer_t& data) {
        return try_read(&data, sizeof(data));
    }

    bool IrBuffer::try_read (void* dest, size_t read_size) {
        if (read_will_overflow(read_size)) {
            return false;
        }
        memcpy(dest, (m_data + m_internal_cursor_pos), read_size);
        m_internal_cursor_pos += read_size;
        return true;
    }

    template <typename encoded_variable_t>
    static bool is_variable_tag (encoded_tag_t tag, bool& is_encoded_var) {
        static_assert(is_same_v<encoded_variable_t, eight_byte_encoded_variable_t> ||
                      is_same_v<encoded_variable_t, four_byte_encoded_variable_t>);

        if (tag == cProtocol::Payload::VarStrLenUByte ||
            tag == cProtocol::Payload::VarStrLenUShort ||
            tag == cProtocol::Payload::VarStrLenInt) {
            is_encoded_var = false;
            return true;
        }

        if constexpr (is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>) {
            if (tag == cProtocol::Payload::VarEightByteEncoding) {
                is_encoded_var = true;
                return true;
            }
        } else {
            if (tag == cProtocol::Payload::VarFourByteEncoding) {
                is_encoded_var = true;
                return true;
            }
        }
        return false;
    }

    template <typename integer_t>
    static bool decode_int (IrBuffer& ir_buf, integer_t& value) {
        integer_t value_small_endian;
        if (ir_buf.try_read(value_small_endian) == false) {
            return false;
        }

        constexpr auto read_size = sizeof(integer_t);
        static_assert(read_size == 1 || read_size == 2 || read_size == 4 || read_size == 8);
        if constexpr (read_size == 1) {
            value = value_small_endian;
        } else if constexpr (read_size == 2) {
            value = bswap_16(value_small_endian);
        } else if constexpr (read_size == 4) {
            value = bswap_32(value_small_endian);
        } else if constexpr (read_size == 8) {
            value = bswap_64(value_small_endian);
        }
        return true;
    }

    static IRErrorCode parse_logtype (IrBuffer& ir_buf, encoded_tag_t encoded_tag,
                                      string_view& logtype)
    {
        size_t logtype_length;
        if (encoded_tag == cProtocol::Payload::LogtypeStrLenUByte) {
            uint8_t length;
            if (false == decode_int(ir_buf, length)) {
                return IRErrorCode_Incomplete_IR;
            }
            logtype_length = length;
        } else if (encoded_tag == cProtocol::Payload::LogtypeStrLenUShort) {
            uint16_t length;
            if (false == decode_int(ir_buf, length)) {
                return IRErrorCode_Incomplete_IR;
            }
            logtype_length = length;
        } else if (encoded_tag == cProtocol::Payload::LogtypeStrLenInt) {
            int32_t length;
            if (false == decode_int(ir_buf, length)) {
                return IRErrorCode_Incomplete_IR;
            }
            logtype_length = length;
        } else {
            return IRErrorCode_Corrupted_IR;
        }

        if (ir_buf.try_read(logtype, logtype_length) == false) {
            return IRErrorCode_Incomplete_IR;
        }
        return IRErrorCode_Success;
    }

    static IRErrorCode parse_dictionary_var (IrBuffer& ir_buf, encoded_tag_t encoded_tag,
                                             string_view& dict_var) {
        // Decode variable's length
        size_t var_length;
        if (cProtocol::Payload::VarStrLenUByte == encoded_tag) {
            uint8_t length;
            if (false == decode_int(ir_buf, length)) {
                return IRErrorCode_Incomplete_IR;
            }
            var_length = length;
        } else if (cProtocol::Payload::VarStrLenUShort == encoded_tag) {
            uint16_t length;
            if (false == decode_int(ir_buf, length)) {
                return IRErrorCode_Incomplete_IR;
            }
            var_length = length;
        } else if (cProtocol::Payload::VarStrLenInt == encoded_tag) {
            int32_t length;
            if (false == decode_int(ir_buf, length)) {
                return IRErrorCode_Incomplete_IR;
            }
            var_length = length;
        } else {
            return IRErrorCode_Corrupted_IR;
        }

        // Read the dictionary variable
        if (false == ir_buf.try_read(dict_var, var_length)) {
            return IRErrorCode_Incomplete_IR;
        }

        return IRErrorCode_Success;
    }

    template <typename encoded_variable_t>
    IRErrorCode parse_timestamp (IrBuffer& ir_buf, encoded_tag_t encoded_tag, epoch_time_ms_t& ts)
    {
        static_assert(is_same_v<encoded_variable_t, eight_byte_encoded_variable_t> ||
                      is_same_v<encoded_variable_t, four_byte_encoded_variable_t>);

        if constexpr (is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>) {
            if (cProtocol::Payload::TimestampVal != encoded_tag) {
                return IRErrorCode_Corrupted_IR;
            }
            if (false == decode_int(ir_buf, ts)) {
                return IRErrorCode_Incomplete_IR;
            }
        } else {
            if (cProtocol::Payload::TimestampDeltaByte == encoded_tag) {
                int8_t ts_delta;
                if (false == decode_int(ir_buf, ts_delta)) {
                    return IRErrorCode_Incomplete_IR;
                }
                ts = ts_delta;
            } else if (cProtocol::Payload::TimestampDeltaShort == encoded_tag) {
                int16_t ts_delta;
                if (false == decode_int(ir_buf, ts_delta)) {
                    return IRErrorCode_Incomplete_IR;
                }
                ts = ts_delta;
            } else if (cProtocol::Payload::TimestampDeltaInt == encoded_tag) {
                int32_t ts_delta;
                if (false == decode_int(ir_buf, ts_delta)) {
                    return IRErrorCode_Incomplete_IR;
                }
                ts = ts_delta;
            } else {
                return IRErrorCode_Corrupted_IR;
            }
        }
        return IRErrorCode_Success;
    }

    static void set_timestamp_info (const nlohmann::json& metadata_json, TimestampInfo& ts_info) {
        ts_info.time_zone_id = metadata_json.at(cProtocol::Metadata::TimeZoneIdKey);
        ts_info.timestamp_pattern = metadata_json.at(cProtocol::Metadata::TimestampPatternKey);
        ts_info.timestamp_pattern_syntax =
                metadata_json.at(cProtocol::Metadata::TimestampPatternSyntaxKey);
    }

    template <typename encoded_variable_t>
    static IRErrorCode generic_decode_next_message (IrBuffer& ir_buf, string& message,
                                                    epoch_time_ms_t& timestamp)
    {
        ir_buf.init_internal_pos();

        encoded_tag_t encoded_tag;
        if (false == ir_buf.try_read(encoded_tag)) {
            return IRErrorCode_Incomplete_IR;
        }
        if (cProtocol::Eof == encoded_tag) {
            return IRErrorCode_Eof;
        }

        // Handle variables
        vector<encoded_variable_t> encoded_vars;
        vector<string_view> dict_vars;
        bool is_encoded_var;
        while (is_variable_tag<encoded_variable_t>(encoded_tag, is_encoded_var)) {
            if (is_encoded_var) {
                encoded_variable_t encoded_variable;
                if (false == decode_int(ir_buf, encoded_variable)) {
                    return IRErrorCode_Incomplete_IR;
                }
                encoded_vars.push_back(encoded_variable);
            } else {
                string_view var_str;
                if (auto error_code = parse_dictionary_var(ir_buf, encoded_tag, var_str);
                        IRErrorCode_Success != error_code)
                {
                    return error_code;
                }
                dict_vars.push_back(var_str);
            }
            if (false == ir_buf.try_read(encoded_tag)) {
                return IRErrorCode_Incomplete_IR;
            }
        }

        // Handle logtype
        string_view logtype;
        if (auto error_code = parse_logtype(ir_buf, encoded_tag, logtype);
                IRErrorCode_Success != error_code)
        {
            return error_code;
        }

        // NOTE: for the eight-byte encoding, the timestamp is the actual
        // timestamp; for the four-byte encoding, the timestamp is a timestamp
        // delta
        if (false == ir_buf.try_read(encoded_tag)) {
            return IRErrorCode_Incomplete_IR;
        }
        if (auto error_code = parse_timestamp<encoded_variable_t>(ir_buf, encoded_tag, timestamp);
                IRErrorCode_Success != error_code) {
            return error_code;
        }

        try {
            message = decode_message(logtype, encoded_vars, dict_vars);
        } catch (const EncodingException& e) {
            return IRErrorCode_Decode_Error;
        }

        ir_buf.commit_internal_pos();
        return IRErrorCode_Success;
    }

    static IRErrorCode read_json_metadata (IrBuffer& ir_buf, string_view& json_metadata) {
        encoded_tag_t encoded_tag;
        if (false == ir_buf.try_read(encoded_tag)) {
            return IRErrorCode_Incomplete_IR;
        }
        if (encoded_tag != cProtocol::Metadata::EncodingJson) {
            return IRErrorCode_Corrupted_IR;
        }

        // Read metadata length
        if (false == ir_buf.try_read(encoded_tag)) {
            return IRErrorCode_Incomplete_IR;
        }
        unsigned int metadata_length;
        switch (encoded_tag) {
            case cProtocol::Metadata::LengthUByte:
                uint8_t ubyte_res;
                if (false == decode_int(ir_buf, ubyte_res)) {
                    return IRErrorCode_Incomplete_IR;
                }
                metadata_length = ubyte_res;
                break;
            case cProtocol::Metadata::LengthUShort:
                uint16_t ushort_res;
                if (false == decode_int(ir_buf, ushort_res)) {
                    return IRErrorCode_Incomplete_IR;
                }
                metadata_length = ushort_res;
                break;
            default:
                return IRErrorCode_Corrupted_IR;
        }

        // Read the serialized metadata
        if (false == ir_buf.try_read(json_metadata, metadata_length)) {
            return IRErrorCode_Incomplete_IR;
        }
        return IRErrorCode_Success;
    }

    template <typename encoded_variable_t>
    static std::string decode_message (
            string_view logtype,
            vector <encoded_variable_t> encoded_vars,
            vector <string_view> dictionary_vars
    ) {
        std::string message;
        size_t encoded_vars_length = encoded_vars.size();
        size_t dict_vars_length = dictionary_vars.size();
        size_t next_static_text_begin_pos = 0;

        size_t dictionary_vars_ix = 0;
        size_t encoded_vars_ix = 0;
        for (size_t cur_pos = 0; cur_pos < logtype.length(); ++cur_pos) {
            auto c = logtype[cur_pos];
            if (enum_to_underlying_type(VariablePlaceholder::Float) == c) {
                message.append(logtype, next_static_text_begin_pos, cur_pos - next_static_text_begin_pos);
                next_static_text_begin_pos = cur_pos + 1;
                if (encoded_vars_ix >= encoded_vars_length) {
                    throw EncodingException(ErrorCode_Corrupt, __FILENAME__, __LINE__,
                                            cTooFewEncodedVarsErrorMessage);
                }
                message.append(decode_float_var(encoded_vars[encoded_vars_ix]));
                ++encoded_vars_ix;
            } else if (enum_to_underlying_type(VariablePlaceholder::Integer) == c) {
                message.append(logtype, next_static_text_begin_pos, cur_pos - next_static_text_begin_pos);
                next_static_text_begin_pos = cur_pos + 1;
                if (encoded_vars_ix >= encoded_vars_length) {
                    throw EncodingException(ErrorCode_Corrupt, __FILENAME__, __LINE__,
                                            cTooFewEncodedVarsErrorMessage);
                }
                message.append(decode_integer_var(encoded_vars[encoded_vars_ix]));
                ++encoded_vars_ix;
            } else if (enum_to_underlying_type(VariablePlaceholder::Dictionary) == c) {
                message.append(logtype, next_static_text_begin_pos, cur_pos - next_static_text_begin_pos);
                next_static_text_begin_pos = cur_pos + 1;
                if (dictionary_vars_ix >= dict_vars_length) {
                    throw EncodingException(ErrorCode_Corrupt, __FILENAME__, __LINE__,
                                            cTooFewDictionaryVarsErrorMessage);
                }
                message.append(dictionary_vars[dictionary_vars_ix]);
                ++dictionary_vars_ix;
            } else if (cVariablePlaceholderEscapeCharacter == c) {
                if (cur_pos == logtype.length() - 1) {
                    throw EncodingException(ErrorCode_Corrupt, __FILENAME__, __LINE__,
                                            cUnexpectedEscapeCharacterMessage);
                }
                message.append(logtype, next_static_text_begin_pos, cur_pos - next_static_text_begin_pos);

                // Skips the escape character
                next_static_text_begin_pos = cur_pos + 1;
                // The next character after the escape character is static text.
                // Increments the cur_pos by 1 to unconditionally append the
                // next character to the message as static text
                ++cur_pos;
            }
        }
        // Add remainder
        if (next_static_text_begin_pos < logtype.length()) {
            message.append(logtype, next_static_text_begin_pos);
        }

        return message;
    }

    IRErrorCode get_encoding_type (IrBuffer& ir_buf, bool& is_four_bytes_encoding) {
        ir_buf.init_internal_pos();

        int8_t buffer[cProtocol::MagicNumberLength];
        if (false == ir_buf.try_read(buffer, cProtocol::MagicNumberLength)) {
            return IRErrorCode_Incomplete_IR;
        }
        if (0 == memcmp(buffer, cProtocol::FourByteEncodingMagicNumber,
                        cProtocol::MagicNumberLength)) {
            is_four_bytes_encoding = true;
        } else if (0 == memcmp(buffer, cProtocol::EightByteEncodingMagicNumber,
                               cProtocol::MagicNumberLength)) {
            is_four_bytes_encoding = false;
        } else {
            return IRErrorCode_Corrupted_IR;
        }
        ir_buf.commit_internal_pos();
        return IRErrorCode_Success;
    }

    namespace four_byte_encoding {
        IRErrorCode decode_preamble (IrBuffer& ir_buf, TimestampInfo& ts_info,
                                     epoch_time_ms_t& reference_ts)
        {
            ir_buf.init_internal_pos();

            string_view json_metadata;
            if (auto error_code = read_json_metadata(ir_buf, json_metadata);
                    error_code != IRErrorCode_Success)
            {
                return error_code;
            }

            try {
                auto metadata_json = nlohmann::json::parse(json_metadata);
                const auto& version = metadata_json.at(cProtocol::Metadata::VersionKey);
                if (version != cProtocol::Metadata::VersionValue) {
                    return IRErrorCode_Unsupported_Version;
                }

                set_timestamp_info(metadata_json, ts_info);
                reference_ts = std::stoll(metadata_json.at(
                        cProtocol::Metadata::ReferenceTimestampKey).get<string>());
            } catch (const nlohmann::json::parse_error& e) {
                return IRErrorCode_Corrupted_Metadata;
            }

            ir_buf.commit_internal_pos();
            return IRErrorCode_Success;
        }

        IRErrorCode decode_next_message (IrBuffer& ir_buf, string& message,
                                         epoch_time_ms_t& timestamp_delta)
        {
            return generic_decode_next_message<four_byte_encoded_variable_t>(
                    ir_buf, message, timestamp_delta
            );
        }
    }

    namespace eight_byte_encoding {
        IRErrorCode decode_preamble (IrBuffer& ir_buf, TimestampInfo& ts_info) {
            ir_buf.init_internal_pos();

            string_view json_metadata;
            if (auto error_code = read_json_metadata(ir_buf, json_metadata);
                    error_code != IRErrorCode_Success)
            {
                return error_code;
            }
            try {
                auto metadata_json = nlohmann::json::parse(json_metadata);
                const auto& version = metadata_json.at(cProtocol::Metadata::VersionKey);
                if (version != cProtocol::Metadata::VersionValue) {
                    return IRErrorCode_Unsupported_Version;
                }

                set_timestamp_info(metadata_json, ts_info);
            } catch (const nlohmann::json::parse_error& e) {
                return IRErrorCode_Corrupted_Metadata;
            }

            ir_buf.commit_internal_pos();
            return IRErrorCode_Success;
        }

        IRErrorCode decode_next_message (IrBuffer& ir_buf, string& message,
                                         epoch_time_ms_t& timestamp)
        {
            return generic_decode_next_message<eight_byte_encoded_variable_t>(
                    ir_buf, message, timestamp
            );
        }
    }
}

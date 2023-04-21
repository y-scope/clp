#include "decoding_methods.hpp"

// json
#include "../../../submodules/json/single_include/nlohmann/json.hpp"

// Project headers
#include "byteswap.hpp"
#include "protocol_constants.hpp"

namespace ffi::ir_stream {
    bool IRBuffer::try_read_string (std::string_view& str_data, size_t read_size) {
        if (will_read_overflow(read_size)) {
            return false;
        }
        str_data = std::string_view((char*)(m_data + m_internal_cursor_pos), read_size);
        m_internal_cursor_pos += read_size;
        return true;
    }

    template <typename integer_t>
    bool IRBuffer::try_read (integer_t& data) {
        constexpr auto read_size = sizeof(integer_t);
        return try_read(&data, read_size);
    }

    bool IRBuffer::try_read (void* dest, size_t read_size) {
        if (will_read_overflow(read_size)) {
            return false;
        }
        memcpy(dest, (m_data + m_internal_cursor_pos), read_size);
        m_internal_cursor_pos += read_size;
        return true;
    }

    template <typename encoded_variable_t>
    static bool is_variable_tag (encoded_tag_t tag, bool& is_encoded_var) {
        static_assert(std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t> ||
                      std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>);

        if (tag == cProtocol::Payload::VarStrLenUByte ||
            tag == cProtocol::Payload::VarStrLenUShort ||
            tag == cProtocol::Payload::VarStrLenInt) {
            is_encoded_var = false;
            return true;
        }

        if constexpr (std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>) {
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
    static bool read_data_big_endian (IRBuffer& ir_buf, integer_t& data) {
        constexpr auto read_size = sizeof(integer_t);
        static_assert(read_size == 1 || read_size == 2 || read_size == 4 || read_size == 8);

        integer_t value_small_endian;
        if (ir_buf.try_read(value_small_endian) == false) {
            return false;
        }

        if constexpr (read_size == 1) {
            data = value_small_endian;
        } else if constexpr (read_size == 2) {
            data = bswap_16(value_small_endian);
        } else if constexpr (read_size == 4) {
            data = bswap_32(value_small_endian);
        } else if constexpr (read_size == 8) {
            data = bswap_64(value_small_endian);
        }
        return true;
    }

    static IRErrorCode
    get_logtype_length (IRBuffer& ir_buf, encoded_tag_t encoded_tag, size_t& logtype_length) {
        if (encoded_tag == cProtocol::Payload::LogtypeStrLenUByte) {
            uint8_t length;
            if (false == read_data_big_endian(ir_buf, length)) {
                return IRErrorCode_InComplete_IR;
            }
            logtype_length = length;
        } else if (encoded_tag == cProtocol::Payload::LogtypeStrLenUShort) {
            uint16_t length;
            if (false == read_data_big_endian(ir_buf, length)) {
                return IRErrorCode_InComplete_IR;
            }
            logtype_length = length;
        } else if (encoded_tag == cProtocol::Payload::LogtypeStrLenInt) {
            int32_t length;
            if (false == read_data_big_endian(ir_buf, length)) {
                return IRErrorCode_InComplete_IR;
            }
            logtype_length = length;
        } else {
            return IRErrorCode_Corrupted_IR;
        }
        return IRErrorCode_Success;
    }

    static IRErrorCode
    parse_logtype (IRBuffer& ir_buf, encoded_tag_t encoded_tag, std::string_view& logtype) {
        size_t log_length;
        if (IRErrorCode error_code = get_logtype_length(ir_buf, encoded_tag, log_length);
                IRErrorCode_Success != error_code) {
            return error_code;
        }
        if (ir_buf.try_read_string(logtype, log_length) == false) {
            return IRErrorCode_InComplete_IR;
        }
        return IRErrorCode_Success;
    }

    static IRErrorCode parse_dictionary_var (IRBuffer& ir_buf, encoded_tag_t encoded_tag,
                                             std::string_view& dict_var) {
        // decode the length of the variable from IR
        size_t var_length;
        if (cProtocol::Payload::VarStrLenUByte == encoded_tag) {
            uint8_t length;
            if (false == read_data_big_endian(ir_buf, length)) {
                return IRErrorCode_InComplete_IR;
            }
            var_length = length;
        } else if (cProtocol::Payload::VarStrLenUShort == encoded_tag) {
            uint16_t length;
            if (false == read_data_big_endian(ir_buf, length)) {
                return IRErrorCode_InComplete_IR;
            }
            var_length = length;
        } else if (cProtocol::Payload::VarStrLenInt == encoded_tag) {
            int32_t length;
            if (false == read_data_big_endian(ir_buf, length)) {
                return IRErrorCode_InComplete_IR;
            }
            var_length = length;
        } else {
            return IRErrorCode_Corrupted_IR;
        }

        // parse out the variable string
        if (false == ir_buf.try_read_string(dict_var, var_length)) {
            return IRErrorCode_InComplete_IR;
        }

        return IRErrorCode_Success;
    }

    /**
     * Parses the next timestamp from the IR. Returns the timestamp delta for
     * four_byte_encoded_variable_t and actual timestamp for
     * eight_byte_encoded_variable_t by reference.
     * @tparam encoded_variable_t Type of the encoded variable
     * @param ir_buf
     * @param encoded_tag
     * @param ts
     * @return
     */
    template <typename encoded_variable_t>
    IRErrorCode
    parse_timestamp (IRBuffer& ir_buf, encoded_tag_t encoded_tag, epoch_time_ms_t& ts) {
        static_assert(std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t> ||
                      std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>);

        if constexpr (std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>) {
            if (cProtocol::Payload::TimestampVal != encoded_tag) {
                return IRErrorCode_Corrupted_IR;
            }
            if (!read_data_big_endian(ir_buf, ts)) {
                return IRErrorCode_InComplete_IR;
            }
        } else {
            if (cProtocol::Payload::TimestampDeltaByte == encoded_tag) {
                int8_t ts_delta;
                if (!read_data_big_endian(ir_buf, ts_delta)) {
                    return IRErrorCode_InComplete_IR;
                }
                ts = ts_delta;
            } else if (cProtocol::Payload::TimestampDeltaShort == encoded_tag) {
                int16_t ts_delta;
                if (!read_data_big_endian(ir_buf, ts_delta)) {
                    return IRErrorCode_InComplete_IR;
                }
                ts = ts_delta;
            } else if (cProtocol::Payload::TimestampDeltaInt == encoded_tag) {
                int32_t ts_delta;
                if (!read_data_big_endian(ir_buf, ts_delta)) {
                    return IRErrorCode_InComplete_IR;
                }
                ts = ts_delta;
            } else {
                return IRErrorCode_Corrupted_IR;
            }
        }
        return IRErrorCode_Success;
    }

    /**
     * Extracts timestamp info from json metadata and stores into ts_info
     * @param metadata_json
     * @param ts_info
     */
    static void setTsInfo (const nlohmann::json& metadata_json, TimestampInfo& ts_info) {
        ts_info.time_zone_id = metadata_json.at(cProtocol::Metadata::TimeZoneIdKey);
        ts_info.timestamp_pattern = metadata_json.at(cProtocol::Metadata::TimestampPatternKey);
        ts_info.timestamp_pattern_syntax =
                metadata_json.at(cProtocol::Metadata::TimestampPatternSyntaxKey);
    }

    IRErrorCode get_encoding_type (IRBuffer& ir_buf, bool& is_four_bytes_encoding) {
        ir_buf.init_internal_pos();

        bool header_match = false;
        int8_t buffer[cProtocol::MagicNumberLength];
        if (false == ir_buf.try_read(buffer, cProtocol::MagicNumberLength)) {
            return IRErrorCode_InComplete_IR;
        }
        if (0 == memcmp(buffer, cProtocol::FourByteEncodingMagicNumber,
                        cProtocol::MagicNumberLength)) {
            is_four_bytes_encoding = true;
            header_match = true;
        } else if (0 == memcmp(buffer, cProtocol::EightByteEncodingMagicNumber,
                               cProtocol::MagicNumberLength)) {
            is_four_bytes_encoding = false;
            header_match = true;
        }
        if (false == header_match) {
            return IRErrorCode_Corrupted_IR;
        }
        ir_buf.commit_internal_pos();
        return IRErrorCode_Success;
    }

    template <typename encoded_variable_t>
    static IRErrorCode decode_next_message_general (IRBuffer& ir_buf,
                                                    std::string& message,
                                                    epoch_time_ms_t& timestamp) {
        ir_buf.init_internal_pos();
        encoded_tag_t encoded_tag;

        if (false == ir_buf.try_read(encoded_tag)) {
            return IRErrorCode_InComplete_IR;
        }
        if (cProtocol::Eof == encoded_tag) {
            return IRErrorCode_Eof;
        }

        std::vector<encoded_variable_t> encoded_vars;
        std::string all_dict_var_strings;
        std::vector<int32_t> dictionary_var_end_offsets;
        bool is_encoded_var;
        // handle variables
        while (is_variable_tag<encoded_variable_t>(encoded_tag, is_encoded_var)) {
            if (is_encoded_var) {
                encoded_variable_t encoded_variable;
                if (false == read_data_big_endian(ir_buf, encoded_variable)) {
                    return IRErrorCode_InComplete_IR;
                }
                encoded_vars.push_back(encoded_variable);
            } else {
                std::string_view var_str;
                if (IRErrorCode error_code = parse_dictionary_var(ir_buf, encoded_tag, var_str);
                        IRErrorCode_Success != error_code) {
                    return error_code;
                }
                all_dict_var_strings.append(var_str);
                dictionary_var_end_offsets.push_back(all_dict_var_strings.length());
            }
            if (false == ir_buf.try_read(encoded_tag)) {
                return IRErrorCode_InComplete_IR;
            }
        }

        // Handles logtype
        std::string_view logtype;
        if (IRErrorCode error_code = parse_logtype(ir_buf, encoded_tag, logtype);
                IRErrorCode_Success != error_code) {
            return error_code;
        }

        // Handles timestamp
        // Note, the timestamp is the actual timestamp for 8-bytes encoding and
        // the timestamp_delta for 4-bytes encoding
        if (false == ir_buf.try_read(encoded_tag)) {
            return IRErrorCode_InComplete_IR;
        }
        if (IRErrorCode error_code = parse_timestamp<encoded_variable_t>(ir_buf, encoded_tag,
                                                                         timestamp);
                IRErrorCode_Success != error_code) {
            return error_code;
        }

        message = decode_message(logtype, encoded_vars.data(),
                                 encoded_vars.size(), all_dict_var_strings,
                                 dictionary_var_end_offsets.data(),
                                 dictionary_var_end_offsets.size());

        ir_buf.commit_internal_pos();
        return IRErrorCode_Success;
    }

    IRErrorCode extract_json_metadata (IRBuffer& ir_buf,
                                       std::string_view& json_metadata) {
        encoded_tag_t encoded_tag;
        if (false == ir_buf.try_read(encoded_tag)) {
            return IRErrorCode_InComplete_IR;
        }
        if (encoded_tag != cProtocol::Metadata::EncodingJson) {
            return IRErrorCode_Corrupted_IR;
        }

        // read length
        if (false == ir_buf.try_read(encoded_tag)) {
            return IRErrorCode_InComplete_IR;
        }
        unsigned int metadata_length;
        switch (encoded_tag) {
            case cProtocol::Metadata::LengthUByte:
                uint8_t ubyte_res;
                if (false == read_data_big_endian(ir_buf, ubyte_res)) {
                    return IRErrorCode_InComplete_IR;
                }
                metadata_length = ubyte_res;
                break;
            case cProtocol::Metadata::LengthUShort:
                uint16_t ushort_res;
                if (false == read_data_big_endian(ir_buf, ushort_res)) {
                    return IRErrorCode_InComplete_IR;
                }
                metadata_length = ushort_res;
                break;
            default:
                return IRErrorCode_Corrupted_IR;
        }

        // extract the json contents
        if (false == ir_buf.try_read_string(json_metadata, metadata_length)) {
            return IRErrorCode_InComplete_IR;
        }
        return IRErrorCode_Success;
    }

    namespace four_byte_encoding {
        IRErrorCode decode_preamble (IRBuffer& ir_buf,
                                     TimestampInfo& ts_info,
                                     epoch_time_ms_t& reference_ts) {
            ir_buf.init_internal_pos();

            std::string_view json_metadata;
            if (IRErrorCode error_code = extract_json_metadata(ir_buf, json_metadata);
                    error_code != IRErrorCode_Success) {
                return error_code;
            }

            try {
                auto metadata_json = nlohmann::json::parse(json_metadata);
                std::string version = metadata_json.at(cProtocol::Metadata::VersionKey);
                if (version != cProtocol::Metadata::VersionValue) {
                    return IRErrorCode_Unsupported_Version;
                }

                setTsInfo(metadata_json, ts_info);
                reference_ts = std::stoll(metadata_json.at(
                        cProtocol::Metadata::ReferenceTimestampKey).get<std::string>());
            } catch (const nlohmann::json::parse_error& e) {
                return IRErrorCode_Corrupted_Metadata;
            }

            ir_buf.commit_internal_pos();
            return IRErrorCode_Success;
        }

        IRErrorCode decode_next_message (IRBuffer& ir_buf,
                                         std::string& message,
                                         epoch_time_ms_t& timestamp_delta) {
            return decode_next_message_general<four_byte_encoded_variable_t>(ir_buf,
                                                                             message,
                                                                             timestamp_delta);
        }
    }

    namespace eight_byte_encoding {
        IRErrorCode decode_preamble (IRBuffer& ir_buf,
                                     TimestampInfo& ts_info) {
            ir_buf.init_internal_pos();

            std::string_view json_metadata;
            if (IRErrorCode error_code = extract_json_metadata(ir_buf, json_metadata);
                    error_code != IRErrorCode_Success) {
                return error_code;
            }
            try {
                auto metadata_json = nlohmann::json::parse(json_metadata);
                std::string version = metadata_json.at(cProtocol::Metadata::VersionKey);
                if (version != cProtocol::Metadata::VersionValue) {
                    return IRErrorCode_Unsupported_Version;
                }

                setTsInfo(metadata_json, ts_info);
            } catch (const nlohmann::json::parse_error& e) {
                return IRErrorCode_Corrupted_Metadata;
            }

            ir_buf.commit_internal_pos();
            return IRErrorCode_Success;
        }

        IRErrorCode decode_next_message (IRBuffer& ir_buf,
                                         std::string& message,
                                         epoch_time_ms_t& timestamp) {
            return decode_next_message_general<eight_byte_encoded_variable_t>(ir_buf,
                                                                              message,
                                                                              timestamp);
        }
    }
}
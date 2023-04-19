#include "decoding_methods.hpp"

// json
#include "../../../submodules/json/single_include/nlohmann/json.hpp"

// logging library
#include <spdlog/spdlog.h>

// Project headers
#include "byteswap.hpp"
#include "protocol_constants.hpp"

namespace ffi::ir_stream {

    template <typename encoded_variable_t>
    static bool is_variable_tag(encoded_tag_t tag, bool& is_encoded_var) {
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

    /**
     * reads the next tag byte from the ir_buf
     * On success, returns the tag by reference and increments
     * the internal cursor of if_buf
     * @param ir_buf
     * @param tag_byte
     * @return true on success, false if the ir_buf is out of data
     **/
    static bool try_read_tag (IRBuffer& ir_buf, encoded_tag_t& tag_byte) {
        // In case we have different tag size in the future
        constexpr size_t read_size = sizeof(encoded_tag_t) / sizeof(int8_t);
        if (ir_buf.size_overflow(read_size)) {
            return false;
        }
        tag_byte = ir_buf.internal_head()[0];
        ir_buf.increment_internal_pos(read_size);
        return true;
    }

    /**
     * reads a string of size = read_size from the ir_buf
     * On success, returns the string as string_view by reference
     * and increments the internal cursor of if_buf
     * @param ir_buf
     * @param str_data
     * @param read_size
     * @return true on success, false if the ir_buf doesn't contain enough data
     **/
    static bool try_read_string (IRBuffer& ir_buf, std::string_view& str_data, size_t read_size) {
        if (ir_buf.size_overflow(read_size)) {
            return false;
        }
        str_data = std::string_view((char*)ir_buf.internal_head(), read_size);
        ir_buf.increment_internal_pos(read_size);
        return true;
    }

    template <typename integer_t>
    static bool read_data_big_endian (IRBuffer& ir_buf, integer_t& data) {
        constexpr size_t read_size = sizeof(integer_t);
        static_assert(read_size == 1 || read_size == 2 || read_size == 4 || read_size == 8);

        integer_t value_small_endian;
        if(ir_buf.size_overflow(read_size)) {
            return false;
        }

        value_small_endian = *reinterpret_cast<const integer_t* const>(ir_buf.internal_head());

        if constexpr (read_size == 1) {
            data = value_small_endian;
        } else if constexpr (read_size == 2) {
            data = bswap_16(value_small_endian);
        } else if constexpr (read_size == 4) {
            data = bswap_32(value_small_endian);
        } else if constexpr (read_size == 8) {
            data = bswap_64(value_small_endian);
        }
        ir_buf.increment_internal_pos(read_size);
        return true;
    }

    static IR_ErrorCode get_logtype_length (IRBuffer& ir_buf, encoded_tag_t encoded_tag, size_t& logtype_length) {
        if(encoded_tag == cProtocol::Payload::LogtypeStrLenUByte) {
            uint8_t length;
            if (false == read_data_big_endian(ir_buf, length)) {
                return ErrorCode_InComplete_IR;
            }
            logtype_length = length;
        } else if (encoded_tag == cProtocol::Payload::LogtypeStrLenUShort) {
            uint16_t length;
            if (false == read_data_big_endian(ir_buf, length)) {
                return ErrorCode_InComplete_IR;
            }
            logtype_length = length;
        } else if (encoded_tag == cProtocol::Payload::LogtypeStrLenInt) {
            int32_t length;
            if (false == read_data_big_endian(ir_buf, length)) {
                return ErrorCode_InComplete_IR;
            }
            logtype_length = length;
        } else {
            SPDLOG_ERROR("Unexpected tag byte {}", encoded_tag);
            return ErrorCode_Corrupted_IR;
        }
        return ErrorCode_Success;
    }

    static IR_ErrorCode parse_log_type(IRBuffer& ir_buf, encoded_tag_t encoded_tag, std::string_view& logtype) {

        size_t log_length;
        if (IR_ErrorCode error_code = get_logtype_length(ir_buf, encoded_tag, log_length);
            ErrorCode_Success != error_code) {
            return error_code;
        }
        if (false == try_read_string(ir_buf, logtype, log_length)) {
            return ErrorCode_InComplete_IR;
        }
        return ErrorCode_Success;
    }


    IR_ErrorCode parse_dictionary_var(IRBuffer& ir_buf, encoded_tag_t encoded_tag, std::string_view& dict_var) {
        // decode the length of the variable from IR
        size_t var_length;
        if (cProtocol::Payload::VarStrLenUByte == encoded_tag) {
            uint8_t length;
            if (false == read_data_big_endian(ir_buf, length)) {
                return ErrorCode_InComplete_IR;
            }
            var_length = length;
        } else if (cProtocol::Payload::VarStrLenUShort == encoded_tag) {
            uint16_t length;
            if (false == read_data_big_endian(ir_buf, length)) {
                return ErrorCode_InComplete_IR;
            }
            var_length = length;
        } else if (cProtocol::Payload::VarStrLenInt == encoded_tag) {
            int32_t length;
            if (false == read_data_big_endian(ir_buf, length)) {
                return ErrorCode_InComplete_IR;
            }
            var_length = length;
        } else {
            SPDLOG_ERROR("Unexpected tag byte {}", encoded_tag);
            return ErrorCode_Corrupted_IR;
        }

        // parse out the variable with length = var_length
        if (false == try_read_string(ir_buf, dict_var, var_length)) {
            return ErrorCode_InComplete_IR;
        }

        return ErrorCode_Success;
    }

    // Parses the next timestamp from the IR
    // Returns the ts_delta for four_byte_encoded_variable_t and actual ts for eight_byte_encoded_variable_t
    template <typename encoded_variable_t>
    IR_ErrorCode parse_timestamp(IRBuffer& ir_buf, encoded_tag_t encoded_tag, epoch_time_ms_t& ts) {
        static_assert(std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t> ||
                      std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>);

        if constexpr (std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>) {
            if(cProtocol::Payload::TimestampVal != encoded_tag) {
                SPDLOG_ERROR("Unexpected timestamp tag {}", encoded_tag);
                return ErrorCode_Corrupted_IR;
            }
            if (!read_data_big_endian(ir_buf, ts)) {
                return ErrorCode_InComplete_IR;
            }
        } else {
            if(cProtocol::Payload::TimestampDeltaByte == encoded_tag) {
                int8_t ts_delta;
                if (!read_data_big_endian(ir_buf, ts_delta)) {
                    return ErrorCode_InComplete_IR;
                }
                ts = ts_delta;
            } else if (cProtocol::Payload::TimestampDeltaShort == encoded_tag) {
                int16_t ts_delta;
                if (!read_data_big_endian(ir_buf, ts_delta)) {
                    return ErrorCode_InComplete_IR;
                }
                ts = ts_delta;
            } else if (cProtocol::Payload::TimestampDeltaInt == encoded_tag) {
                int32_t ts_delta;
                if (!read_data_big_endian(ir_buf, ts_delta)) {
                    return ErrorCode_InComplete_IR;
                }
                ts = ts_delta;
            } else {
                SPDLOG_ERROR("Unexpected timestamp tag {}", encoded_tag);
                return ErrorCode_Corrupted_IR;
            }
        }
        return ErrorCode_Success;
    }

    IR_ErrorCode get_encoding_type(IRBuffer& ir_buf, bool& is_four_bytes_encoding) {

        ir_buf.init_internal_pos();
        bool header_match = false;
        if (ir_buf.size_overflow(cProtocol::MagicNumberLength)) {
            return ErrorCode_InComplete_IR;
        }
        if (0 == memcmp(ir_buf.internal_head(), cProtocol::FourByteEncodingMagicNumber, cProtocol::MagicNumberLength)) {
            is_four_bytes_encoding = true;
            header_match = true;
        } else if (0 == memcmp(ir_buf.internal_head(), cProtocol::EightByteEncodingMagicNumber, cProtocol::MagicNumberLength)) {
            is_four_bytes_encoding = false;
            header_match = true;
        }
        if (false == header_match) {
            SPDLOG_ERROR("Unrecognized encoding");
            return ErrorCode_Corrupted_IR;
        }
        ir_buf.increment_internal_pos(cProtocol::MagicNumberLength);
        ir_buf.commit_internal_pos();
        return ErrorCode_Success;
    }

    template <typename encoded_variable_t>
    static IR_ErrorCode decode_next_message_general (IRBuffer& ir_buf,
                                                     std::string& message,
                                                     epoch_time_ms_t& timestamp) {
        ir_buf.init_internal_pos();
        encoded_tag_t encoded_tag;

        if (false == try_read_tag(ir_buf, encoded_tag)) {
            return ErrorCode_InComplete_IR;
        }

        if (cProtocol::Eof == encoded_tag) {
            return ErrorCode_End_of_IR;
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
                    return ErrorCode_InComplete_IR;
                }
                encoded_vars.push_back(encoded_variable);
            } else {
                std::string_view var_str;
                if (IR_ErrorCode error_code = parse_dictionary_var(ir_buf, encoded_tag, var_str);
                    ErrorCode_Success != error_code) {
                    return error_code;
                }
                all_dict_var_strings.append(var_str);
                dictionary_var_end_offsets.push_back(all_dict_var_strings.length());
            }
            if (false == try_read_tag(ir_buf, encoded_tag)) {
                return ErrorCode_InComplete_IR;
            }
        }

        // now handle logtype
        std::string_view logtype;
        if (IR_ErrorCode error_code = parse_log_type(ir_buf, encoded_tag, logtype);
            ErrorCode_Success != error_code) {
            return error_code;
        }

        // now handle timestamp
        // this is different between 8 bytes and 4 bytes
        if (false == try_read_tag(ir_buf, encoded_tag)) {
            return ErrorCode_InComplete_IR;
        }
        if (IR_ErrorCode error_code = parse_timestamp<encoded_variable_t>(ir_buf, encoded_tag, timestamp);
            ErrorCode_Success != error_code) {
            return error_code;
        }
        // now decode message
        message = decode_message(logtype, encoded_vars.data(),
                                 encoded_vars.size(), all_dict_var_strings,
                                 dictionary_var_end_offsets.data(),
                                 dictionary_var_end_offsets.size());

        ir_buf.commit_internal_pos();
        return ErrorCode_Success;
    }

    // Check json encoding and return the encoded json as a string_view
    // by reference
    IR_ErrorCode extract_json_metadata(IRBuffer& ir_buf,
                                       std::string_view& json_metadata) {

        encoded_tag_t encoded_tag;
        if (false == try_read_tag(ir_buf, encoded_tag)) {
            return ErrorCode_InComplete_IR;
        }
        if (encoded_tag != cProtocol::Metadata::EncodingJson) {
            SPDLOG_ERROR("Unexpected encoding tag {}", encoded_tag);
            return ErrorCode_Corrupted_IR;
        }

        // read length
        if(false == try_read_tag(ir_buf, encoded_tag)) {
            return ErrorCode_InComplete_IR;
        }
        unsigned int metadata_length;
        switch(encoded_tag) {
            case cProtocol::Metadata::LengthUByte:
                uint8_t ubyte_res;
                if (false == read_data_big_endian(ir_buf, ubyte_res)) {
                    return ErrorCode_InComplete_IR;
                }
                metadata_length = ubyte_res;
                break;
            case cProtocol::Metadata::LengthUShort:
                uint16_t ushort_res;
                if (false == read_data_big_endian(ir_buf, ushort_res)) {
                    return ErrorCode_InComplete_IR;
                }
                metadata_length = ushort_res;
                break;
            default:
                SPDLOG_ERROR("Invalid Length Tag {}", encoded_tag);
                return ErrorCode_Corrupted_IR;
        }

        // read the json contents
        if (false == try_read_string(ir_buf, json_metadata, metadata_length)) {
            return ErrorCode_InComplete_IR;
        }
        return ErrorCode_Success;
    }

    namespace four_byte_encoding {
        IR_ErrorCode decode_preamble (IRBuffer& ir_buf,
                                      TimestampInfo& ts_info,
                                      epoch_time_ms_t& reference_ts) {
            ir_buf.init_internal_pos();

            std::string_view json_metadata;
            if (IR_ErrorCode error_code = extract_json_metadata(ir_buf, json_metadata); error_code != ErrorCode_Success) {
                return error_code;
            }

            nlohmann::basic_json metadata_json = nlohmann::json::parse(json_metadata);
            std::string version = metadata_json.at(cProtocol::Metadata::VersionKey);
            if (version != cProtocol::Metadata::VersionValue) {
                SPDLOG_ERROR("Deprecated version: {}", version);
                return ErrorCode_Unsupported_Version;
            }

            ts_info.time_zone_id = metadata_json.at(cProtocol::Metadata::TimeZoneIdKey);
            ts_info.timestamp_pattern = metadata_json.at(cProtocol::Metadata::TimestampPatternKey);
            ts_info.timestamp_pattern_syntax = metadata_json.at(cProtocol::Metadata::TimestampPatternSyntaxKey);
            reference_ts = std::stoll(metadata_json.at(cProtocol::Metadata::ReferenceTimestampKey).get<std::string>());

            ir_buf.commit_internal_pos();
            return ErrorCode_Success;
        }

        IR_ErrorCode decode_next_message (IRBuffer& ir_buf,
                                          std::string& message,
                                          epoch_time_ms_t& ts_delta) {
            return decode_next_message_general<four_byte_encoded_variable_t>(ir_buf,
                                                                             message,
                                                                             ts_delta);
        }
    }

    namespace eight_byte_encoding {
        IR_ErrorCode decode_preamble (IRBuffer& ir_buf,
                                      TimestampInfo& ts_info) {
            ir_buf.init_internal_pos();

            std::string_view json_metadata;
            if (IR_ErrorCode error_code = extract_json_metadata(ir_buf, json_metadata); error_code != ErrorCode_Success) {
                return error_code;
            }

            auto metadata_json = nlohmann::json::parse(json_metadata);
            std::string version = metadata_json.at(cProtocol::Metadata::VersionKey);
            if (version != cProtocol::Metadata::VersionValue) {
                SPDLOG_ERROR("Deprecated version: {}", version);
                return ErrorCode_Unsupported_Version;
            }

            ts_info.time_zone_id = metadata_json.at(cProtocol::Metadata::TimeZoneIdKey);
            ts_info.timestamp_pattern = metadata_json.at(cProtocol::Metadata::TimestampPatternKey);
            ts_info.timestamp_pattern_syntax = metadata_json.at(cProtocol::Metadata::TimestampPatternSyntaxKey);

            ir_buf.commit_internal_pos();
            return ErrorCode_Success;
        }

        IR_ErrorCode decode_next_message (IRBuffer& ir_buf,
                                          std::string& message,
                                          epoch_time_ms_t& timestamp) {

            return decode_next_message_general<eight_byte_encoded_variable_t>(ir_buf,
                                                                              message,
                                                                              timestamp);
        }
    }
}
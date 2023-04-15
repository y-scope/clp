#include "decoding_methods.hpp"

// json
#include "../../../submodules/json/single_include/nlohmann/json.hpp"

// logging library

// Project headers
#include "byteswap.hpp"
#include "protocol_constants.hpp"

using std::string_view;
using std::string;
using std::vector;

#include <spdlog/spdlog.h>

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

    // read next tag byte and increment read_pos by size of tag
    static bool try_read_tag (const std::vector<int8_t>& ir_buf, size_t& cursor_pos, encoded_tag_t& tag_byte) {
        // In case we have different tag size in the future
        constexpr size_t read_pos_incr = sizeof(encoded_tag_t) / sizeof(int8_t);
        if (cursor_pos >= ir_buf.size()) {
            return false;
        }
        tag_byte = ir_buf.at(cursor_pos);
        cursor_pos += read_pos_incr;
        return true;
    }

    static bool try_read_string (const std::vector<int8_t>& ir_buf, size_t& cursor_pos, std::string& str_data, size_t read_size) {
        if (ir_buf.size() < cursor_pos + read_size) {
            return false;
        }
        str_data = std::string((char*)(ir_buf.data() + cursor_pos), read_size);
        cursor_pos += read_size;
        return true;
    }

    template <typename integer_t>
    static bool read_data_big_endian (const std::vector<int8_t>& ir_buf, integer_t& data, size_t& cursor_pos) {
        constexpr size_t read_size = sizeof(integer_t);
        static_assert(read_size == 1 || read_size == 2 || read_size == 4 || read_size == 8);

        integer_t value_small_endian;
        if(ir_buf.size() < cursor_pos + read_size) {
            return false;
        }

        memcpy(&value_small_endian, ir_buf.data() + cursor_pos, read_size);

        if constexpr (read_size == 1) {
            data = value_small_endian;
        } else if constexpr (read_size == 2) {
            data = bswap_16(value_small_endian);
        } else if constexpr (read_size == 4) {
            data = bswap_32(value_small_endian);
        } else if constexpr (read_size == 8) {
            data = bswap_64(value_small_endian);
        }
        cursor_pos += read_size;
        return true;
    }

    static IR_ErrorCode get_logtype_length (const std::vector<int8_t>& ir_buf, encoded_tag_t encoded_tag, size_t& logtype_length, size_t& cursor_pos) {
        if(encoded_tag == ffi::ir_stream::cProtocol::Payload::LogtypeStrLenUByte) {
            uint8_t length;
            if (false == read_data_big_endian(ir_buf, length, cursor_pos)) {
                return ErrorCode_InComplete_IR;
            }
            logtype_length = length;
        } else if (encoded_tag == ffi::ir_stream::cProtocol::Payload::LogtypeStrLenUShort) {
            uint16_t length;
            if (false == read_data_big_endian(ir_buf, length, cursor_pos)) {
                return ErrorCode_InComplete_IR;
            }
            logtype_length = length;
        } else if (encoded_tag == ffi::ir_stream::cProtocol::Payload::LogtypeStrLenInt) {
            int32_t length;
            if (false == read_data_big_endian(ir_buf, length, cursor_pos)) {
                return ErrorCode_InComplete_IR;
            }
            logtype_length = length;
        } else {
            SPDLOG_ERROR("Unexpected tag byte {}\n", encoded_tag);
            return ErrorCode_Corrupted_IR;
        }
        return ErrorCode_Success;
    }

    static IR_ErrorCode parse_log_type(const std::vector<int8_t>& ir_buf, encoded_tag_t encoded_tag, std::string& logtype, size_t& cursor_pos) {

        size_t log_length;
        IR_ErrorCode error_code = get_logtype_length(ir_buf, encoded_tag, log_length, cursor_pos);
        if (ErrorCode_Success != error_code) {
            return error_code;
        }
        if (false == try_read_string(ir_buf, cursor_pos, logtype, log_length)) {
            return ErrorCode_InComplete_IR;
        }
        return ErrorCode_Success;
    }

    IR_ErrorCode parse_dictionary_var(const std::vector<int8_t>& ir_buf, encoded_tag_t encoded_tag, std::string& dict_var, size_t& cursor_pos) {
        size_t var_length;
        if (cProtocol::Payload::VarStrLenUByte == encoded_tag) {
            uint8_t length;
            if (false == read_data_big_endian(ir_buf, length, cursor_pos)) {
                return ErrorCode_InComplete_IR;
            }
            var_length = length;
        } else if (cProtocol::Payload::VarStrLenUShort == encoded_tag) {
            uint16_t length;
            if (false == read_data_big_endian(ir_buf, length, cursor_pos)) {
                return ErrorCode_InComplete_IR;
            }
            var_length = length;
        } else if (cProtocol::Payload::VarStrLenInt == encoded_tag) {
            int32_t length;
            if (false == read_data_big_endian(ir_buf, length, cursor_pos)) {
                return ErrorCode_InComplete_IR;
            }
            var_length = length;
        } else {
            SPDLOG_ERROR("Unexpected tag byte {}", encoded_tag);
            return ErrorCode_Corrupted_IR;
        }

        if (false == try_read_string(ir_buf, cursor_pos, dict_var, var_length)) {
            return ErrorCode_InComplete_IR;
        }

        return ErrorCode_Success;
    }

    // Need to think about how to handle reference timestamp later
    template <typename encoded_variable_t>
    IR_ErrorCode parse_timestamp(const std::vector<int8_t>& ir_buf, encoded_tag_t encoded_tag, epoch_time_ms_t& ts, size_t& cursor_pos) {
        static_assert(std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t> ||
                      std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>);

        if constexpr (std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>) {
            if(cProtocol::Payload::TimestampVal != encoded_tag) {
                SPDLOG_ERROR("Unexpected timestamp tag {}", encoded_tag);
                return ErrorCode_Corrupted_IR;
            }
            if (!read_data_big_endian(ir_buf, ts, cursor_pos)) {
                return ErrorCode_InComplete_IR;
            }
        } else {
            if(cProtocol::Payload::TimestampDeltaByte == encoded_tag) {
                int8_t ts_delta;
                if (!read_data_big_endian(ir_buf, ts_delta, cursor_pos)) {
                    return ErrorCode_InComplete_IR;
                }
                ts = ts_delta;
            } else if (cProtocol::Payload::TimestampDeltaShort == encoded_tag) {
                int16_t ts_delta;
                if (!read_data_big_endian(ir_buf, ts_delta, cursor_pos)) {
                    return ErrorCode_InComplete_IR;
                }
                ts = ts_delta;
            } else if (cProtocol::Payload::TimestampDeltaInt == encoded_tag) {
                int32_t ts_delta;
                if (!read_data_big_endian(ir_buf, ts_delta, cursor_pos)) {
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

    IR_ErrorCode get_encoding_type(const std::vector<int8_t>& ir_buf, size_t& cursor_pos, bool& is_four_bytes_encoding) {
        bool header_match = false;
        if (ir_buf.size() < cProtocol::MagicNumberLength) {
            return ErrorCode_InComplete_IR;
        }
        if (0 == memcmp(ir_buf.data(), cProtocol::FourByteEncodingMagicNumber, cProtocol::MagicNumberLength)) {
            is_four_bytes_encoding = true;
            header_match = true;
        } else if (0 == memcmp(ir_buf.data(), cProtocol::EightByteEncodingMagicNumber, cProtocol::MagicNumberLength)) {
            is_four_bytes_encoding = false;
            header_match = true;
        }
        if (false == header_match) {
            return ErrorCode_Corrupted_IR;
        }
        cursor_pos += cProtocol::MagicNumberLength;
        return ErrorCode_Success;
    }

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
    template <typename encoded_variable_t>
    static IR_ErrorCode decode_next_message_general (const std::vector<int8_t>& ir_buf,
                                                     std::string& message,
                                                     epoch_time_ms_t& timestamp,
                                                     size_t& ending_pos) {
        size_t cursor_pos = ending_pos;
        encoded_tag_t encoded_tag;

        if (false == try_read_tag(ir_buf, cursor_pos, encoded_tag)) {
            return ErrorCode_InComplete_IR;
        }

        if (cProtocol::Eof == encoded_tag) {
            // TODO: do we want to sanity check if the current tag is the last byte of ir_buf
            return ErrorCode_End_of_IR;
        }

        std::vector<encoded_variable_t> encoded_vars;
        std::string all_dict_var_strings;
        vector<int32_t> dictionary_var_end_offsets;
        IR_ErrorCode error_code;
        bool is_encoded_var;
        // handle variables
        while (is_variable_tag<encoded_variable_t>(encoded_tag, is_encoded_var)) {
            if (is_encoded_var) {
                encoded_variable_t encoded_variable;
                if (false == read_data_big_endian(ir_buf, encoded_variable, cursor_pos)) {
                    return ErrorCode_InComplete_IR;
                }
                encoded_vars.push_back(encoded_variable);
            } else {
                std::string var_str;
                error_code = parse_dictionary_var(ir_buf, encoded_tag, var_str, cursor_pos);
                if (ErrorCode_Success != error_code) {
                    return error_code;
                }
                all_dict_var_strings.append(var_str);
                dictionary_var_end_offsets.push_back(all_dict_var_strings.length());
            }
            if (false == try_read_tag(ir_buf, cursor_pos, encoded_tag)) {
                return ErrorCode_InComplete_IR;
            }
        }

        // now handle logtype
        std::string logtype;
        error_code = parse_log_type(ir_buf, encoded_tag, logtype, cursor_pos);
        if (ErrorCode_Success != error_code) {
            return error_code;
        }

        // now handle timestamp
        // this is different between 8 bytes and 4 bytes
        if (false == try_read_tag(ir_buf, cursor_pos, encoded_tag)) {
            return ErrorCode_InComplete_IR;
        }
        error_code = parse_timestamp<encoded_variable_t>(ir_buf, encoded_tag, timestamp, cursor_pos);
        if (ErrorCode_Success != error_code) {
            return error_code;
        }
        // now decode message
        message = decode_message(logtype, encoded_vars.data(),
                                 encoded_vars.size(), all_dict_var_strings,
                                 dictionary_var_end_offsets.data(),
                                 dictionary_var_end_offsets.size());

        ending_pos = cursor_pos;

        return ErrorCode_Success;
    }

    namespace four_byte_encoding {
        IR_ErrorCode decode_next_message (const std::vector<int8_t>& ir_buf,
                                          std::string& message,
                                          epoch_time_ms_t& ts_delta,
                                          size_t& ending_pos) {
            return decode_next_message_general<four_byte_encoded_variable_t>(ir_buf,
                                                                             message,
                                                                             ts_delta,
                                                                             ending_pos);
        }
    }

    namespace eight_byte_encoding {
        IR_ErrorCode decode_preamble (std::vector<int8_t>& ir_buf,
                                      TimestampInfo& ts_info,
                                      size_t& ending_pos) {

            size_t cursor_pos = ending_pos;
            encoded_tag_t encoded_tag;
            if (false == try_read_tag(ir_buf, cursor_pos, encoded_tag)) {
                return ErrorCode_InComplete_IR;
            }
            if (encoded_tag != cProtocol::Metadata::EncodingJson) {
                SPDLOG_ERROR("Unexpected encoding tag {}", encoded_tag);
                return ErrorCode_Corrupted_IR;
            }

            // read length
            if(false == try_read_tag(ir_buf, cursor_pos, encoded_tag)) {
                return ErrorCode_InComplete_IR;
            }
            unsigned int metadata_length;
            switch(encoded_tag) {
                case cProtocol::Metadata::LengthUByte:
                    uint8_t ubyte_res;
                    if (false == read_data_big_endian(ir_buf, ubyte_res, cursor_pos)) {
                        return ErrorCode_InComplete_IR;
                    }
                    metadata_length = ubyte_res;
                    break;
                case cProtocol::Metadata::LengthUShort:
                    uint16_t ushort_res;
                    if (false == read_data_big_endian(ir_buf, ushort_res, cursor_pos)) {
                        return ErrorCode_InComplete_IR;
                    }
                    metadata_length = ushort_res;
                    break;
                default:
                    SPDLOG_ERROR("Invalid Length Tag {}", encoded_tag);
                    return ErrorCode_Corrupted_IR;
            }

            // read the json contents
            std::string json_string;
            if (false == try_read_string(ir_buf, cursor_pos, json_string, metadata_length)) {
                return ErrorCode_InComplete_IR;
            }

            // TODO: should we do a try & catch to handle a corrupted json?
            auto metadata_json = nlohmann::json::parse(json_string);

            // Only here is the non-generic part
            ts_info.time_zone_id = metadata_json.at(ffi::ir_stream::cProtocol::Metadata::TimeZoneIdKey);
            ts_info.timestamp_pattern = metadata_json.at(ffi::ir_stream::cProtocol::Metadata::TimestampPatternKey);
            ts_info.timestamp_pattern_syntax = metadata_json.at(ffi::ir_stream::cProtocol::Metadata::TimestampPatternSyntaxKey);
            std::string version = metadata_json.at(ffi::ir_stream::cProtocol::Metadata::VersionKey);
            if (version != ffi::ir_stream::cProtocol::Metadata::VersionValue) {
                SPDLOG_ERROR("Deprecated version: {}", version);
                return ErrorCode_Unsupported_Version;
            }
            ending_pos = cursor_pos;

            return ErrorCode_Success;
        }

        IR_ErrorCode decode_next_message (const std::vector<int8_t>& ir_buf,
                                          std::string& message,
                                          epoch_time_ms_t& timestamp,
                                          size_t& ending_pos) {

            return decode_next_message_general<eight_byte_encoded_variable_t>(ir_buf,
                                                                              message,
                                                                              timestamp,
                                                                              ending_pos);
        }
    }
}
#include "encoding_methods.hpp"

// json
#include "../../../submodules/json/single_include/nlohmann/json.hpp"

// Project headers
#include "byteswap.hpp"
#include "protocol_constants.hpp"

using std::string_view;
using std::string;
using std::vector;

namespace ffi::ir_stream {
    // Local function prototypes
    /**
     * Encodes an integer into the IR stream
     * @tparam integer_t
     * @param value
     * @param ir_buf
     */
    template <typename integer_t>
    static void encode_int (integer_t value, vector<int8_t>& ir_buf);

    /**
     * Encodes the given logtype into the IR stream
     * @param logtype
     * @param ir_buf
     * @return true on success, false otherwise
     */
    static bool encode_logtype (string_view logtype, vector<int8_t>& ir_buf);

    /**
     * Encodes the given metadata into the IR stream
     * @param metadata
     * @param ir_buf
     * @return true on success, false otherwise
     */
    static bool encode_metadata (nlohmann::json& metadata, vector<int8_t>& ir_buf);

    /**
     * Adds the basic metadata fields to the given JSON object
     * @param timestamp_pattern
     * @param timestamp_pattern_syntax
     * @param time_zone_id
     * @param metadata
     */
    static void add_base_metadata_fields (string_view timestamp_pattern,
                                          string_view timestamp_pattern_syntax,
                                          string_view time_zone_id, nlohmann::json& metadata);

    /**
     * Appends a constant to the logtype, escaping variable placeholders when
     * @p contains_variable_placeholder is true
     * @param constant
     * @param contains_variable_placeholder Whether the constant contains a
     * variable placeholder
     * @param logtype
     * @return true
     */
    static bool append_constant_to_logtype (string_view constant,
                                            bool contains_variable_placeholder, string& logtype);

    /**
     * Appends the final constant to the logtype, escaping variable
     * placeholders as necessary
     * @param constant
     * @param logtype
     * @return Same as append_constant_to_logtype
     */
    static bool append_final_constant_to_logtype (string_view constant, string& logtype);

    /**
     * A functor for encoding dictionary variables in a message
     */
    class DictionaryVariableHandler {
    public:
        /**
         * Functor constructor
         * @param ir_buf Output buffer for the encoded data
         */
        explicit DictionaryVariableHandler (vector<int8_t>& ir_buf) : m_ir_buf(ir_buf) {}

        bool operator() (string_view message, size_t begin_pos, size_t end_pos) {
            auto length = end_pos - begin_pos;
            if (length <= UINT8_MAX) {
                m_ir_buf.push_back(cProtocol::Payload::VarStrLenUByte);
                m_ir_buf.push_back(bit_cast<int8_t>(static_cast<uint8_t>(length)));
            } else if (length <= UINT16_MAX) {
                m_ir_buf.push_back(cProtocol::Payload::VarStrLenUShort);
                encode_int(static_cast<uint16_t>(length), m_ir_buf);
            } else if (length <= INT32_MAX) {
                m_ir_buf.push_back(cProtocol::Payload::VarStrLenInt);
                encode_int(static_cast<int32_t>(length), m_ir_buf);
            } else {
                return false;
            }
            auto message_begin = message.cbegin();
            m_ir_buf.insert(m_ir_buf.cend(), message_begin + begin_pos, message_begin + end_pos);
            return true;
        }
    private:
        vector<int8_t>& m_ir_buf;
    };

    template <typename integer_t>
    static void encode_int (integer_t value, vector<int8_t>& ir_buf) {
        integer_t value_big_endian;
        static_assert(sizeof(integer_t) == 2 || sizeof(integer_t) == 4 || sizeof(integer_t) == 8);
        if constexpr (sizeof(value) == 2) {
            value_big_endian = bswap_16(value);
        } else if constexpr (sizeof(value) == 4) {
            value_big_endian = bswap_32(value);
        } else if constexpr (sizeof(value) == 8) {
            value_big_endian = bswap_64(value);
        }
        auto data = reinterpret_cast<int8_t*>(&value_big_endian);
        ir_buf.insert(ir_buf.end(), data, data + sizeof(value));
    }

    static bool encode_logtype (string_view logtype, vector<int8_t>& ir_buf) {
        auto length = logtype.length();
        if (length <= UINT8_MAX) {
            ir_buf.push_back(cProtocol::Payload::LogtypeStrLenUByte);
            ir_buf.push_back(bit_cast<int8_t>(static_cast<uint8_t>(length)));
        } else if (length <= UINT16_MAX) {
            ir_buf.push_back(cProtocol::Payload::LogtypeStrLenUShort);
            encode_int(static_cast<uint16_t>(length), ir_buf);
        } else if (length <= INT32_MAX) {
            ir_buf.push_back(cProtocol::Payload::LogtypeStrLenInt);
            encode_int(static_cast<int32_t>(length), ir_buf);
        } else {
            // Logtype is too long for encoding
            return false;
        }
        ir_buf.insert(ir_buf.cend(), logtype.cbegin(), logtype.cend());
        return true;
    }

    static bool encode_metadata (nlohmann::json& metadata, vector<int8_t>& ir_buf) {
        ir_buf.push_back(cProtocol::Metadata::EncodingJson);

        auto metadata_serialized = metadata.dump(-1, ' ', false,
                                                 nlohmann::json::error_handler_t::ignore);
        auto metadata_serialized_length = metadata_serialized.length();
        if (metadata_serialized_length <= UINT8_MAX) {
            ir_buf.push_back(cProtocol::Metadata::LengthUByte);
            ir_buf.push_back(bit_cast<int8_t>(static_cast<uint8_t>(metadata_serialized_length)));
        } else if (metadata_serialized_length <= UINT16_MAX) {
            ir_buf.push_back(cProtocol::Metadata::LengthUShort);
            encode_int(static_cast<uint16_t>(metadata_serialized_length), ir_buf);
        } else {
            // Can't encode metadata longer than 64 KiB
            return false;
        }
        ir_buf.insert(ir_buf.cend(), metadata_serialized.cbegin(), metadata_serialized.cend());

        return true;
    }

    static void add_base_metadata_fields (string_view timestamp_pattern,
                                          string_view timestamp_pattern_syntax,
                                          string_view time_zone_id, nlohmann::json& metadata)
    {
        metadata[cProtocol::Metadata::VersionKey] = cProtocol::Metadata::VersionValue;
        metadata[cProtocol::Metadata::VariablesSchemaIdKey] = cVariablesSchemaVersion;
        metadata[cProtocol::Metadata::VariableEncodingMethodsIdKey] =
                cVariableEncodingMethodsVersion;
        metadata[cProtocol::Metadata::TimestampPatternKey] = timestamp_pattern;
        metadata[cProtocol::Metadata::TimestampPatternSyntaxKey] = timestamp_pattern_syntax;
        metadata[cProtocol::Metadata::TimeZoneIdKey] = time_zone_id;
    }

    static bool append_constant_to_logtype (string_view constant,
                                            bool contains_variable_placeholder, string& logtype)
    {
        size_t begin_pos = 0;
        auto constant_len = constant.length();
        for (size_t i = 0; i < constant_len; ++i) {
            auto c = constant[i];
            if (cVariablePlaceholderEscapeCharacter == c || is_variable_placeholder(c)) {
                logtype.append(constant, begin_pos, i - begin_pos);
                logtype += cVariablePlaceholderEscapeCharacter;
                // NOTE: We don't need to append the character of interest
                // immediately since the next constant copy operation will
                // get it
                begin_pos = i;
            }
        }
        logtype.append(constant, begin_pos, constant_len - begin_pos);
        return true;
    }

    static bool append_final_constant_to_logtype (string_view constant, string& logtype) {
        // Since we don't know if the last constant contains a variable
        // placeholder, we'll simply say it does and append_constant_to_logtype
        // will escape one if it finds one
        return append_constant_to_logtype(constant, true, logtype);
    }

    namespace eight_byte_encoding {
        bool encode_preamble (string_view timestamp_pattern, string_view timestamp_pattern_syntax,
                              string_view time_zone_id, vector<int8_t>& ir_buf)
        {
            // Write magic number
            for (auto b : cProtocol::EightByteEncodingMagicNumber) {
                ir_buf.push_back(b);
            }

            // Assemble metadata
            nlohmann::json metadata_json;
            add_base_metadata_fields(timestamp_pattern, timestamp_pattern_syntax, time_zone_id,
                                     metadata_json);

            return encode_metadata(metadata_json, ir_buf);
        }

        bool encode_message (epoch_time_ms_t timestamp, string_view message, string& logtype,
                             vector<int8_t>& ir_buf)
        {
            auto encoded_var_handler = [&ir_buf] (eight_byte_encoded_variable_t encoded_var) {
                ir_buf.push_back(cProtocol::Payload::VarEightByteEncoding);
                encode_int(encoded_var, ir_buf);
            };

            if (false == encode_message_generically<eight_byte_encoded_variable_t>(
                    message, logtype, append_constant_to_logtype, append_final_constant_to_logtype,
                    encoded_var_handler, DictionaryVariableHandler(ir_buf)
            )) {
                return false;
            }

            if (false == encode_logtype(logtype, ir_buf)) {
                return false;
            }

            // Encode timestamp
            ir_buf.push_back(cProtocol::Payload::TimestampVal);
            encode_int(timestamp, ir_buf);

            return true;
        }
    }

    namespace four_byte_encoding {
        bool encode_preamble (
                string_view timestamp_pattern,
                string_view timestamp_pattern_syntax,
                string_view time_zone_id,
                epoch_time_ms_t reference_timestamp,
                vector<int8_t>& ir_buf
        ) {
            // Write magic number
            for (auto b : cProtocol::FourByteEncodingMagicNumber) {
                ir_buf.push_back(b);
            }

            // Assemble metadata
            nlohmann::json metadata_json;
            add_base_metadata_fields(timestamp_pattern, timestamp_pattern_syntax, time_zone_id,
                                     metadata_json);
            metadata_json[cProtocol::Metadata::ReferenceTimestampKey] =
                    std::to_string(reference_timestamp);

            return encode_metadata(metadata_json, ir_buf);
        }

        bool encode_message (epoch_time_ms_t timestamp_delta, string_view message, string& logtype,
                             vector<int8_t>& ir_buf)
        {
            if (false == encode_message(message, logtype, ir_buf)) {
                return false;
            }

            if (false == encode_timestamp(timestamp_delta, ir_buf)) {
                return false;
            }

            return true;
        }

        bool encode_message (string_view message, string& logtype, vector<int8_t>& ir_buf) {
            auto encoded_var_handler = [&ir_buf] (four_byte_encoded_variable_t encoded_var) {
                ir_buf.push_back(cProtocol::Payload::VarFourByteEncoding);
                encode_int(encoded_var, ir_buf);
            };

            if (false == encode_message_generically<four_byte_encoded_variable_t>(
                    message, logtype, append_constant_to_logtype, append_final_constant_to_logtype,
                    encoded_var_handler, DictionaryVariableHandler(ir_buf)
            )) {
                return false;
            }

            if (false == encode_logtype(logtype, ir_buf)) {
                return false;
            }

            return true;
        }

        bool encode_timestamp (epoch_time_ms_t timestamp_delta, std::vector<int8_t>& ir_buf) {
            if (INT8_MIN <= timestamp_delta && timestamp_delta <= INT8_MAX) {
                ir_buf.push_back(cProtocol::Payload::TimestampDeltaByte);
                ir_buf.push_back(static_cast<int8_t>(timestamp_delta));
            } else if (INT16_MIN <= timestamp_delta && timestamp_delta <= INT16_MAX) {
                ir_buf.push_back(cProtocol::Payload::TimestampDeltaShort);
                encode_int(static_cast<int16_t>(timestamp_delta), ir_buf);
            } else if (INT32_MIN <= timestamp_delta && timestamp_delta <= INT32_MAX) {
                ir_buf.push_back(cProtocol::Payload::TimestampDeltaInt);
                encode_int(static_cast<int32_t>(timestamp_delta), ir_buf);
            } else {
                // Delta exceeds maximum representable by an int (24.86 days)
                return false;
            }

            return true;
        }
    }
}

#include "IrMessageParser.hpp"

// C standard libraries

// C++ standard libraries

// Project headers
#include "BufferReader.hpp"
#include "EncodedVariableInterpreter.hpp"
#include "ffi/encoding_methods.hpp"
#include "ffi/ir_stream/protocol_constants.hpp"

// spdlog
#include "spdlog/spdlog.h"

// json
#include "../../../submodules/json/single_include/nlohmann/json.hpp"

using ffi::cVariablePlaceholderEscapeCharacter;
using ffi::four_byte_encoded_variable_t;
using ffi::ir_stream::cProtocol::MagicNumberLength;
using ffi::ir_stream::IRErrorCode;
using ffi::VariablePlaceholder;
using std::string;

static bool decode_json_preamble (ReaderInterface& reader, std::string& json_metadata);

bool IrMessageParser::parse_four_bytes_encoded_message(ReaderInterface& reader,
                                                       ParsedIrMessage& msg,
                                                       epochtime_t& reference_ts) {
    std::vector<four_byte_encoded_variable_t> encoded_vars;
    std::vector<string> dict_vars;
    string logtype;
    epochtime_t ts;

    auto error_code = ffi::ir_stream::four_byte_encoding::decode_tokens(
            reader, logtype, encoded_vars, dict_vars, ts
    );

    if (IRErrorCode::IRErrorCode_Success != error_code) {
        if (IRErrorCode::IRErrorCode_Eof != error_code) {
            SPDLOG_ERROR("Corrupted IR with error code {}", error_code);
        }
        return false;
    }

    msg.clear();

    reference_ts += ts;
    msg.set_ts(reference_ts);

    size_t encoded_vars_length = encoded_vars.size();
    size_t dict_vars_length = dict_vars.size();
    size_t next_static_text_begin_pos = 0;

    size_t dictionary_vars_ix = 0;
    size_t encoded_vars_ix = 0;
    for (size_t cur_pos = 0; cur_pos < logtype.size(); ++cur_pos) {
        auto c = logtype[cur_pos];
        switch(c) {
            case enum_to_underlying_type(VariablePlaceholder::Float): {
                msg.append_to_logtype(logtype, next_static_text_begin_pos,
                                      cur_pos - next_static_text_begin_pos);
                next_static_text_begin_pos = cur_pos + 1;
                if (encoded_vars_ix >= encoded_vars_length) {

                    return false;
                }

                auto encoded_float = encoded_vars[encoded_vars_ix];

                // handle float
                // assume that we need the actual size
                auto decoded_float = ffi::decode_float_var(encoded_float);
                auto clp_encoded_float = EncodedVariableInterpreter::convert_four_bytes_float_to_clp_encoded_float(encoded_float);

                msg.add_encoded_float(clp_encoded_float, decoded_float.size());

                ++encoded_vars_ix;
                break;
            }

            case enum_to_underlying_type(VariablePlaceholder::Integer): {
                msg.append_to_logtype(logtype, next_static_text_begin_pos,
                                      cur_pos - next_static_text_begin_pos);
                next_static_text_begin_pos = cur_pos + 1;
                if (encoded_vars_ix >= encoded_vars_length) {
                    SPDLOG_ERROR("Some error message");
                    return false;
                }

                // handle integer
                auto encoded_int = encoded_vars[encoded_vars_ix];
                // assume that we need the actual size
                auto decoded_int = ffi::decode_integer_var(encoded_int);
                msg.add_encoded_integer(encoded_int, decoded_int.size());

                ++encoded_vars_ix;
                break;
            }

            case enum_to_underlying_type(VariablePlaceholder::Dictionary): {
                msg.append_to_logtype(logtype, next_static_text_begin_pos,
                                      cur_pos - next_static_text_begin_pos);
                next_static_text_begin_pos = cur_pos + 1;
                if (dictionary_vars_ix >= dict_vars_length) {
                    SPDLOG_ERROR("Some error message");
                    return false;
                }

                const auto& var_string {dict_vars[dictionary_vars_ix]} ;

                encoded_variable_t converted_var;
                if (EncodedVariableInterpreter::convert_string_to_representable_integer_var(var_string, converted_var)) {
                    msg.add_encoded_integer(converted_var, var_string.size());
                } else if (EncodedVariableInterpreter::convert_string_to_representable_float_var(var_string, converted_var)) {
                    msg.add_encoded_float(converted_var, var_string.size());
                } else {
                    msg.add_dictionary_var(var_string);
                }

                ++dictionary_vars_ix;

                break;
            }

            case cVariablePlaceholderEscapeCharacter: {
                // Ensure the escape character is followed by a
                // character that's being escaped
                if (cur_pos == logtype.size() - 1) {
                    SPDLOG_ERROR("Some error message");
                }
                msg.append_to_logtype(logtype, next_static_text_begin_pos,
                                      cur_pos - next_static_text_begin_pos);

                // Skip the escape character
                next_static_text_begin_pos = cur_pos + 1;
                // The character after the escape character is static text
                // (regardless of whether it is a variable placeholder), so
                // increment cur_pos by 1 to ensure we don't process the
                // next character in any of the other cases (instead it will
                // be added to the message).
                ++cur_pos;

                break;
            }
        }
    }
    // Add remainder
    if (next_static_text_begin_pos < logtype.size()) {
        msg.append_to_logtype(logtype, next_static_text_begin_pos,
                              logtype.size() - next_static_text_begin_pos);
    }

    return true;
}

bool IrMessageParser::decode_four_bytes_preamble (ReaderInterface& reader, std::string& ts_pattern,
                                                  epochtime_t& reference_ts)
{
    string json_metadata;
    const string mocked_ts_pattern = "%Y-%m-%dT%H:%M:%S.%3";
    if (false == decode_json_preamble(reader, json_metadata)) {
        return false;
    }
    try {
        auto metadata_json = nlohmann::json::parse(json_metadata);
        string version = metadata_json.at(ffi::ir_stream::cProtocol::Metadata::VersionKey);
        if (version != ffi::ir_stream::cProtocol::Metadata::VersionValue) {
            SPDLOG_ERROR("Unsupported version");
            return false;
        }

        // For now, use a fixed timestamp pattern
        ts_pattern = mocked_ts_pattern;

        reference_ts = std::stoll(metadata_json.at(
                ffi::ir_stream::cProtocol::Metadata::ReferenceTimestampKey).get<string>());

    } catch (const nlohmann::json::parse_error& e) {
        SPDLOG_ERROR("Failed to parse json metadata");
        return false;
    }

    return true;
}

bool IrMessageParser::is_ir_encoded (ReaderInterface& reader, bool& is_four_bytes_encoded) {

    // Note. currently this method doesn't recover file pos.
    if (ffi::ir_stream::IRErrorCode_Success !=
        ffi::ir_stream::get_encoding_type(reader, is_four_bytes_encoded)) {
        return false;
    }
    return true;
}

static bool decode_json_preamble (ReaderInterface& reader, std::string& json_metadata) {
    // Decode and parse metadata
    ffi::ir_stream::encoded_tag_t metadata_type;
    std::vector<int8_t> metadata_vec;

    if (ffi::ir_stream::IRErrorCode_Success !=
        ffi::ir_stream::decode_preamble(reader, metadata_type, metadata_vec)) {
        SPDLOG_ERROR("Failed to parse metadata");
        return false;
    }

    if (ffi::ir_stream::cProtocol::Metadata::EncodingJson != metadata_type) {
        SPDLOG_ERROR("Unexpected metadata type");
        return false;
    }

    json_metadata.assign(reinterpret_cast<const char*>(metadata_vec.data()),
                         metadata_vec.size());

    return true;
}
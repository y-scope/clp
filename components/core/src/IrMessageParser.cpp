#include "IrMessageParser.hpp"

// C standard libraries

// C++ standard libraries

// Project headers
#include "ffi/encoding_methods.hpp"
#include "LogTypeDictionaryEntry.hpp"
#include "EncodedVariableInterpreter.hpp"

// spdlog
#include "spdlog/spdlog.h"

using ffi::ir_stream::IRErrorCode;
using ffi::VariablePlaceholder;
using ffi::cVariablePlaceholderEscapeCharacter;
using ffi::four_byte_encoded_variable_t;

bool IrMessageParser::parse_four_bytes_encoded_message(BufferedReaderInterface& reader,
                                                       ParsedIrMessage& msg,
                                                       epochtime_t& reference_ts) {
    std::vector<four_byte_encoded_variable_t> encoded_vars;
    std::vector<BufferedReaderInterface::MyStringView> dict_vars;
    BufferedReaderInterface::MyStringView logtype;
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

    std::string logtype_str;

    auto buffer_begin_ptr = reader.get_buffer_ptr();
    size_t encoded_vars_length = encoded_vars.size();
    size_t dict_vars_length = dict_vars.size();
    size_t next_static_text_begin_pos = 0;

    size_t dictionary_vars_ix = 0;
    size_t encoded_vars_ix = 0;
    auto logtype_ptr = buffer_begin_ptr + logtype.m_buffer_pos;
    for (size_t cur_pos = 0; cur_pos < logtype.m_size; ++cur_pos) {
        auto c = logtype_ptr[cur_pos];
        switch(c) {
            case enum_to_underlying_type(VariablePlaceholder::Float): {
                msg.append_to_logtype(logtype_ptr + next_static_text_begin_pos,
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
                msg.append_to_logtype(logtype_ptr + next_static_text_begin_pos,
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
                msg.append_to_logtype(logtype_ptr + next_static_text_begin_pos,
                                      cur_pos - next_static_text_begin_pos);
                next_static_text_begin_pos = cur_pos + 1;
                if (dictionary_vars_ix >= dict_vars_length) {
                    SPDLOG_ERROR("Some error message");
                    return false;
                }

                auto offset = dict_vars[dictionary_vars_ix].m_buffer_pos;
                auto size = dict_vars[dictionary_vars_ix].m_size;
                std::string var_string {buffer_begin_ptr + offset, size};

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
                if (cur_pos == logtype.m_size - 1) {
                    SPDLOG_ERROR("Some error message");
                }
                msg.append_to_logtype(logtype_ptr + next_static_text_begin_pos,
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
    if (next_static_text_begin_pos < logtype.m_size) {
        msg.append_to_logtype(logtype_ptr + next_static_text_begin_pos,
                              logtype.m_size - next_static_text_begin_pos);
    }

    return true;
}
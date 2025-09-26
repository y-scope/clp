#ifndef CLP_FFI_IR_STREAM_DECODING_METHODS_INC
#define CLP_FFI_IR_STREAM_DECODING_METHODS_INC

#include <string>
#include <vector>

#include "../../ir/types.hpp"
#include "../encoding_methods.hpp"
#include "decoding_methods.hpp"
#include "protocol_constants.hpp"

namespace clp::ffi::ir_stream {
template <
        bool unescape_logtype,
        typename encoded_variable_t,
        typename ConstantHandler,
        typename EncodedIntHandler,
        typename EncodedFloatHandler,
        typename DictVarHandler>
void generic_decode_message(
        std::string const& logtype,
        std::vector<encoded_variable_t> const& encoded_vars,
        std::vector<std::string> const& dict_vars,
        ConstantHandler constant_handler,
        EncodedIntHandler encoded_int_handler,
        EncodedFloatHandler encoded_float_handler,
        DictVarHandler dict_var_handler
) {
    auto const logtype_length = logtype.length();
    auto const encoded_vars_length = encoded_vars.size();
    auto const dict_vars_length = dict_vars.size();
    size_t next_static_text_begin_pos = 0;

    size_t dictionary_vars_ix = 0;
    size_t encoded_vars_ix = 0;
    for (size_t cur_pos = 0; cur_pos < logtype_length; ++cur_pos) {
        auto c = logtype[cur_pos];
        switch (c) {
            case enum_to_underlying_type(ir::VariablePlaceholder::Float): {
                constant_handler(
                        logtype,
                        next_static_text_begin_pos,
                        cur_pos - next_static_text_begin_pos
                );
                next_static_text_begin_pos = cur_pos + 1;
                if (encoded_vars_ix >= encoded_vars_length) {
                    throw DecodingException(
                            ErrorCode_Corrupt,
                            __FILENAME__,
                            __LINE__,
                            cTooFewEncodedVarsErrorMessage
                    );
                }
                encoded_float_handler(encoded_vars[encoded_vars_ix]);
                ++encoded_vars_ix;

                break;
            }

            case enum_to_underlying_type(ir::VariablePlaceholder::Integer): {
                constant_handler(
                        logtype,
                        next_static_text_begin_pos,
                        cur_pos - next_static_text_begin_pos
                );
                next_static_text_begin_pos = cur_pos + 1;
                if (encoded_vars_ix >= encoded_vars_length) {
                    throw DecodingException(
                            ErrorCode_Corrupt,
                            __FILENAME__,
                            __LINE__,
                            cTooFewEncodedVarsErrorMessage
                    );
                }
                encoded_int_handler(encoded_vars[encoded_vars_ix]);
                ++encoded_vars_ix;

                break;
            }

            case enum_to_underlying_type(ir::VariablePlaceholder::Dictionary): {
                constant_handler(
                        logtype,
                        next_static_text_begin_pos,
                        cur_pos - next_static_text_begin_pos
                );
                next_static_text_begin_pos = cur_pos + 1;
                if (dictionary_vars_ix >= dict_vars_length) {
                    throw DecodingException(
                            ErrorCode_Corrupt,
                            __FILENAME__,
                            __LINE__,
                            cTooFewDictionaryVarsErrorMessage
                    );
                }
                dict_var_handler(dict_vars[dictionary_vars_ix]);
                ++dictionary_vars_ix;

                break;
            }

            case enum_to_underlying_type(ir::VariablePlaceholder::Escape): {
                // Ensure the escape character is followed by a character that's being escaped
                if (cur_pos == logtype_length - 1) {
                    throw DecodingException(
                            ErrorCode_Corrupt,
                            __FILENAME__,
                            __LINE__,
                            cUnexpectedEscapeCharacterMessage
                    );
                }

                if constexpr (unescape_logtype) {
                    constant_handler(
                            logtype,
                            next_static_text_begin_pos,
                            cur_pos - next_static_text_begin_pos
                    );

                    // Skip the escape character
                    next_static_text_begin_pos = cur_pos + 1;
                }
                // The character after the escape character is static text (regardless of whether it
                // is a variable placeholder), so increment cur_pos by 1 to ensure we don't process
                // the next character in any of the other cases (instead it will be added to the
                // message).
                ++cur_pos;

                break;
            }
        }
    }
    // Add remainder
    if (next_static_text_begin_pos < logtype_length) {
        constant_handler(
                logtype,
                next_static_text_begin_pos,
                logtype_length - next_static_text_begin_pos
        );
    }
}
}  // namespace clp::ffi::ir_stream

#endif  // CLP_FFI_IR_STREAM_DECODING_METHODS_INC

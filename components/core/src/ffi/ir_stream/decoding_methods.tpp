#ifndef FFI_IR_STREAM_DECODING_METHODS_TPP
#define FFI_IR_STREAM_DECODING_METHODS_TPP

// C++ standard libraries
#include <string_view>
#include <vector>

// Project headers
#include "../../ReaderInterface.hpp"
#include "../encoding_methods.hpp"
#include "decoding_methods.hpp"
#include "protocol_constants.hpp"

namespace ffi::ir_stream {

    /**
     * TBD
     * @tparam encoded_variable_t
     * @tparam ConstantHandler
     * @tparam EncodedIntHandler
     * @tparam EncodedFloatHandler
     * @tparam DictVarHandler
     * @param logtype
     * @param encoded_vars
     * @param dict_vars
     * @param constant_handler
     * @param encoded_int_handler
     * @param encoded_float_handler
     * @param dict_var_handler
     */
    template <typename encoded_variable_t, typename ConstantHandler,
              typename EncodedIntHandler, typename EncodedFloatHandler, typename DictVarHandler>
    void generic_decode_message (const std::string& logtype,
                                 const std::vector<encoded_variable_t>& encoded_vars,
                                 const std::vector<std::string>& dict_vars,
                                 ConstantHandler constant_handler,
                                 EncodedIntHandler encoded_int_handler,
                                 EncodedFloatHandler encoded_float_handler,
                                 DictVarHandler dict_var_handler) {
        size_t encoded_vars_length = encoded_vars.size();
        size_t dict_vars_length = dict_vars.size();
        size_t next_static_text_begin_pos = 0;

        size_t dictionary_vars_ix = 0;
        size_t encoded_vars_ix = 0;
        for (size_t cur_pos = 0; cur_pos < logtype.length(); ++cur_pos) {
            auto c = logtype[cur_pos];
            switch (c) {
                case enum_to_underlying_type(VariablePlaceholder::Float): {
                    constant_handler(logtype, next_static_text_begin_pos,
                                     cur_pos - next_static_text_begin_pos);
                    next_static_text_begin_pos = cur_pos + 1;
                    if (encoded_vars_ix >= encoded_vars_length) {
                        throw DecodingException(ErrorCode_Corrupt, __FILENAME__, __LINE__,
                                                cTooFewEncodedVarsErrorMessage);
                    }
                    encoded_float_handler(encoded_vars[encoded_vars_ix]);
                    ++encoded_vars_ix;

                    break;
                }

                case enum_to_underlying_type(VariablePlaceholder::Integer): {
                    constant_handler(logtype, next_static_text_begin_pos,
                                     cur_pos - next_static_text_begin_pos);
                    next_static_text_begin_pos = cur_pos + 1;
                    if (encoded_vars_ix >= encoded_vars_length) {
                        throw DecodingException(ErrorCode_Corrupt, __FILENAME__, __LINE__,
                                                cTooFewEncodedVarsErrorMessage);
                    }
                    encoded_int_handler(encoded_vars[encoded_vars_ix]);
                    ++encoded_vars_ix;

                    break;
                }

                case enum_to_underlying_type(VariablePlaceholder::Dictionary): {
                    constant_handler(logtype, next_static_text_begin_pos,
                                     cur_pos - next_static_text_begin_pos);
                    next_static_text_begin_pos = cur_pos + 1;
                    if (dictionary_vars_ix >= dict_vars_length) {
                        throw DecodingException(ErrorCode_Corrupt, __FILENAME__, __LINE__,
                                                cTooFewDictionaryVarsErrorMessage);
                    }
                    dict_var_handler(dict_vars[dictionary_vars_ix]);
                    ++dictionary_vars_ix;

                    break;
                }

                case cVariablePlaceholderEscapeCharacter: {
                    // Ensure the escape character is followed by a
                    // character that's being escaped
                    if (cur_pos == logtype.length() - 1) {
                        throw DecodingException(ErrorCode_Corrupt, __FILENAME__, __LINE__,
                                                cUnexpectedEscapeCharacterMessage);
                    }
                    constant_handler(logtype, next_static_text_begin_pos,
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
        if (next_static_text_begin_pos < logtype.length()) {
            constant_handler(logtype, next_static_text_begin_pos,
                             logtype.length() - next_static_text_begin_pos);
        }
    }
}

#endif //FFI_IR_STREAM_DECODING_METHODS_TPP

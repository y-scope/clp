#ifndef GLT_FFI_ENCODING_METHODS_INC
#define GLT_FFI_ENCODING_METHODS_INC

#include <algorithm>

#include <string_utils/string_utils.hpp>

#include "../ir/parsing.hpp"
#include "../ir/types.hpp"
#include "../type_utils.hpp"

namespace glt::ffi {
template <typename encoded_variable_t>
bool encode_float_string(std::string_view str, encoded_variable_t& encoded_var) {
    auto const value_length = str.length();
    if (0 == value_length) {
        // Can't convert an empty string
        return false;
    }

    size_t pos = 0;
    constexpr size_t cMaxDigitsInRepresentableFloatVar
            = std::is_same_v<encoded_variable_t, ir::four_byte_encoded_variable_t>
                      ? cMaxDigitsInRepresentableFourByteFloatVar
                      : cMaxDigitsInRepresentableEightByteFloatVar;
    // +1 for decimal point
    size_t max_length = cMaxDigitsInRepresentableFloatVar + 1;

    // Check for a negative sign
    bool is_negative = false;
    if ('-' == str[pos]) {
        is_negative = true;
        ++pos;
        // Include sign in max length
        ++max_length;
    }

    // Check if value can be represented in encoded format
    if (value_length > max_length) {
        return false;
    }

    size_t num_digits = 0;
    size_t decimal_point_pos = std::string::npos;
    std::conditional_t<
            std::is_same_v<encoded_variable_t, ir::four_byte_encoded_variable_t>,
            uint32_t,
            uint64_t>
            digits = 0;
    for (; pos < value_length; ++pos) {
        auto c = str[pos];
        if ('0' <= c && c <= '9') {
            digits *= 10;
            digits += (c - '0');
            ++num_digits;
        } else if (std::string::npos == decimal_point_pos && '.' == c) {
            decimal_point_pos = value_length - 1 - pos;
        } else {
            // Invalid character
            return false;
        }
    }
    if (std::string::npos == decimal_point_pos || 0 == decimal_point_pos || 0 == num_digits) {
        // No decimal point found, decimal point is after all digits, or no digits found
        return false;
    }
    if constexpr (std::is_same_v<encoded_variable_t, ir::four_byte_encoded_variable_t>) {
        if (cFourByteEncodedFloatDigitsBitMask < digits) {
            // digits is larger than maximum representable
            return false;
        }
    }

    encoded_var = encode_float_properties<encoded_variable_t>(
            is_negative,
            digits,
            num_digits,
            decimal_point_pos
    );

    return true;
}

template <typename encoded_variable_t>
encoded_variable_t encode_float_properties(
        bool is_negative,
        std::conditional_t<
                std::is_same_v<encoded_variable_t, ir::four_byte_encoded_variable_t>,
                uint32_t,
                uint64_t> digits,
        size_t num_digits,
        size_t decimal_point_pos
) {
    static_assert(
            (std::is_same_v<encoded_variable_t, ir::eight_byte_encoded_variable_t>
             || std::is_same_v<encoded_variable_t, ir::four_byte_encoded_variable_t>)
    );
    if constexpr (std::is_same_v<encoded_variable_t, ir::eight_byte_encoded_variable_t>) {
        // Encode into 64 bits with the following format (from MSB to LSB):
        // -  1 bit : is negative
        // -  1 bit : unused
        // - 54 bits: The digits of the float without the decimal, as an integer
        // -  4 bits: # of decimal digits minus 1
        //     - This format can represent floats with between 1 and 16 decimal digits, so we use 4
        //       bits and map the range [1, 16] to [0x0, 0xF]
        // -  4 bits: position of the decimal from the right minus 1
        //     - To see why the position is taken from the right, consider
        //       (1) "-123456789012345.6", (2) "-.1234567890123456", and
        //       (3) ".1234567890123456"
        //         - For (1), the decimal point is at index 16 from the left and index 1 from the
        //           right.
        //         - For (2), the decimal point is at index 1 from the left and index 16 from the
        //           right.
        //         - For (3), the decimal point is at index 0 from the left and index 16 from the
        //           right.
        //         - So if we take the decimal position from the left, it can range from 0 to 16
        //           because of the negative sign. Whereas from the right, the negative sign is
        //           inconsequential.
        //     - Thus, we use 4 bits and map the range [1, 16] to [0x0, 0xF].
        uint64_t encoded_float = 0;
        if (is_negative) {
            encoded_float = 1;
        }
        encoded_float <<= 55;  // 1 unused + 54 for digits of the float
        encoded_float |= digits & cEightByteEncodedFloatDigitsBitMask;
        encoded_float <<= 4;
        encoded_float |= (num_digits - 1) & 0x0F;
        encoded_float <<= 4;
        encoded_float |= (decimal_point_pos - 1) & 0x0F;
        return bit_cast<encoded_variable_t>(encoded_float);
    } else {
        // std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>

        // Encode into 32 bits with the following format (from MSB to LSB):
        // -  1 bit : is negative
        // - 25 bits: The digits of the float without the decimal, as an integer
        // -  3 bits: # of decimal digits minus 1
        //     - This format can represent floats with between 1 and 8 decimal digits, so we use 3
        //       bits and map the range [1, 8] to [0x0, 0x7]
        // -  3 bits: position of the decimal from the right minus 1
        //     - To see why the position is taken from the right, consider
        //       (1) "-1234567.8", (2) "-.12345678", and (3) ".12345678"
        //         - For (1), the decimal point is at index 8 from the left and index 1 from the
        //           right.
        //         - For (2), the decimal point is at index 1 from the left and index 8 from the
        //           right.
        //         - For (3), the decimal point is at index 0 from the left and index 8 from the
        //           right.
        //         - So if we take the decimal position from the left, it can range from 0 to 8
        //           because of the negative sign. Whereas from the right, the negative sign is
        //           inconsequential.
        //     - Thus, we use 3 bits and map the range [1, 8] to [0x0, 0x7].
        uint32_t encoded_float = 0;
        if (is_negative) {
            encoded_float = 1;
        }
        encoded_float <<= 25;  // 25 for digits of the float
        encoded_float |= digits & cFourByteEncodedFloatDigitsBitMask;
        encoded_float <<= 3;
        encoded_float |= (num_digits - 1) & 0x07;
        encoded_float <<= 3;
        encoded_float |= (decimal_point_pos - 1) & 0x07;
        return bit_cast<encoded_variable_t>(encoded_float);
    }
}

template <typename encoded_variable_t>
void decode_float_properties(
        encoded_variable_t encoded_var,
        bool& is_negative,
        std::conditional_t<
                std::is_same_v<encoded_variable_t, ir::four_byte_encoded_variable_t>,
                uint32_t,
                uint64_t>& digits,
        uint8_t& num_digits,
        uint8_t& decimal_point_pos
) {
    static_assert(
            (std::is_same_v<encoded_variable_t, ir::eight_byte_encoded_variable_t>
             || std::is_same_v<encoded_variable_t, ir::four_byte_encoded_variable_t>)
    );
    if constexpr (std::is_same_v<encoded_variable_t, ir::eight_byte_encoded_variable_t>) {
        auto encoded_float = bit_cast<uint64_t>(encoded_var);

        // Decode according to the format described in encode_float_string
        decimal_point_pos = (encoded_float & 0x0F) + 1;
        encoded_float >>= 4;
        num_digits = (encoded_float & 0x0F) + 1;
        encoded_float >>= 4;
        digits = encoded_float & cEightByteEncodedFloatDigitsBitMask;
        // This is the maximum base-10 number with cMaxDigitsInRepresentableEightByteFloatVar
        constexpr uint64_t cMaxRepresentableDigitsValue = 9'999'999'999'999'999;
        if (digits > cMaxRepresentableDigitsValue) {
            throw EncodingException(
                    ErrorCode_Corrupt,
                    __FILENAME__,
                    __LINE__,
                    "Digits in encoded float are larger than max representable "
                    "value."
            );
        }
        encoded_float >>= 55;
        is_negative = encoded_float > 0;
    } else {
        // std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>
        auto encoded_float = bit_cast<uint32_t>(encoded_var);

        // Decode according to the format in encode_string_as_float_compact_var
        decimal_point_pos = (encoded_float & 0x07) + 1;
        encoded_float >>= 3;
        num_digits = (encoded_float & 0x07) + 1;
        encoded_float >>= 3;
        digits = encoded_float & cFourByteEncodedFloatDigitsBitMask;
        encoded_float >>= 25;
        is_negative = encoded_float > 0;
    }
}

template <typename encoded_variable_t>
std::string decode_float_var(encoded_variable_t encoded_var) {
    std::string value;

    uint8_t decimal_point_pos;
    uint8_t num_digits;
    std::conditional_t<
            std::is_same_v<encoded_variable_t, ir::four_byte_encoded_variable_t>,
            uint32_t,
            uint64_t>
            digits;
    bool is_negative;
    decode_float_properties(encoded_var, is_negative, digits, num_digits, decimal_point_pos);

    if (num_digits < decimal_point_pos) {
        throw EncodingException(
                ErrorCode_Corrupt,
                __FILENAME__,
                __LINE__,
                "Invalid decimal-point position in encoded float."
        );
    }

    size_t value_length = num_digits + 1 + is_negative;
    value.resize(value_length);
    size_t num_chars_to_process = value_length;

    // Add sign
    if (is_negative) {
        value[0] = '-';
        --num_chars_to_process;
    }

    // Decode until the decimal or the non-zero digits are exhausted
    size_t pos = value_length - 1;
    auto decimal_point_pos_from_left = value_length - 1 - decimal_point_pos;
    for (; pos > decimal_point_pos_from_left && digits > 0; --pos) {
        value[pos] = (char)('0' + (digits % 10));
        digits /= 10;
        --num_chars_to_process;
    }

    if (digits > 0) {
        constexpr char cTooManyDigitsErrorMsg[] = "Encoded number of digits doesn't match "
                                                  "encoded digits in encoded float.";
        if (0 == num_chars_to_process) {
            throw EncodingException(
                    ErrorCode_Corrupt,
                    __FILENAME__,
                    __LINE__,
                    cTooManyDigitsErrorMsg
            );
        }
        // Skip decimal since it's added at the end
        --pos;
        --num_chars_to_process;

        while (digits > 0) {
            if (0 == num_chars_to_process) {
                throw EncodingException(
                        ErrorCode_Corrupt,
                        __FILENAME__,
                        __LINE__,
                        cTooManyDigitsErrorMsg
                );
            }

            value[pos--] = (char)('0' + (digits % 10));
            digits /= 10;
            --num_chars_to_process;
        }
    }

    // Add remaining zeros
    for (; num_chars_to_process > 0; --num_chars_to_process) {
        value[pos--] = '0';
    }

    // Add decimal
    value[decimal_point_pos_from_left] = '.';

    return value;
}

template <typename encoded_variable_t>
bool encode_integer_string(std::string_view str, encoded_variable_t& encoded_var) {
    size_t length = str.length();
    if (0 == length) {
        // Empty string cannot be converted
        return false;
    }

    // Ensure start of value is an integer with no zero-padding or positive sign
    if ('-' == str[0]) {
        // Ensure first character after sign is a non-zero integer
        if (length < 2 || str[1] < '1' || '9' < str[1]) {
            return false;
        }
    } else {
        // Ensure first character is a digit
        if (str[0] < '0' || '9' < str[0]) {
            return false;
        }

        // Ensure value is not zero-padded
        if (length > 1 && '0' == str[0]) {
            return false;
        }
    }

    encoded_variable_t result;
    if (false == clp::string_utils::convert_string_to_int(str, result)) {
        // Conversion failed
        return false;
    } else {
        encoded_var = result;
    }

    return true;
}

template <typename encoded_variable_t>
std::string decode_integer_var(encoded_variable_t encoded_var) {
    return std::to_string(encoded_var);
}

template <
        typename encoded_variable_t,
        typename ConstantHandler,
        typename EncodedVariableHandler,
        typename DictionaryVariableHandler>
bool encode_message_generically(
        std::string_view message,
        std::string& logtype,
        ConstantHandler constant_handler,
        EncodedVariableHandler encoded_variable_handler,
        DictionaryVariableHandler dictionary_variable_handler
) {
    size_t var_begin_pos = 0;
    size_t var_end_pos = 0;
    size_t constant_begin_pos = 0;
    logtype.clear();
    logtype.reserve(message.length());
    while (ir::get_bounds_of_next_var(message, var_begin_pos, var_end_pos)) {
        std::string_view constant{&message[constant_begin_pos], var_begin_pos - constant_begin_pos};
        constant_handler(constant, logtype);
        constant_begin_pos = var_end_pos;

        // Encode the variable
        std::string_view var_string{&message[var_begin_pos], var_end_pos - var_begin_pos};
        encoded_variable_t encoded_variable;
        if (encode_float_string(var_string, encoded_variable)) {
            logtype += enum_to_underlying_type(ir::VariablePlaceholder::Float);
            encoded_variable_handler(encoded_variable);
        } else if (encode_integer_string(var_string, encoded_variable)) {
            logtype += enum_to_underlying_type(ir::VariablePlaceholder::Integer);
            encoded_variable_handler(encoded_variable);
        } else {
            logtype += enum_to_underlying_type(ir::VariablePlaceholder::Dictionary);
            if (false == dictionary_variable_handler(message, var_begin_pos, var_end_pos)) {
                return false;
            }
        }
    }
    // Append any remaining message content to the logtype
    if (constant_begin_pos < message.length()) {
        std::string_view constant{
                &message[constant_begin_pos],
                message.length() - constant_begin_pos
        };
        constant_handler(constant, logtype);
    }

    return true;
}

template <typename encoded_variable_t>
bool encode_message(
        std::string_view message,
        std::string& logtype,
        std::vector<encoded_variable_t>& encoded_vars,
        std::vector<int32_t>& dictionary_var_bounds
) {
    auto encoded_variable_handler = [&encoded_vars](encoded_variable_t encoded_variable) {
        encoded_vars.push_back(encoded_variable);
    };
    auto dictionary_variable_handler
            = [&dictionary_var_bounds](std::string_view, size_t begin_pos, size_t end_pos) {
                  if (begin_pos > INT32_MAX || end_pos > INT32_MAX) {
                      return false;
                  }

                  dictionary_var_bounds.push_back(static_cast<int32_t>(begin_pos));
                  dictionary_var_bounds.push_back(static_cast<int32_t>(end_pos));
                  return true;
              };

    if (false
        == encode_message_generically<encoded_variable_t>(
                message,
                logtype,
                ir::escape_and_append_const_to_logtype,
                encoded_variable_handler,
                dictionary_variable_handler
        ))
    {
        return false;
    }

    return true;
}

template <typename encoded_variable_t>
std::string decode_message(
        std::string_view logtype,
        encoded_variable_t* encoded_vars,
        size_t encoded_vars_length,
        std::string_view all_dictionary_vars,
        int32_t const* dictionary_var_end_offsets,
        size_t dictionary_var_end_offsets_length
) {
    std::string message;
    size_t last_variable_end_pos = 0;
    size_t dictionary_var_begin_pos = 0;
    size_t dictionary_var_bounds_ix = 0;
    size_t encoded_vars_ix = 0;
    for (size_t i = 0; i < logtype.length(); ++i) {
        auto c = logtype[i];
        if (enum_to_underlying_type(ir::VariablePlaceholder::Float) == c) {
            message.append(logtype, last_variable_end_pos, i - last_variable_end_pos);
            last_variable_end_pos = i + 1;
            if (encoded_vars_ix >= encoded_vars_length) {
                throw EncodingException(
                        ErrorCode_Corrupt,
                        __FILENAME__,
                        __LINE__,
                        cTooFewEncodedVarsErrorMessage
                );
            }
            message.append(decode_float_var(encoded_vars[encoded_vars_ix]));
            ++encoded_vars_ix;
        } else if (enum_to_underlying_type(ir::VariablePlaceholder::Integer) == c) {
            message.append(logtype, last_variable_end_pos, i - last_variable_end_pos);
            last_variable_end_pos = i + 1;
            if (encoded_vars_ix >= encoded_vars_length) {
                throw EncodingException(
                        ErrorCode_Corrupt,
                        __FILENAME__,
                        __LINE__,
                        cTooFewEncodedVarsErrorMessage
                );
            }
            message.append(decode_integer_var(encoded_vars[encoded_vars_ix]));
            ++encoded_vars_ix;
        } else if (enum_to_underlying_type(ir::VariablePlaceholder::Dictionary) == c) {
            message.append(logtype, last_variable_end_pos, i - last_variable_end_pos);
            last_variable_end_pos = i + 1;
            if (dictionary_var_bounds_ix >= dictionary_var_end_offsets_length) {
                throw EncodingException(
                        ErrorCode_Corrupt,
                        __FILENAME__,
                        __LINE__,
                        cTooFewDictionaryVarsErrorMessage
                );
            }
            auto end_pos = dictionary_var_end_offsets[dictionary_var_bounds_ix];
            message.append(
                    all_dictionary_vars,
                    dictionary_var_begin_pos,
                    end_pos - dictionary_var_begin_pos
            );
            dictionary_var_begin_pos = end_pos;
            ++dictionary_var_bounds_ix;
        }
    }
    // Add remainder
    if (last_variable_end_pos < logtype.length()) {
        message.append(logtype, last_variable_end_pos);
    }

    return message;
}

template <ir::VariablePlaceholder var_placeholder, typename encoded_variable_t>
bool wildcard_query_matches_any_encoded_var(
        std::string_view wildcard_query,
        std::string_view logtype,
        encoded_variable_t* encoded_vars,
        size_t encoded_vars_length
) {
    size_t encoded_vars_ix = 0;
    for (auto c : logtype) {
        if (enum_to_underlying_type(ir::VariablePlaceholder::Float) == c) {
            if (encoded_vars_ix >= encoded_vars_length) {
                throw EncodingException(
                        ErrorCode_Corrupt,
                        __FILENAME__,
                        __LINE__,
                        cTooFewEncodedVarsErrorMessage
                );
            }

            if constexpr (ir::VariablePlaceholder::Float == var_placeholder) {
                auto decoded_var = decode_float_var(encoded_vars[encoded_vars_ix]);
                if (clp::string_utils::wildcard_match_unsafe(decoded_var, wildcard_query)) {
                    return true;
                }
            }

            ++encoded_vars_ix;
        } else if (enum_to_underlying_type(ir::VariablePlaceholder::Integer) == c) {
            if (encoded_vars_ix >= encoded_vars_length) {
                throw EncodingException(
                        ErrorCode_Corrupt,
                        __FILENAME__,
                        __LINE__,
                        cTooFewEncodedVarsErrorMessage
                );
            }

            if constexpr (ir::VariablePlaceholder::Integer == var_placeholder) {
                auto decoded_var = decode_integer_var(encoded_vars[encoded_vars_ix]);
                if (clp::string_utils::wildcard_match_unsafe(decoded_var, wildcard_query)) {
                    return true;
                }
            }

            ++encoded_vars_ix;
        }
    }

    return false;
}

template <typename encoded_variable_t>
bool wildcard_match_encoded_vars(
        std::string_view logtype,
        encoded_variable_t* encoded_vars,
        size_t encoded_vars_length,
        std::string_view wildcard_var_placeholders,
        std::vector<std::string_view> const& wildcard_var_queries
) {
    // Validate arguments
    if (nullptr == encoded_vars) {
        throw EncodingException(
                ErrorCode_BadParam,
                __FILENAME__,
                __LINE__,
                cTooFewEncodedVarsErrorMessage
        );
    }
    if (wildcard_var_queries.size() != wildcard_var_placeholders.length()) {
        throw EncodingException(
                ErrorCode_BadParam,
                __FILENAME__,
                __LINE__,
                cTooFewEncodedVarsErrorMessage
        );
    }

    auto wildcard_var_queries_len = wildcard_var_queries.size();
    size_t var_ix = 0;
    size_t wildcard_var_ix = 0;
    for (auto c : logtype) {
        if (enum_to_underlying_type(ir::VariablePlaceholder::Float) == c) {
            if (var_ix >= encoded_vars_length) {
                throw EncodingException(
                        ErrorCode_Corrupt,
                        __FILENAME__,
                        __LINE__,
                        cTooFewEncodedVarsErrorMessage
                );
            }

            if (wildcard_var_placeholders[wildcard_var_ix] == c) {
                auto decoded_var = decode_float_var(encoded_vars[var_ix]);
                if (clp::string_utils::wildcard_match_unsafe(
                            decoded_var,
                            wildcard_var_queries[wildcard_var_ix]
                    ))
                {
                    ++wildcard_var_ix;
                    if (wildcard_var_ix == wildcard_var_queries_len) {
                        break;
                    }
                }
            }

            ++var_ix;
        } else if (enum_to_underlying_type(ir::VariablePlaceholder::Integer) == c) {
            if (var_ix >= encoded_vars_length) {
                throw EncodingException(
                        ErrorCode_Corrupt,
                        __FILENAME__,
                        __LINE__,
                        cTooFewEncodedVarsErrorMessage
                );
            }

            if (wildcard_var_placeholders[wildcard_var_ix] == c) {
                auto decoded_var = decode_integer_var(encoded_vars[var_ix]);
                if (clp::string_utils::wildcard_match_unsafe(
                            decoded_var,
                            wildcard_var_queries[wildcard_var_ix]
                    ))
                {
                    ++wildcard_var_ix;
                    if (wildcard_var_ix == wildcard_var_queries_len) {
                        break;
                    }
                }
            }

            ++var_ix;
        }
    }

    return (wildcard_var_queries_len == wildcard_var_ix);
}
}  // namespace glt::ffi

#endif  // GLT_FFI_ENCODING_METHODS_INC

#include "encoding_methods.hpp"

// C++ standard libraries
#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>

// Project headers
#include "../string_utils.hpp"
#include "../type_utils.hpp"

using std::string;
using std::string_view;

// Constants
constexpr unsigned long long cDigitsInRepresentableFloatBitMask = (1ULL << 54) - 1;

namespace ffi {
    /**
     * @param str
     * @return Whether the given string could be a multi-digit hex value
     */
    static bool could_be_multi_digit_hex_value (string_view str);

    /**
     * Checks if the given character is a delimiter
     * We treat everything *except* the following quoted characters as a
     * delimiter: "+-.0-9A-Z\_a-z"
     * @param c
     * @return Whether c is a delimiter
     */
    static bool is_delim (signed char c);

    static bool could_be_multi_digit_hex_value (const string_view str) {
        if (str.length() < 2) {
            return false;
        }

        return std::all_of(str.cbegin(), str.cend(), [] (char c) {
            return ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F') || ('0' <= c && c <= '9');
        });
    }

    /*
     * For performance, we rely on the ASCII ordering of characters to compare
     * ranges of characters at a time instead of comparing individual
     * characters
     */
    static bool is_delim (signed char c) {
        return !('+' == c || ('-' <= c && c <= '.') || ('0' <= c && c <= '9') ||
                 ('A' <= c && c <= 'Z') || '\\' == c || '_' == c || ('a' <= c && c <= 'z'));
    }

    bool is_variable_placeholder (char c) {
        return (enum_to_underlying_type(VariablePlaceholder::Integer) == c) ||
               (enum_to_underlying_type(VariablePlaceholder::Dictionary) == c) ||
               (enum_to_underlying_type(VariablePlaceholder::Float) == c);
    }

    bool get_bounds_of_next_var (const string_view str, size_t& begin_pos, size_t& end_pos,
                                 bool& contains_var_placeholder)
    {
        const auto msg_length = str.length();
        if (end_pos >= msg_length) {
            return false;
        }

        while (true) {
            begin_pos = end_pos;

            // Find next non-delimiter
            for (; begin_pos < msg_length; ++begin_pos) {
                auto c = str[begin_pos];
                if (false == is_delim(c)) {
                    break;
                } else if (enum_to_underlying_type(VariablePlaceholder::Integer) == c ||
                           enum_to_underlying_type(VariablePlaceholder::Dictionary) == c ||
                           enum_to_underlying_type(VariablePlaceholder::Float) == c)
                {
                    contains_var_placeholder = true;
                }
            }
            if (msg_length == begin_pos) {
                // Early exit for performance
                return false;
            }

            bool contains_decimal_digit = false;
            bool contains_alphabet = false;

            // Find next delimiter
            end_pos = begin_pos;
            for (; end_pos < msg_length; ++end_pos) {
                auto c = str[end_pos];
                if (is_decimal_digit(c)) {
                    contains_decimal_digit = true;
                } else if (is_alphabet(c)) {
                    contains_alphabet = true;
                } else if (is_delim(c)) {
                    break;
                }
            }

            string_view variable(str.cbegin() + begin_pos, end_pos - begin_pos);
            // Treat token as variable if:
            // - it contains a decimal digit, or
            // - it's directly preceded by an equals sign and contains an alphabet, or
            // - it could be a multi-digit hex value
            if (contains_decimal_digit ||
                (begin_pos > 0 && '=' == str[begin_pos - 1] && contains_alphabet) ||
                could_be_multi_digit_hex_value(variable)) {
                break;
            }
        }

        return (msg_length != begin_pos);
    }

    bool encode_float_string (string_view str, encoded_variable_t& encoded_var) {
        const auto value_length = str.length();
        if (0 == value_length) {
            // Can't convert an empty string
            return false;
        }

        size_t pos = 0;
        constexpr size_t cMaxDigitsInRepresentableFloatVar = 16;
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
        size_t decimal_point_pos = string::npos;
        uint64_t digits = 0;
        for (; pos < value_length; ++pos) {
            auto c = str[pos];
            if ('0' <= c && c <= '9') {
                digits *= 10;
                digits += (c - '0');
                ++num_digits;
            } else if (string::npos == decimal_point_pos && '.' == c) {
                decimal_point_pos = value_length - 1 - pos;
            } else {
                // Invalid character
                return false;
            }
        }
        if (string::npos == decimal_point_pos || 0 == decimal_point_pos || 0 == num_digits) {
            // No decimal point found, decimal point is after all digits, or no
            // digits found
            return false;
        }

        // Encode into 64 bits with the following format (from MSB to LSB):
        // -  1 bit : is negative
        // -  1 bit : unused
        // - 54 bits: The digits of the float without the decimal, as an
        //            integer
        // -  4 bits: # of decimal digits minus 1
        //     - This format can represent floats with between 1 and 16 decimal
        //       digits, so we use 4 bits and map the range [1, 16] to
        //       [0x0, 0xF]
        // -  4 bits: position of the decimal from the right minus 1
        //     - To see why the position is taken from the right, consider
        //       (1) "-123456789012345.6", (2) "-.1234567890123456", and
        //       (3) ".1234567890123456"
        //         - For (1), the decimal point is at index 16 from the left and
        //           index 1 from the right.
        //         - For (2), the decimal point is at index 1 from the left and
        //           index 16 from the right.
        //         - For (3), the decimal point is at index 0 from the left and
        //           index 16 from the right.
        //         - So if we take the decimal position from the left, it can
        //           range from 0 to 16 because of the negative sign. Whereas
        //           from the right, the negative sign is inconsequential.
        //     - Thus, we use 4 bits and map the range [1, 16] to [0x0, 0xF].
        uint64_t encoded_float = 0;
        if (is_negative) {
            encoded_float = 1;
        }
        encoded_float <<= 55;  // 1 unused + 54 for digits of the float
        encoded_float |= digits & cDigitsInRepresentableFloatBitMask;
        encoded_float <<= 4;
        encoded_float |= (num_digits - 1) & 0x0F;
        encoded_float <<= 4;
        encoded_float |= (decimal_point_pos - 1) & 0x0F;
        encoded_var = bit_cast<encoded_variable_t>(encoded_float);

        return true;
    }

    string decode_float_var (encoded_variable_t encoded_var) {
        string value;

        auto encoded_float = bit_cast<uint64_t>(encoded_var);

        // Decode according to the format described in encode_float_string
        uint8_t decimal_pos = (encoded_float & 0x0F) + 1;
        encoded_float >>= 4;
        uint8_t num_digits = (encoded_float & 0x0F) + 1;
        encoded_float >>= 4;
        uint64_t digits = encoded_float & cDigitsInRepresentableFloatBitMask;
        encoded_float >>= 55;
        bool is_negative = encoded_float > 0;

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
        for (; pos > (value_length - 1 - decimal_pos) && digits > 0; --pos) {
            value[pos] = (char)('0' + (digits % 10));
            digits /= 10;
            --num_chars_to_process;
        }

        if (digits > 0) {
            // Skip decimal since it's added at the end
            --pos;
            --num_chars_to_process;

            while (digits > 0) {
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
        value[value_length - 1 - decimal_pos] = '.';

        return value;
    }

    bool encode_integer_string (string_view str, encoded_variable_t& encoded_var) {
        size_t length = str.length();
        if (0 == length) {
            // Empty string cannot be converted
            return false;
        }

        // Ensure start of value is an integer with no zero-padding or positive
        // sign
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

        int64_t result;
        if (false == convert_string_to_int64(str, result)) {
            // Conversion failed
            return false;
        } else {
            encoded_var = result;
        }

        return true;
    }

    std::string decode_integer_var (encoded_variable_t encoded_var) {
        return std::to_string(encoded_var);
    }

    bool encode_message (std::string_view message, std::string& logtype,
                         std::vector<encoded_variable_t>& encoded_vars,
                         std::vector<int32_t>& dictionary_var_bounds)
    {
        size_t begin_pos = 0;
        size_t end_pos = 0;
        bool message_contains_variable_placeholder = false;
        size_t last_var_end_pos = 0;
        logtype.clear();
        logtype.reserve(message.length());
        while (get_bounds_of_next_var(message, begin_pos, end_pos,
                                      message_contains_variable_placeholder))
        {
            if (message_contains_variable_placeholder) {
                return false;
            }

            // Append the content between the last variable and this one
            logtype.append(message, last_var_end_pos, begin_pos - last_var_end_pos);
            last_var_end_pos = end_pos;

            // Encode the variable
            string_view var_string(&message[begin_pos], end_pos - begin_pos);
            encoded_variable_t encoded_variable;
            if (encode_float_string(var_string, encoded_variable)) {
                logtype += enum_to_underlying_type(VariablePlaceholder::Float);
                encoded_vars.push_back(encoded_variable);
            } else if (encode_integer_string(var_string, encoded_variable)) {
                logtype += enum_to_underlying_type(VariablePlaceholder::Integer);
                encoded_vars.push_back(encoded_variable);
            } else {
                logtype += enum_to_underlying_type(VariablePlaceholder::Dictionary);
                dictionary_var_bounds.push_back(begin_pos);
                dictionary_var_bounds.push_back(end_pos);
            }
        }
        // Append any remaining message content to the logtype
        if (last_var_end_pos < message.length()) {
            // Ensure the remaining content doesn't contain a variable
            // placeholder
            message_contains_variable_placeholder = std::any_of(
                    message.cbegin() + last_var_end_pos, message.cend(),
                    is_variable_placeholder);
            if (message_contains_variable_placeholder) {
                return false;
            }
            logtype.append(message, last_var_end_pos);
        }

        return true;
    }

    string decode_message (string_view logtype, encoded_variable_t* encoded_vars,
                           size_t encoded_vars_length, string_view all_dictionary_vars,
                           const int32_t* dictionary_var_end_offsets,
                           size_t dictionary_var_end_offsets_length)
    {
        string message;
        size_t last_variable_end_pos = 0;
        size_t dictionary_var_begin_pos = 0;
        size_t dictionary_var_bounds_ix = 0;
        size_t encoded_vars_ix = 0;
        for (size_t i = 0; i < logtype.length(); ++i) {
            auto c = logtype[i];
            if (enum_to_underlying_type(VariablePlaceholder::Float) == c) {
                message.append(logtype, last_variable_end_pos, i - last_variable_end_pos);
                last_variable_end_pos = i + 1;
                if (encoded_vars_ix >= encoded_vars_length) {
                    throw EncodingException(ErrorCode_Corrupt, __FILENAME__, __LINE__,
                                            cTooFewEncodedVarsErrorMessage);
                }
                message.append(decode_float_var(encoded_vars[encoded_vars_ix]));
                ++encoded_vars_ix;
            } else if (enum_to_underlying_type(VariablePlaceholder::Integer) == c) {
                message.append(logtype, last_variable_end_pos, i - last_variable_end_pos);
                last_variable_end_pos = i + 1;
                if (encoded_vars_ix >= encoded_vars_length) {
                    throw EncodingException(ErrorCode_Corrupt, __FILENAME__, __LINE__,
                                            cTooFewEncodedVarsErrorMessage);
                }
                message.append(decode_integer_var(encoded_vars[encoded_vars_ix]));
                ++encoded_vars_ix;
            } else if (enum_to_underlying_type(VariablePlaceholder::Dictionary) == c) {
                message.append(logtype, last_variable_end_pos, i - last_variable_end_pos);
                last_variable_end_pos = i + 1;
                if (dictionary_var_bounds_ix >= dictionary_var_end_offsets_length) {
                    throw EncodingException(ErrorCode_Corrupt, __FILENAME__, __LINE__,
                                            cTooFewDictionaryVarsErrorMessage);
                }
                auto end_pos = dictionary_var_end_offsets[dictionary_var_bounds_ix];
                message.append(all_dictionary_vars, dictionary_var_begin_pos,
                               end_pos - dictionary_var_begin_pos);
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
}

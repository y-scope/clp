#include "EncodedVariableInterpreter.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <string>
#include <string_view>

#include <string_utils/string_utils.hpp>

#include "Defs.h"
#include "type_utils.hpp"

using clp::ffi::cEightByteEncodedFloatDigitsBitMask;
using std::string;
using std::string_view;

namespace clp {
variable_dictionary_id_t EncodedVariableInterpreter::decode_var_dict_id(
        encoded_variable_t encoded_var
) {
    return bit_cast<variable_dictionary_id_t>(encoded_var);
}

bool EncodedVariableInterpreter::convert_string_to_representable_integer_var(
        string_view value,
        encoded_variable_t& encoded_var
) {
    size_t length = value.length();
    if (0 == length) {
        // Empty string cannot be converted
        return false;
    }

    // Ensure start of value is an integer with no zero-padding or positive sign
    if ('-' == value[0]) {
        // Ensure first character after sign is a non-zero integer
        if (length < 2 || value[1] < '1' || '9' < value[1]) {
            return false;
        }
    } else {
        // Ensure first character is a digit
        if (value[0] < '0' || '9' < value[0]) {
            return false;
        }

        // Ensure value is not zero-padded
        if (length > 1 && '0' == value[0]) {
            return false;
        }
    }

    int64_t result;
    if (false == string_utils::convert_string_to_int(value, result)) {
        // Conversion failed
        return false;
    } else {
        encoded_var = result;
    }

    return true;
}

bool EncodedVariableInterpreter::convert_string_to_representable_float_var(
        string_view value,
        encoded_variable_t& encoded_var
) {
    if (value.empty()) {
        // Can't convert an empty string
        return false;
    }

    size_t pos = 0;
    constexpr size_t cMaxDigitsInRepresentableFloatVar = 16;
    // +1 for decimal point
    size_t max_length = cMaxDigitsInRepresentableFloatVar + 1;

    // Check for a negative sign
    bool is_negative = false;
    if ('-' == value[pos]) {
        is_negative = true;
        ++pos;
        // Include sign in max length
        ++max_length;
    }

    // Check if value can be represented in encoded format
    if (value.length() > max_length) {
        return false;
    }

    size_t num_digits = 0;
    size_t decimal_point_pos = string::npos;
    uint64_t digits = 0;
    for (; pos < value.length(); ++pos) {
        auto c = value[pos];
        if ('0' <= c && c <= '9') {
            digits *= 10;
            digits += (c - '0');
            ++num_digits;
        } else if (string::npos == decimal_point_pos && '.' == c) {
            decimal_point_pos = value.length() - 1 - pos;
        } else {
            // Invalid character
            return false;
        }
    }
    if (string::npos == decimal_point_pos || 0 == decimal_point_pos || 0 == num_digits) {
        // No decimal point found, decimal point is after all digits, or no digits found
        return false;
    }

    // Encode into 64 bits with the following format (from MSB to LSB):
    // -  1 bit : is negative
    // -  1 bit : unused
    // - 54 bits: The digits of the float without the decimal, as an integer
    // -  4 bits: # of decimal digits minus 1
    //     - This format can represent floats with between 1 and 16 decimal digits, so we use 4 bits
    //       and map the range [1, 16] to [0x0, 0xF]
    // -  4 bits: position of the decimal from the right minus 1
    //     - To see why the position is taken from the right, consider
    //       (1) "-123456789012345.6", (2) "-.1234567890123456", and
    //       (3) ".1234567890123456"
    //         - For (1), the decimal point is at index 16 from the left and index 1 from the right.
    //         - For (2), the decimal point is at index 1 from the left and index 16 from the right.
    //         - For (3), the decimal point is at index 0 from the left and index 16 from the right.
    //         - So if we take the decimal position from the left, it can range from 0 to 16 because
    //           of the negative sign. Whereas from the right, the negative sign is inconsequential.
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
    encoded_var = bit_cast<encoded_variable_t>(encoded_float);

    return true;
}

void EncodedVariableInterpreter::convert_encoded_float_to_string(
        encoded_variable_t encoded_var,
        string& value
) {
    auto encoded_float = bit_cast<uint64_t>(encoded_var);

    // Decode according to the format described in
    // EncodedVariableInterpreter::convert_string_to_representable_float_var
    uint8_t decimal_pos = (encoded_float & 0x0F) + 1;
    encoded_float >>= 4;
    uint8_t num_digits = (encoded_float & 0x0F) + 1;
    encoded_float >>= 4;
    uint64_t digits = encoded_float & cEightByteEncodedFloatDigitsBitMask;
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
}

auto EncodedVariableInterpreter::wildcard_string_could_be_representable_integer_var(
        std::string_view value
) -> bool {
    if (value.empty()) {
        return false;
    }
    return false == std::ranges::any_of(value, [](char const c) -> bool {
               return false == (('0' <= c && c <= '9') || c == '-' || c == '?' || c == '*');
           });
}

auto EncodedVariableInterpreter::wildcard_string_could_be_representable_float_var(
        std::string_view value
) -> bool {
    if (value.empty()) {
        return false;
    }
    return false == std::ranges::any_of(value, [](char const c) -> bool {
               return false
                      == (('0' <= c && c <= '9') || c == '-' || c == '.' || c == '?' || c == '*');
           });
}

encoded_variable_t EncodedVariableInterpreter::encode_var_dict_id(variable_dictionary_id_t id) {
    return bit_cast<encoded_variable_t>(id);
}
}  // namespace clp

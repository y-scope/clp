// Code from CLP

#include "VariableEncoder.hpp"

namespace clp_s {
void VariableEncoder::encode_and_add_to_dictionary(
        std::string const& message,
        LogTypeDictionaryEntry& logtype_dict_entry,
        VariableDictionaryWriter& var_dict,
        std::vector<int64_t>& encoded_vars
) {
    // Extract all variables and add to dictionary while building logtype
    size_t var_begin_pos = 0;
    size_t var_end_pos = 0;
    std::string var_str;
    logtype_dict_entry.clear();
    // To avoid reallocating the logtype as we append to it, reserve enough space to hold the entire
    // message
    logtype_dict_entry.reserve_constant_length(message.length());
    while (logtype_dict_entry.parse_next_var(message, var_begin_pos, var_end_pos, var_str)) {
        // Encode variable
        int64_t encoded_var;
        if (convert_string_to_representable_integer_var(var_str, encoded_var)) {
            logtype_dict_entry.add_non_double_var();
        } else if (convert_string_to_representable_double_var(var_str, encoded_var)) {
            logtype_dict_entry.add_double_var();
        } else {
            // Variable string looks like a dictionary variable, so encode it as so
            uint64_t id;
            var_dict.add_entry(var_str, id);
            encoded_var = encode_var_dict_id(id);

            logtype_dict_entry.add_non_double_var();
        }

        encoded_vars.push_back(encoded_var);
    }
}

bool VariableEncoder::convert_string_to_int64(std::string const& raw, int64_t& converted) {
    if (raw.empty()) {
        // Can't convert an empty string
        return false;
    }

    char const* c_str = raw.c_str();
    char* endptr;
    // Reset errno so we can detect if it's been set
    errno = 0;
    int64_t raw_as_int = strtoll(c_str, &endptr, 10);
    if (endptr - c_str != raw.length() || (LLONG_MAX == raw_as_int && ERANGE == errno)) {
        // Conversion failed
        return false;
    }
    converted = raw_as_int;
    return true;
}

bool VariableEncoder::convert_string_to_representable_integer_var(
        std::string const& value,
        int64_t& encoded_var
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
    // Conversion failed or value is in dictionary variable range, so cannot be converted
    if (false == convert_string_to_int64(value, result) || result >= cVarDictIdRangeBegin) {
        return false;
    } else {
        encoded_var = result;
    }

    return true;
}

bool VariableEncoder::convert_string_to_representable_double_var(
        std::string const& value,
        int64_t& encoded_var
) {
    if (value.empty()) {
        // Can't convert an empty string
        return false;
    }

    size_t pos = 0;
    constexpr size_t cMaxDigitsInRepresentableDoubleVar = 16;
    // +1 for decimal point
    size_t max_length = cMaxDigitsInRepresentableDoubleVar + 1;

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
    size_t decimal_point_pos = std::string::npos;
    uint64_t digits = 0;
    for (; pos < value.length(); ++pos) {
        auto c = value[pos];
        if ('0' <= c && c <= '9') {
            digits *= 10;
            digits += (c - '0');
            ++num_digits;
        } else if (std::string::npos == decimal_point_pos && '.' == c) {
            decimal_point_pos = value.length() - 1 - pos;
        } else {
            // Invalid character
            return false;
        }
    }
    if (std::string::npos == decimal_point_pos || 0 == decimal_point_pos || 0 == num_digits) {
        // No decimal point found, decimal point is after all digits, or no digits found
        return false;
    }

    // Encode into 64 bits with the following format (from MSB to LSB):
    // -  1 bit : is negative
    // -  4 bits: # of decimal digits minus 1
    //     - This format can represent doubles with between 1 and 16 decimal digits, so we use 4
    //     bits and map the range [1, 16] to [0x0, 0xF]
    // -  4 bits: position of the decimal from the right minus 1
    //     - To see why the position is taken from the right, consider (1) "-123456789012345.6", (2)
    //     "-.1234567890123456", and (3) ".1234567890123456"
    //         - For (1), the decimal point is at index 16 from the left and index 1 from the right.
    //         - For (2), the decimal point is at index 1 from the left and index 16 from the right.
    //         - For (3), the decimal point is at index 0 from the left and index 16 from the right.
    //         - So if we take the decimal position from the left, it can range from 0 to 16 because
    //         of the negative sign. Whereas from the right, the
    //           negative sign is inconsequential.
    //     - Thus, we use 4 bits and map the range [1, 16] to [0x0, 0xF].
    // -  1 bit : unused
    // - 54 bits: The digits of the double without the decimal, as an integer
    uint64_t encoded_double = 0;
    if (is_negative) {
        encoded_double = 1;
    }
    encoded_double <<= 4;
    encoded_double |= (num_digits - 1) & 0x0F;
    encoded_double <<= 4;
    encoded_double |= (decimal_point_pos - 1) & 0x0F;
    encoded_double <<= 55;
    encoded_double |= digits & 0x003F'FFFF'FFFF'FFFF;
    static_assert(
            sizeof(encoded_var) == sizeof(encoded_double),
            "sizeof(encoded_var) != sizeof(encoded_double)"
    );
    // NOTE: We use memcpy rather than reinterpret_cast to avoid violating strict aliasing; a smart
    // compiler should optimize it to a register move
    std::memcpy(&encoded_var, &encoded_double, sizeof(encoded_double));

    return true;
}
}  // namespace clp_s

#include "EncodedVariableInterpreter.hpp"

// C++ standard libraries
#include <cassert>
#include <cmath>

// Project headers
#include "Defs.h"
#include "ffi/encoding_methods.hpp"
#include "spdlog_with_specializations.hpp"
#include "string_utils.hpp"
#include "type_utils.hpp"

using ffi::cEightByteEncodedFloatDigitsBitMask;
using std::string;
using std::unordered_set;
using std::vector;

variable_dictionary_id_t EncodedVariableInterpreter::decode_var_dict_id (encoded_variable_t encoded_var) {
    return bit_cast<variable_dictionary_id_t>(encoded_var);
}

bool EncodedVariableInterpreter::convert_string_to_representable_integer_var (const string& value, encoded_variable_t& encoded_var) {
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
    if (false == convert_string_to_int(value, result)) {
        // Conversion failed
        return false;
    } else {
        encoded_var = result;
    }

    return true;
}

bool EncodedVariableInterpreter::convert_string_to_representable_float_var (
        const string& value, encoded_variable_t& encoded_var)
{
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
    encoded_float |= digits & cEightByteEncodedFloatDigitsBitMask;
    encoded_float <<= 4;
    encoded_float |= (num_digits - 1) & 0x0F;
    encoded_float <<= 4;
    encoded_float |= (decimal_point_pos - 1) & 0x0F;
    encoded_var = bit_cast<encoded_variable_t>(encoded_float);

    return true;
}

void EncodedVariableInterpreter::convert_encoded_float_to_string (encoded_variable_t encoded_var,
                                                                  string& value) {
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

void EncodedVariableInterpreter::encode_and_add_to_dictionary (const string& message, LogTypeDictionaryEntry& logtype_dict_entry,
                                                               VariableDictionaryWriter& var_dict, vector<encoded_variable_t>& encoded_vars,
                                                               vector<variable_dictionary_id_t>& var_ids)
{
    // Extract all variables and add to dictionary while building logtype
    size_t var_begin_pos = 0;
    size_t var_end_pos = 0;
    string var_str;
    logtype_dict_entry.clear();
    // To avoid reallocating the logtype as we append to it, reserve enough space to hold the entire message
    logtype_dict_entry.reserve_constant_length(message.length());
    while (logtype_dict_entry.parse_next_var(message, var_begin_pos, var_end_pos, var_str)) {
        // Encode variable
        encoded_variable_t encoded_var;
        if (convert_string_to_representable_integer_var(var_str, encoded_var)) {
            logtype_dict_entry.add_int_var();
        } else if (convert_string_to_representable_float_var(var_str, encoded_var)) {
            logtype_dict_entry.add_float_var();
        } else {
            // Variable string looks like a dictionary variable, so encode it as so
            variable_dictionary_id_t id;
            var_dict.add_entry(var_str, id);
            encoded_var = encode_var_dict_id(id);
            var_ids.push_back(id);

            logtype_dict_entry.add_dictionary_var();
        }

        encoded_vars.push_back(encoded_var);
    }
}

bool EncodedVariableInterpreter::decode_variables_into_message (const LogTypeDictionaryEntry& logtype_dict_entry, const VariableDictionaryReader& var_dict,
                                                                const vector<encoded_variable_t>& encoded_vars, string& decompressed_msg)
{
    size_t num_vars_in_logtype = logtype_dict_entry.get_num_vars();

    // Ensure the number of variables in the logtype matches the number of encoded variables given
    const auto& logtype_value = logtype_dict_entry.get_value();
    if (num_vars_in_logtype != encoded_vars.size()) {
        SPDLOG_ERROR("EncodedVariableInterpreter: Logtype '{}' contains {} variables, but {} were given for decoding.", logtype_value.c_str(),
                     num_vars_in_logtype, encoded_vars.size());
        return false;
    }

    LogTypeDictionaryEntry::VarDelim var_delim;
    size_t constant_begin_pos = 0;
    string float_str;
    variable_dictionary_id_t var_dict_id;
    for (size_t i = 0; i < num_vars_in_logtype; ++i) {
        size_t var_position = logtype_dict_entry.get_var_info(i, var_delim);

        // Add the constant that's between the last variable and this one
        decompressed_msg.append(logtype_value, constant_begin_pos,
                                var_position - constant_begin_pos);
        switch (var_delim) {
            case LogTypeDictionaryEntry::VarDelim::Integer:
                decompressed_msg += std::to_string(encoded_vars[i]);
                break;
            case LogTypeDictionaryEntry::VarDelim::Float:
                convert_encoded_float_to_string(encoded_vars[i], float_str);
                decompressed_msg += float_str;
                break;
            case LogTypeDictionaryEntry::VarDelim::Dictionary:
                var_dict_id = decode_var_dict_id(encoded_vars[i]);
                decompressed_msg += var_dict.get_value(var_dict_id);
                break;
            default:
                SPDLOG_ERROR(
                    "EncodedVariableInterpreter: Logtype '{}' contains "
                    "unexpected variable placeholder 0x{:x}",
                    logtype_value,
                    enum_to_underlying_type(var_delim));
                return false;
        }
        // Move past the variable delimiter
        constant_begin_pos = var_position + 1;
    }
    // Append remainder of logtype, if any
    if (constant_begin_pos < logtype_value.length()) {
        decompressed_msg.append(logtype_value, constant_begin_pos, string::npos);
    }

    return true;
}

bool EncodedVariableInterpreter::encode_and_search_dictionary (const string& var_str, const VariableDictionaryReader& var_dict, bool ignore_case,
                                                               string& logtype, SubQuery& sub_query)
{
    size_t length = var_str.length();
    if (0 == length) {
        throw OperationFailed(ErrorCode_BadParam, __FILENAME__, __LINE__);
    }

    encoded_variable_t encoded_var;
    if (convert_string_to_representable_integer_var(var_str, encoded_var)) {
        LogTypeDictionaryEntry::add_int_var(logtype);
        sub_query.add_non_dict_var(encoded_var);
    } else if (convert_string_to_representable_float_var(var_str, encoded_var)) {
        LogTypeDictionaryEntry::add_float_var(logtype);
        sub_query.add_non_dict_var(encoded_var);
    } else {
        auto entry = var_dict.get_entry_matching_value(var_str, ignore_case);
        if (nullptr == entry) {
            // Not in dictionary
            return false;
        }
        encoded_var = encode_var_dict_id(entry->get_id());

        LogTypeDictionaryEntry::add_dict_var(logtype);
        sub_query.add_dict_var(encoded_var, entry);
    }

    return true;
}

bool EncodedVariableInterpreter::wildcard_search_dictionary_and_get_encoded_matches (const std::string& var_wildcard_str,
                                                                                     const VariableDictionaryReader& var_dict,
                                                                                     bool ignore_case, SubQuery& sub_query)
{
    // Find matches
    unordered_set<const VariableDictionaryEntry*> var_dict_entries;
    var_dict.get_entries_matching_wildcard_string(var_wildcard_str, ignore_case, var_dict_entries);
    if (var_dict_entries.empty()) {
        // Not in dictionary
        return false;
    }

    // Encode matches
    unordered_set<encoded_variable_t> encoded_vars;
    for (auto entry : var_dict_entries) {
        encoded_vars.insert(encode_var_dict_id(entry->get_id()));
    }

    sub_query.add_imprecise_dict_var(encoded_vars, var_dict_entries);

    return true;
}

encoded_variable_t EncodedVariableInterpreter::encode_var_dict_id (variable_dictionary_id_t id) {
    return bit_cast<encoded_variable_t>(id);
}

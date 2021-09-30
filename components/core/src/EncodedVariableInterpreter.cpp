#include "EncodedVariableInterpreter.hpp"

// C++ standard libraries
#include <cassert>
#include <cmath>

// spdlog
#include <spdlog/spdlog.h>

// Project headers
#include "Defs.h"
#include "Utils.hpp"

using std::string;
using std::unordered_set;
using std::vector;

// Constants
// 1 sign + LogTypeDictionaryEntry::cMaxDigitsInRepresentableDoubleVar + 1 decimal point
static const size_t cMaxCharsInRepresentableDoubleVar = LogTypeDictionaryEntry::cMaxDigitsInRepresentableDoubleVar + 2;

encoded_variable_t EncodedVariableInterpreter::get_var_dict_id_range_begin () {
    return m_var_dict_id_range_begin;
}

encoded_variable_t EncodedVariableInterpreter::get_var_dict_id_range_end () {
    return m_var_dict_id_range_end;
}

bool EncodedVariableInterpreter::is_var_dict_id (encoded_variable_t encoded_var) {
    return (m_var_dict_id_range_begin <= encoded_var && encoded_var < m_var_dict_id_range_end);
}

variable_dictionary_id_t EncodedVariableInterpreter::decode_var_dict_id (encoded_variable_t encoded_var) {
    variable_dictionary_id_t id = encoded_var - m_var_dict_id_range_begin;
    return id;
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
    if (false == convert_string_to_int64(value, result)) {
        // Conversion failed
        return false;
    } else if (result >= m_var_dict_id_range_begin) {
        // Value is in dictionary variable range, so cannot be converted
        return false;
    } else {
        encoded_var = result;
    }

    return true;
}

bool EncodedVariableInterpreter::convert_string_to_representable_double_var (const string& value, uint8_t& num_integer_digits, uint8_t& num_fractional_digits,
                                                                             encoded_variable_t& encoded_var)
{
    size_t length = value.length();

    // Check for preceding negative sign
    size_t first_digit_pos = 0;
    if (first_digit_pos < length && '-' == value[first_digit_pos]) {
        ++first_digit_pos;

        if (length > cMaxCharsInRepresentableDoubleVar) {
            // Too many characters besides sign to represent precisely
            return false;
        }
    } else {
        // No negative sign, so check against max size - 1
        if (length > cMaxCharsInRepresentableDoubleVar - 1) {
            // Too many characters to represent precisely
            return false;
        }
    }

    // Find decimal point
    size_t decimal_point_pos = string::npos;
    for (size_t i = first_digit_pos; i < length; ++i) {
        char c = value[i];
        if ('.' == c) {
            decimal_point_pos = i;
            break;
        } else if (!('0' <= c && c <= '9')) {
            // Unrepresentable double character
            return false;
        }
    }
    if (string::npos == decimal_point_pos) {
        // Decimal point doesn't exist
        return false;
    }

    num_integer_digits = decimal_point_pos - first_digit_pos;

    // Check that remainder of string is purely numbers
    for (size_t i = decimal_point_pos + 1; i < length; ++i) {
        char c = value[i];
        if (!('0' <= c && c <= '9')) {
            return false;
        }
    }

    num_fractional_digits = length - (decimal_point_pos + 1);

    double result;
    if (false == convert_string_to_double(value, result)) {
        // Conversion failed
        return false;
    } else {
        encoded_var = *reinterpret_cast<encoded_variable_t*>(&result);
    }

    return true;
}

void EncodedVariableInterpreter::encode_and_add_to_dictionary (const string& message, LogTypeDictionaryEntry& logtype_dict_entry,
                                                               VariableDictionaryWriter& var_dict, vector<encoded_variable_t>& encoded_vars)
{
    // Extract all variables and add to dictionary while building logtype
    size_t tok_begin_pos = 0;
    size_t next_delim_pos = 0;
    size_t last_var_end_pos = 0;
    string var_str;
    logtype_dict_entry.clear();
    // To avoid reallocating the logtype as we append to it, reserve enough space to hold the entire message
    logtype_dict_entry.reserve_constant_length(message.length());
    while (logtype_dict_entry.parse_next_var(message, tok_begin_pos, next_delim_pos, last_var_end_pos, var_str)) {
        // Encode variable
        encoded_variable_t encoded_var;
        uint8_t num_integer_digits;
        uint8_t num_fractional_digits;
        if (convert_string_to_representable_integer_var(var_str, encoded_var)) {
            logtype_dict_entry.add_non_double_var();
        } else if (convert_string_to_representable_double_var(var_str, num_integer_digits, num_fractional_digits, encoded_var)) {
            logtype_dict_entry.add_double_var(num_integer_digits, num_fractional_digits);
        } else {
            // Variable string looks like a dictionary variable, so encode it as so
            variable_dictionary_id_t id;
            var_dict.add_occurrence(var_str, id);
            encoded_var = encode_var_dict_id(id);

            logtype_dict_entry.add_non_double_var();
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
    uint8_t num_integer_digits;
    uint8_t num_fractional_digits;
    size_t constant_begin_pos = 0;
    char double_str[cMaxCharsInRepresentableDoubleVar + 1];
    for (size_t i = 0; i < num_vars_in_logtype; ++i) {
        size_t var_position = logtype_dict_entry.get_var_info(i, var_delim, num_integer_digits, num_fractional_digits);

        // Add the constant that's between the last variable and this one
        decompressed_msg.append(logtype_value, constant_begin_pos, var_position - constant_begin_pos);

        if (LogTypeDictionaryEntry::VarDelim::NonDouble == var_delim) {
            if (!is_var_dict_id(encoded_vars[i])) {
                decompressed_msg += std::to_string(encoded_vars[i]);
            } else {
                auto var_dict_id = decode_var_dict_id(encoded_vars[i]);
                decompressed_msg += var_dict.get_value(var_dict_id);
            }

            // Move past the variable delimiter
            constant_begin_pos = var_position + 1;
        } else { // LogTypeDictionaryEntry::VarDelim::Double == var_delim
            double var_as_double = *reinterpret_cast<const double*>(&encoded_vars[i]);
            int double_str_length = num_integer_digits + 1 + num_fractional_digits;
            if (std::signbit(var_as_double)) {
                ++double_str_length;
            }
            snprintf(double_str, sizeof(double_str), "%0*.*f", double_str_length, num_fractional_digits, var_as_double);

            decompressed_msg += double_str;

            // Move past the variable delimiter and the double's precision
            constant_begin_pos = var_position + 2;
        }
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

    uint8_t num_integer_digits;
    uint8_t num_fractional_digits;
    encoded_variable_t encoded_var;
    if (convert_string_to_representable_integer_var(var_str, encoded_var)) {
        LogTypeDictionaryEntry::add_non_double_var(logtype);
        sub_query.add_non_dict_var(encoded_var);
    } else if (convert_string_to_representable_double_var(var_str, num_integer_digits, num_fractional_digits, encoded_var)) {
        LogTypeDictionaryEntry::add_double_var(num_integer_digits, num_fractional_digits, logtype);
        sub_query.add_non_dict_var(encoded_var);
    } else {
        auto entry = var_dict.get_entry_matching_value(var_str, ignore_case);
        if (nullptr == entry) {
            // Not in dictionary
            return false;
        }
        encoded_var = encode_var_dict_id(entry->get_id());

        LogTypeDictionaryEntry::add_non_double_var(logtype);
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
    return (encoded_variable_t)id + m_var_dict_id_range_begin;
}

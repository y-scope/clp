// Code from CLP

#include "VariableDecoder.hpp"

namespace clp_s {
bool VariableDecoder::decode_variables_into_message(
        LogTypeDictionaryEntry const& logtype_dict_entry,
        VariableDictionaryReader const& var_dict,
        UnalignedMemSpan<int64_t> encoded_vars,
        std::string& decompressed_msg
) {
    size_t num_vars_in_logtype = logtype_dict_entry.get_num_vars();

    // Ensure the number of variables in the logtype matches the number of encoded variables given
    auto const& logtype_value = logtype_dict_entry.get_value();
    if (num_vars_in_logtype != encoded_vars.size()) {
        SPDLOG_ERROR(
                "VariableDecoder: Logtype '{}' contains {} variables, but {} were given for "
                "decoding.",
                logtype_value.c_str(),
                num_vars_in_logtype,
                encoded_vars.size()
        );
        return false;
    }

    LogTypeDictionaryEntry::VarDelim var_delim;
    size_t constant_begin_pos = 0;
    std::string double_str;
    for (size_t i = 0; i < num_vars_in_logtype; ++i) {
        size_t var_position = logtype_dict_entry.get_var_info(i, var_delim);

        // Add the constant that's between the last variable and this one
        decompressed_msg
                .append(logtype_value, constant_begin_pos, var_position - constant_begin_pos);

        if (LogTypeDictionaryEntry::VarDelim::NonDouble == var_delim) {
            if (false == is_var_dict_id(encoded_vars[i])) {
                decompressed_msg += std::to_string(encoded_vars[i]);
            } else {
                auto var_dict_id = decode_var_dict_id(encoded_vars[i]);
                decompressed_msg += var_dict.get_value(var_dict_id);
            }
        } else {  // LogTypeDictionaryEntry::VarDelim::Double == var_delim
            convert_encoded_double_to_string(encoded_vars[i], double_str);

            decompressed_msg += double_str;
        }
        // Move past the variable delimiter
        constant_begin_pos = var_position + 1;
    }
    // Append remainder of logtype, if any
    if (constant_begin_pos < logtype_value.length()) {
        decompressed_msg.append(logtype_value, constant_begin_pos, std::string::npos);
    }

    return true;
}

void VariableDecoder::convert_encoded_double_to_string(int64_t encoded_var, std::string& value) {
    uint64_t encoded_double;
    static_assert(
            sizeof(encoded_double) == sizeof(encoded_var),
            "sizeof(encoded_double) != sizeof(encoded_var)"
    );
    // NOTE: We use memcpy rather than reinterpret_cast to avoid violating strict aliasing; a smart
    // compiler should optimize it to a register move
    std::memcpy(&encoded_double, &encoded_var, sizeof(encoded_var));

    // Decode according to the format described in
    // VariableDecoder::convert_string_to_representable_double_var
    uint64_t digits = encoded_double & 0x003F'FFFF'FFFF'FFFF;
    encoded_double >>= 55;
    uint8_t decimal_pos = (encoded_double & 0x0F) + 1;
    encoded_double >>= 4;
    uint8_t num_digits = (encoded_double & 0x0F) + 1;
    encoded_double >>= 4;
    bool is_negative = encoded_double > 0;

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
}  // namespace clp_s

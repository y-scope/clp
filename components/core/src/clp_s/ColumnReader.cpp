#include "ColumnReader.hpp"

#include "../clp/EncodedVariableInterpreter.hpp"
#include "BufferViewReader.hpp"
#include "ColumnWriter.hpp"
#include "Utils.hpp"

namespace clp_s {
void Int64ColumnReader::load(BufferViewReader& reader, uint64_t num_messages) {
    m_values = reader.read_unaligned_span<int64_t>(num_messages);
}

std::variant<int64_t, double, std::string, uint8_t> Int64ColumnReader::extract_value(
        uint64_t cur_message
) {
    return m_values[cur_message];
}

void DeltaEncodedInt64ColumnReader::load(BufferViewReader& reader, uint64_t num_messages) {
    m_values = reader.read_unaligned_span<int64_t>(num_messages);
    if (num_messages > 0) {
        m_cur_idx = 0;
        m_cur_value = m_values[0];
    }
}

int64_t DeltaEncodedInt64ColumnReader::get_value_at_idx(size_t idx) {
    if (m_cur_idx == idx) {
        return m_cur_value;
    }
    if (idx > m_cur_idx) {
        for (; m_cur_idx < idx; ++m_cur_idx) {
            m_cur_value += m_values[m_cur_idx + 1];
        }
        return m_cur_value;
    }
    for (; m_cur_idx > idx; --m_cur_idx) {
        m_cur_value -= m_values[m_cur_idx];
    }
    return m_cur_value;
}

std::variant<int64_t, double, std::string, uint8_t> DeltaEncodedInt64ColumnReader::extract_value(
        uint64_t cur_message
) {
    return get_value_at_idx(cur_message);
}

void FloatColumnReader::load(BufferViewReader& reader, uint64_t num_messages) {
    m_values = reader.read_unaligned_span<double>(num_messages);
}

void FormattedFloatColumnReader::load(BufferViewReader& reader, uint64_t num_messages) {
    m_values = reader.read_unaligned_span<double>(num_messages);
    m_formats = reader.read_unaligned_span<uint16_t>(num_messages);
}

void
Int64ColumnReader::extract_string_value_into_buffer(uint64_t cur_message, std::string& buffer) {
    buffer.append(std::to_string(m_values[cur_message]));
}

void DeltaEncodedInt64ColumnReader::extract_string_value_into_buffer(
        uint64_t cur_message,
        std::string& buffer
) {
    buffer.append(std::to_string(get_value_at_idx(cur_message)));
}

std::variant<int64_t, double, std::string, uint8_t> FloatColumnReader::extract_value(
        uint64_t cur_message
) {
    return m_values[cur_message];
}

std::variant<int64_t, double, std::string, uint8_t> FormattedFloatColumnReader::extract_value(
        uint64_t cur_message
) {
    return m_values[cur_message];
}

void BooleanColumnReader::load(BufferViewReader& reader, uint64_t num_messages) {
    m_values = reader.read_unaligned_span<uint8_t>(num_messages);
}

void
FloatColumnReader::extract_string_value_into_buffer(uint64_t cur_message, std::string& buffer) {
    buffer.append(std::to_string(m_values[cur_message]));
}

void FormattedFloatColumnReader::extract_string_value_into_buffer(
        uint64_t cur_message,
        std::string& buffer
) {
    buffer.append(restore_format(cur_message));
}

std::string FormattedFloatColumnReader::restore_format(uint64_t cur_message) const {
    std::ostringstream oss;
    uint16_t const significant_digits = get_significant_digits(cur_message);
    oss << std::scientific << std::setprecision(significant_digits - 1);
    if (has_scientific_notation(cur_message)) {
        if (is_uppercase_exponent(cur_message)) {
            oss << std::uppercase;
        }
        oss << m_values[cur_message];
        auto formatted_double_str = oss.str();
        auto const exp_pos = formatted_double_str.find_first_of("Ee");
        assert(std::string::npos != exp_pos && exp_pos + 1 < formatted_double_str.length());
        unsigned char const maybe_sign
                = static_cast<unsigned char>(formatted_double_str[exp_pos + 1]);
        uint16_t const exp_digits = get_exponent_digits(cur_message);
        if (has_exponent_sign(cur_message, float_format_encoding::cEmptyExponentSign)) {
            if ('+' == maybe_sign || '-' == maybe_sign) {
                formatted_double_str.erase(exp_pos + 1, 1);
            }
            if (exp_digits < (formatted_double_str.length() - exp_pos - 1)) {
                formatted_double_str
                        = trim_leading_zeros(formatted_double_str, exp_pos + 1, exp_digits);
            } else {
                formatted_double_str.insert(
                        exp_pos + 1,
                        exp_digits - (formatted_double_str.length() - exp_pos - 1),
                        '0'
                );
            }
        } else {
            if (exp_digits < (formatted_double_str.length() - exp_pos - 2)) {
                formatted_double_str
                        = trim_leading_zeros(formatted_double_str, exp_pos + 2, exp_digits);
            } else {
                formatted_double_str.insert(
                        exp_pos + 2,
                        exp_digits - (formatted_double_str.length() - exp_pos - 2),
                        '0'
                );
            }
            if (has_exponent_sign(cur_message, float_format_encoding::cPlusExponentSign)) {
                if (std::isdigit(maybe_sign)) {
                    formatted_double_str.insert(exp_pos + 1, "+");
                } else {
                    formatted_double_str[exp_pos + 1] = '+';
                }
            } else if (has_exponent_sign(cur_message, float_format_encoding::cMinusExponentSign)) {
                if (std::isdigit(maybe_sign)) {
                    formatted_double_str.insert(exp_pos + 1, "-");
                } else {
                    formatted_double_str[exp_pos + 1] = '-';
                }
            }
        }

        return formatted_double_str;
    }

    // Convert the scientific notation to the standard decimal
    oss << m_values[cur_message];
    return scientific_to_decimal(oss.str());
}

bool FormattedFloatColumnReader::has_exponent_sign(uint64_t cur_message, uint16_t sign) const {
    return sign << float_format_encoding::cExponentSignPos
           == (m_formats[cur_message] & 0b11 << float_format_encoding::cExponentSignPos);
}

bool FormattedFloatColumnReader::has_scientific_notation(uint64_t cur_message) const {
    return m_formats[cur_message] & 1 << float_format_encoding::cExponentNotationPos;
}

bool FormattedFloatColumnReader::is_uppercase_exponent(uint64_t cur_message) const {
    return m_formats[cur_message] & 1 << (float_format_encoding::cExponentNotationPos + 1);
}

uint16_t FormattedFloatColumnReader::get_exponent_digits(uint64_t cur_message) const {
    return (m_formats[cur_message] >> float_format_encoding::cNumExponentDigitsPos & 0x03) + 1;
}

uint16_t FormattedFloatColumnReader::get_significant_digits(uint64_t cur_message) const {
    return (m_formats[cur_message] >> float_format_encoding::cNumSignificantDigitsPos & 0x0F) + 1;
}

std::string FormattedFloatColumnReader::trim_leading_zeros(
        std::string_view scientific_notation,
        size_t start,
        size_t exp_digits
) {
    auto sci_str = std::string(scientific_notation);
    size_t actual_number_of_zeros_to_trim{0};
    for (size_t i{start}; i < sci_str.length() - exp_digits; ++i) {
        if ('0' == sci_str[i]) {
            actual_number_of_zeros_to_trim++;
        } else {
            break;
        }
    }
    sci_str.erase(start, actual_number_of_zeros_to_trim);
    return sci_str;
}

std::string FormattedFloatColumnReader::scientific_to_decimal(
        std::string_view scientific_notation
) {
    auto sci_str = std::string(scientific_notation);
    bool isNegative = false;
    if (false == std::isdigit(static_cast<unsigned char>(sci_str[0]))) {
        isNegative = true;
        sci_str.erase(0, 1);
    }
    size_t const exp_pos = sci_str.find_first_of("Ee");
    assert(std::string::npos != exp_pos && exp_pos + 1 < sci_str.length());

    // Split into mantissa and exponent parts
    std::string mantissa_str = sci_str.substr(0, exp_pos);
    int const exponent = std::stoi(sci_str.substr(exp_pos + 1));

    // Remove the decimal point from the mantissa
    size_t const dot_pos = mantissa_str.find('.');
    std::string digits;
    if (dot_pos != std::string::npos) {
        digits = mantissa_str.substr(0, dot_pos) + mantissa_str.substr(dot_pos + 1);
    } else {
        digits = mantissa_str;
    }

    // Adjust position of decimal point based on exponent
    int const decimal_pos = static_cast<int>(dot_pos) + exponent;

    std::string result{""};
    if (isNegative) {
        result = "-";
    }
    if (decimal_pos <= 0) {
        result += "0." + std::string(-decimal_pos, '0') + digits;
    } else if (decimal_pos < static_cast<int>(digits.size())) {
        result += digits.substr(0, decimal_pos) + "." + digits.substr(decimal_pos);
    } else {
        result += digits + std::string(decimal_pos - digits.size(), '0');
    }

    return result;
}

std::variant<int64_t, double, std::string, uint8_t> BooleanColumnReader::extract_value(
        uint64_t cur_message
) {
    return m_values[cur_message];
}

void ClpStringColumnReader::load(BufferViewReader& reader, uint64_t num_messages) {
    m_logtypes = reader.read_unaligned_span<uint64_t>(num_messages);
    size_t encoded_vars_length = reader.read_value<size_t>();
    m_encoded_vars = reader.read_unaligned_span<int64_t>(encoded_vars_length);
}

void
BooleanColumnReader::extract_string_value_into_buffer(uint64_t cur_message, std::string& buffer) {
    buffer.append(0 == m_values[cur_message] ? "false" : "true");
}

std::variant<int64_t, double, std::string, uint8_t> ClpStringColumnReader::extract_value(
        uint64_t cur_message
) {
    std::string message;
    extract_string_value_into_buffer(cur_message, message);
    return message;
}

void
ClpStringColumnReader::extract_string_value_into_buffer(uint64_t cur_message, std::string& buffer) {
    auto value = m_logtypes[cur_message];
    int64_t logtype_id = ClpStringColumnWriter::get_encoded_log_dict_id(value);
    auto& entry = m_log_dict->get_entry(logtype_id);

    if (false == entry.initialized()) {
        entry.decode_log_type();
    }

    int64_t encoded_vars_offset = ClpStringColumnWriter::get_encoded_offset(value);
    auto encoded_vars = m_encoded_vars.sub_span(encoded_vars_offset, entry.get_num_variables());

    clp::EncodedVariableInterpreter::decode_variables_into_message(
            entry,
            *m_var_dict,
            encoded_vars,
            buffer
    );
}

void ClpStringColumnReader::extract_escaped_string_value_into_buffer(
        uint64_t cur_message,
        std::string& buffer
) {
    if (false == m_is_array) {
        // TODO: escape while decoding instead of after.
        std::string tmp;
        extract_string_value_into_buffer(cur_message, tmp);
        StringUtils::escape_json_string(buffer, tmp);
    } else {
        extract_string_value_into_buffer(cur_message, buffer);
    }
}

int64_t ClpStringColumnReader::get_encoded_id(uint64_t cur_message) {
    auto value = m_logtypes[cur_message];
    return ClpStringColumnWriter::get_encoded_log_dict_id(value);
}

UnalignedMemSpan<int64_t> ClpStringColumnReader::get_encoded_vars(uint64_t cur_message) {
    auto value = m_logtypes[cur_message];
    auto logtype_id = ClpStringColumnWriter::get_encoded_log_dict_id(value);
    auto& entry = m_log_dict->get_entry(logtype_id);

    // It should be initialized before because we are searching on this field
    if (false == entry.initialized()) {
        entry.decode_log_type();
    }

    int64_t encoded_vars_offset = ClpStringColumnWriter::get_encoded_offset(value);

    return m_encoded_vars.sub_span(encoded_vars_offset, entry.get_num_variables());
}

void VariableStringColumnReader::load(BufferViewReader& reader, uint64_t num_messages) {
    m_variables = reader.read_unaligned_span<uint64_t>(num_messages);
}

std::variant<int64_t, double, std::string, uint8_t> VariableStringColumnReader::extract_value(
        uint64_t cur_message
) {
    return m_var_dict->get_value(m_variables[cur_message]);
}

void VariableStringColumnReader::extract_string_value_into_buffer(
        uint64_t cur_message,
        std::string& buffer
) {
    buffer.append(m_var_dict->get_value(m_variables[cur_message]));
}

void VariableStringColumnReader::extract_escaped_string_value_into_buffer(
        uint64_t cur_message,
        std::string& buffer
) {
    StringUtils::escape_json_string(buffer, m_var_dict->get_value(m_variables[cur_message]));
}

int64_t VariableStringColumnReader::get_variable_id(uint64_t cur_message) {
    return m_variables[cur_message];
}

void DateStringColumnReader::load(BufferViewReader& reader, uint64_t num_messages) {
    m_timestamps = reader.read_unaligned_span<int64_t>(num_messages);
    m_timestamp_encodings = reader.read_unaligned_span<int64_t>(num_messages);
}

std::variant<int64_t, double, std::string, uint8_t> DateStringColumnReader::extract_value(
        uint64_t cur_message
) {
    return m_timestamp_dict->get_string_encoding(
            m_timestamps[cur_message],
            m_timestamp_encodings[cur_message]
    );
}

void DateStringColumnReader::extract_string_value_into_buffer(
        uint64_t cur_message,
        std::string& buffer
) {
    buffer.append(m_timestamp_dict->get_string_encoding(
            m_timestamps[cur_message],
            m_timestamp_encodings[cur_message]
    ));
}

epochtime_t DateStringColumnReader::get_encoded_time(uint64_t cur_message) {
    return m_timestamps[cur_message];
}
}  // namespace clp_s

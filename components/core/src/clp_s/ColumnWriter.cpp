#include "ColumnWriter.hpp"

namespace clp_s {
size_t Int64ColumnWriter::add_value(ParsedMessage::variable_t& value) {
    m_values.push_back(std::get<int64_t>(value));
    return sizeof(int64_t);
}

void Int64ColumnWriter::store(ZstdCompressor& compressor) {
    size_t size = m_values.size() * sizeof(int64_t);
    compressor.write(reinterpret_cast<char const*>(m_values.data()), size);
}

size_t DeltaEncodedInt64ColumnWriter::add_value(ParsedMessage::variable_t& value) {
    if (0 == m_values.size()) {
        m_cur = std::get<int64_t>(value);
        m_values.push_back(m_cur);
    } else {
        auto next = std::get<int64_t>(value);
        m_values.push_back(next - m_cur);
        m_cur = next;
    }
    return sizeof(int64_t);
}

void DeltaEncodedInt64ColumnWriter::store(ZstdCompressor& compressor) {
    size_t size = m_values.size() * sizeof(int64_t);
    compressor.write(reinterpret_cast<char const*>(m_values.data()), size);
}

size_t FloatColumnWriter::add_value(ParsedMessage::variable_t& value) {
    m_values.push_back(std::get<double>(value));
    return sizeof(double);
}

void FloatColumnWriter::store(ZstdCompressor& compressor) {
    size_t size = m_values.size() * sizeof(double);
    compressor.write(reinterpret_cast<char const*>(m_values.data()), size);
}

size_t FormattedFloatColumnWriter::add_value(ParsedMessage::variable_t& value) {
    auto float_str = std::get<std::string>(value);

    // Trim the raw string
    float_str.erase(std::remove_if(float_str.begin(), float_str.end(), [](char c) {
       return !std::isdigit(static_cast<unsigned char>(c)) && '.' != c && '+' != c && '-' != c && 'E' != c && 'e' != c;
    }), float_str.end());

    // If the raw JSON token is an illegal double number then here will throw
    // exception
    m_values.push_back(std::stod(float_str));

    const auto dot_pos = float_str.find('.');
    uint16_t format{0};

    // Check whether it is the scientific; if so, if the exponent is E or e
    size_t exp_pos = float_str.find_first_of("Ee");
    if (std::string::npos != exp_pos) {
        format |= 1 << float_format_encoding::cScientificExponentNotePos;
        format |= ('E' == float_str[exp_pos]) << (float_format_encoding::cScientificExponentNotePos + 1);

        // Check whether there is a sign for the exponent
        if ('+' == float_str[exp_pos + 1]) {
            format |= 1 << float_format_encoding::cScientificExponentSignPos;
        } else if ('-' == float_str[exp_pos + 1]) {
            format |= 1 << (float_format_encoding::cScientificExponentSignPos + 1);
        }

        // Set the number of exponent digits
        int exp_digits = float_str.length() - exp_pos - 1;
        if (false == std::isdigit(float_str[exp_pos + 1])) {
            exp_digits--;
        }
        format |= (static_cast<uint16_t>(std::min(exp_digits - 1, 3)) & 0x03) << float_format_encoding::cScientificExponentDigitsPos;
    } else {
        exp_pos = float_str.length();
    }

    // According to the JSON grammar, there is no leading zeros for the integer
    // part of a number, so we can check whether the first or the second (if the
    // there is a sign) digit is 0 to know if the first non-zero number is in
    // integer part.
    size_t first_non_zero_frac_digit_pos = std::isdigit(float_str[0]) ? 0 : 1;
    if (std::isdigit(float_str[0]) && '0' != float_str[0]) {
        first_non_zero_frac_digit_pos = 0;
    } else if (std::isdigit(float_str[1]) && '0' != float_str[1]) {
        first_non_zero_frac_digit_pos = 1;
    } else if (std::string::npos != dot_pos) {
        for (size_t i = dot_pos + 1; i < float_str.length(); ++i) {
            if (std::isdigit(float_str[i]) && '0' != float_str[i]) {
                first_non_zero_frac_digit_pos = i;
                break;
            }
        }
    }

    int significant_digits = exp_pos - first_non_zero_frac_digit_pos;
    if (std::string::npos != dot_pos && first_non_zero_frac_digit_pos < dot_pos) {
        significant_digits--;
    }
    const uint16_t compressed_significant_digits = static_cast<uint16_t>(std::min(significant_digits, 16)) & 0x1F;

    format |= compressed_significant_digits << float_format_encoding::cSignificantDigitsPos;

    m_format.push_back(format);
    return sizeof(double) + sizeof(uint16_t);
}

void FormattedFloatColumnWriter::store(ZstdCompressor& compressor) {
    assert(m_format.size() == m_values.size());
    const auto values_size = m_values.size() * sizeof(double);
    const auto format_size = m_format.size() * sizeof(uint16_t);
    compressor.write(reinterpret_cast<char const*>(m_values.data()), values_size);
    compressor.write(reinterpret_cast<char const*>(m_format.data()), format_size);
}

size_t BooleanColumnWriter::add_value(ParsedMessage::variable_t& value) {
    m_values.push_back(std::get<bool>(value) ? 1 : 0);
    return sizeof(uint8_t);
}

void BooleanColumnWriter::store(ZstdCompressor& compressor) {
    size_t size = m_values.size() * sizeof(uint8_t);
    compressor.write(reinterpret_cast<char const*>(m_values.data()), size);
}

size_t ClpStringColumnWriter::add_value(ParsedMessage::variable_t& value) {
    std::string string_var = std::get<std::string>(value);
    uint64_t id;
    uint64_t offset = m_encoded_vars.size();
    VariableEncoder::encode_and_add_to_dictionary(
            string_var,
            m_logtype_entry,
            *m_var_dict,
            m_encoded_vars
    );
    m_log_dict->add_entry(m_logtype_entry, id);
    auto encoded_id = encode_log_dict_id(id, offset);
    m_logtypes.push_back(encoded_id);
    return sizeof(int64_t) + sizeof(int64_t) * (m_encoded_vars.size() - offset);
}

void ClpStringColumnWriter::store(ZstdCompressor& compressor) {
    size_t logtypes_size = m_logtypes.size() * sizeof(int64_t);
    compressor.write(reinterpret_cast<char const*>(m_logtypes.data()), logtypes_size);
    size_t encoded_vars_size = m_encoded_vars.size() * sizeof(int64_t);
    size_t num_encoded_vars = m_encoded_vars.size();
    compressor.write_numeric_value(num_encoded_vars);
    compressor.write(reinterpret_cast<char const*>(m_encoded_vars.data()), encoded_vars_size);
}

size_t VariableStringColumnWriter::add_value(ParsedMessage::variable_t& value) {
    std::string string_var = std::get<std::string>(value);
    uint64_t id;
    m_var_dict->add_entry(string_var, id);
    m_variables.push_back(id);
    return sizeof(int64_t);
}

void VariableStringColumnWriter::store(ZstdCompressor& compressor) {
    size_t size = m_variables.size() * sizeof(int64_t);
    compressor.write(reinterpret_cast<char const*>(m_variables.data()), size);
}

size_t DateStringColumnWriter::add_value(ParsedMessage::variable_t& value) {
    auto encoded_timestamp = std::get<std::pair<uint64_t, epochtime_t>>(value);
    m_timestamps.push_back(encoded_timestamp.second);
    m_timestamp_encodings.push_back(encoded_timestamp.first);
    return 2 * sizeof(int64_t);
    ;
}

void DateStringColumnWriter::store(ZstdCompressor& compressor) {
    size_t timestamps_size = m_timestamps.size() * sizeof(int64_t);
    compressor.write(reinterpret_cast<char const*>(m_timestamps.data()), timestamps_size);
    size_t encodings_size = m_timestamp_encodings.size() * sizeof(int64_t);
    compressor.write(reinterpret_cast<char const*>(m_timestamp_encodings.data()), encodings_size);
}
}  // namespace clp_s

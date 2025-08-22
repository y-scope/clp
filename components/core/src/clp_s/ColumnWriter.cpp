#include "ColumnWriter.hpp"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <variant>

#include "../clp/Defs.h"
#include "../clp/EncodedVariableInterpreter.hpp"
#include "ParsedMessage.hpp"
#include "ZstdCompressor.hpp"

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

    // Filter to allowed numeric/exponent characters (digits, '.', '+', '-', 'E', 'e')
    float_str.erase(
            std::remove_if(
                    float_str.begin(),
                    float_str.end(),
                    [](char c) {
                        return !std::isdigit(static_cast<unsigned char>(c)) && '.' != c && '+' != c
                               && '-' != c && 'E' != c && 'e' != c;
                    }
            ),
            float_str.end()
    );

    m_values.push_back(std::stod(float_str));

    auto const dot_pos = float_str.find('.');
    uint16_t format{0};

    // Check whether it is scientific; if so, whether the exponent is E or e
    size_t exp_pos = float_str.find_first_of("Ee");
    if (std::string::npos != exp_pos) {
        // Exponent must be followed by an integer (e.g., "1E" or "1e+" are illegal)
        assert(exp_pos + 1 < float_str.length()
               && (std::isdigit(static_cast<unsigned char>(float_str[exp_pos + 1]))
                   || (exp_pos + 2 < float_str.length()
                       && (('+' == float_str[exp_pos + 1]) || ('-' == float_str[exp_pos + 1]))
                       && std::isdigit(static_cast<unsigned char>(float_str[exp_pos + 2])))));
        format |= static_cast<uint16_t>(1u) << float_format_encoding::cExponentNotationPos;
        format |= static_cast<uint16_t>('E' == float_str[exp_pos] ? 1u : 0u)
                  << (float_format_encoding::cExponentNotationPos + 1);

        // Check whether there is a sign for the exponent
        if ('+' == float_str[exp_pos + 1]) {
            format |= static_cast<uint16_t>(1u) << float_format_encoding::cExponentSignPos;
        } else if ('-' == float_str[exp_pos + 1]) {
            format |= static_cast<uint16_t>(1u) << (float_format_encoding::cExponentSignPos + 1);
        }

        // Set the number of exponent digits
        int exp_digits = float_str.length() - exp_pos - 1;
        if (false == std::isdigit(static_cast<unsigned char>(float_str[exp_pos + 1]))) {
            exp_digits--;
        }
        format |= (static_cast<uint16_t>(std::min(exp_digits - 1, 3)) & static_cast<uint16_t>(0x03))
                  << float_format_encoding::cNumExponentDigitsPos;
    } else {
        exp_pos = float_str.length();
    }

    // Find first non-zero digit position
    size_t first_non_zero_frac_digit_pos = 0;
    if (false == std::isdigit(static_cast<unsigned char>(float_str[0]))) {
        first_non_zero_frac_digit_pos = 1;  // Skip sign
    }

    if ('0' == float_str[first_non_zero_frac_digit_pos]) {
        // JSON doesn't allow leading zeros in integer part
        assert(first_non_zero_frac_digit_pos + 1 >= float_str.length()
               || false
                          == std::isdigit(
                                  static_cast<unsigned char>(
                                          float_str[first_non_zero_frac_digit_pos + 1]
                                  )
                          ));

        // For "0.xxx", find first non-zero in fractional part
        if (std::string::npos != dot_pos) {
            for (size_t i = dot_pos + 1; i < exp_pos; ++i) {
                if ('0' != float_str[i]) {
                    first_non_zero_frac_digit_pos = i;
                    break;
                }
            }
        }
    }

    int significant_digits = exp_pos - first_non_zero_frac_digit_pos;
    assert(first_non_zero_frac_digit_pos < exp_pos);
    if (std::string::npos != dot_pos && first_non_zero_frac_digit_pos < dot_pos) {
        significant_digits--;
    }

    // Number of significant digits must be greater than zero (e.g., E0 or . is illegal)
    assert(significant_digits > 0);
    uint16_t const compressed_significant_digits
            = static_cast<uint16_t>(std::min(significant_digits - 1, 15)) & 0x0F;

    format |= static_cast<uint16_t>(compressed_significant_digits)
              << float_format_encoding::cNumSignificantDigitsPos;

    m_formats.push_back(format);
    return sizeof(double) + sizeof(uint16_t);
}

void FormattedFloatColumnWriter::store(ZstdCompressor& compressor) {
    assert(m_formats.size() == m_values.size());
    auto const values_size = m_values.size() * sizeof(double);
    auto const format_size = m_formats.size() * sizeof(uint16_t);
    compressor.write(reinterpret_cast<char const*>(m_values.data()), values_size);
    compressor.write(reinterpret_cast<char const*>(m_formats.data()), format_size);
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
    uint64_t offset{m_encoded_vars.size()};
    std::vector<clp::variable_dictionary_id_t> temp_var_dict_ids;
    clp::EncodedVariableInterpreter::encode_and_add_to_dictionary(
            std::get<std::string>(value),
            m_logtype_entry,
            *m_var_dict,
            m_encoded_vars,
            temp_var_dict_ids
    );
    clp::logtype_dictionary_id_t id{};
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
    clp::variable_dictionary_id_t id{};
    m_var_dict->add_entry(std::get<std::string>(value), id);
    m_var_dict_ids.push_back(id);
    return sizeof(clp::variable_dictionary_id_t);
}

void VariableStringColumnWriter::store(ZstdCompressor& compressor) {
    auto size{m_var_dict_ids.size() * sizeof(clp::variable_dictionary_id_t)};
    compressor.write(reinterpret_cast<char const*>(m_var_dict_ids.data()), size);
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

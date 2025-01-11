// Code from CLP

#include "DictionaryEntry.hpp"

#include "Utils.hpp"

using std::string;

namespace clp_s {
size_t LogTypeDictionaryEntry::get_var_info(size_t var_ix, VarDelim& var_delim) const {
    if (var_ix >= m_var_positions.size()) {
        return SIZE_MAX;
    }

    auto var_position = m_var_positions[var_ix];
    var_delim = (VarDelim)m_value[var_position];

    return m_var_positions[var_ix];
}

LogTypeDictionaryEntry::VarDelim LogTypeDictionaryEntry::get_var_delim(size_t var_ix) const {
    if (var_ix >= m_var_positions.size()) {
        return VarDelim::Length;
    }

    auto var_position = m_var_positions[var_ix];
    return (VarDelim)m_value[var_position];
}

size_t LogTypeDictionaryEntry::get_var_length_in_logtype(size_t var_ix) const {
    auto var_delim = get_var_delim(var_ix);
    switch (var_delim) {
        case VarDelim::NonDouble:
            return 1;
        case VarDelim::Double:
            return 2;
        case VarDelim::Length:
        default:
            throw OperationFailed(ErrorCodeBadParam, __FILENAME__, __LINE__);
    }
}

size_t LogTypeDictionaryEntry::get_data_size() const {
    // NOTE: sizeof(vector[0]) is executed at compile time so there's no risk of an exception at
    // runtime
    return sizeof(m_id) + m_value.length() + m_var_positions.size() * sizeof(m_var_positions[0]);
}

void LogTypeDictionaryEntry::add_constant(
        string const& value_containing_constant,
        size_t begin_pos,
        size_t length
) {
    m_value.append(value_containing_constant, begin_pos, length);
}

void LogTypeDictionaryEntry::add_non_double_var() {
    m_var_positions.push_back(m_value.length());
    add_non_double_var(m_value);
}

void LogTypeDictionaryEntry::add_double_var() {
    m_var_positions.push_back(m_value.length());
    add_double_var(m_value);
}

bool LogTypeDictionaryEntry::parse_next_var(
        string const& msg,
        size_t& var_begin_pos,
        size_t& var_end_pos,
        string& var
) {
    auto last_var_end_pos = var_end_pos;
    if (StringUtils::get_bounds_of_next_var(msg, var_begin_pos, var_end_pos)) {
        // Append to log type: from end of last variable to start of current variable
        add_constant(msg, last_var_end_pos, var_begin_pos - last_var_end_pos);

        var.assign(msg, var_begin_pos, var_end_pos - var_begin_pos);
        return true;
    }
    if (last_var_end_pos < msg.length()) {
        // Append to log type: from end of last variable to end
        add_constant(msg, last_var_end_pos, msg.length() - last_var_end_pos);
    }

    return false;
}

void LogTypeDictionaryEntry::clear() {
    m_value.clear();
    m_var_positions.clear();
}

void LogTypeDictionaryEntry::write_to_file(ZstdCompressor& compressor) const {
    string escaped_value;
    get_value_with_unfounded_variables_escaped(escaped_value);
    compressor.write_numeric_value(escaped_value.length());
    compressor.write_string(escaped_value);
}

ErrorCode
LogTypeDictionaryEntry::try_read_from_file(ZstdDecompressor& decompressor, uint64_t id, bool lazy) {
    clear();

    m_id = id;
    ErrorCode error_code;
    uint64_t escaped_value_length;
    error_code = decompressor.try_read_numeric_value(escaped_value_length);
    if (ErrorCodeSuccess != error_code) {
        return error_code;
    }

    string escaped_value;
    error_code = decompressor.try_read_string(escaped_value_length, escaped_value);
    if (ErrorCodeSuccess != error_code) {
        return error_code;
    }

    if (lazy) {
        m_value = std::move(escaped_value);
    } else {
        decode_log_type(escaped_value);
    }

    return error_code;
}

void
LogTypeDictionaryEntry::read_from_file(ZstdDecompressor& decompressor, uint64_t id, bool lazy) {
    auto error_code = try_read_from_file(decompressor, id, lazy);
    if (ErrorCodeSuccess != error_code) {
        throw OperationFailed(error_code, __FILENAME__, __LINE__);
    }
}

void LogTypeDictionaryEntry::decode_log_type(string& escaped_value) {
    bool is_escaped = false;
    string constant;
    for (char c : escaped_value) {
        if (is_escaped) {
            constant += c;
            is_escaped = false;
        } else if (cEscapeChar == c) {
            is_escaped = true;
        } else {
            if ((char)LogTypeDictionaryEntry::VarDelim::NonDouble == c) {
                add_constant(constant, 0, constant.length());
                constant.clear();

                add_non_double_var();
            } else if ((char)LogTypeDictionaryEntry::VarDelim::Double == c) {
                add_constant(constant, 0, constant.length());
                constant.clear();

                add_double_var();
            } else {
                constant += c;
            }
        }
    }

    if (false == constant.empty()) {
        add_constant(constant, 0, constant.length());
    }

    m_init = true;
}

void LogTypeDictionaryEntry::decode_log_type() {
    string escaped_value = std::move(m_value);
    m_value.clear();
    decode_log_type(escaped_value);
}

void LogTypeDictionaryEntry::get_value_with_unfounded_variables_escaped(
        string& escaped_logtype_value
) const {
    size_t begin_ix = 0;
    // Reset escaped value and reserve enough space to at least contain the whole value
    escaped_logtype_value.clear();
    escaped_logtype_value.reserve(m_value.length());
    for (auto var_position : m_var_positions) {
        size_t end_ix = var_position;

        escape_variable_delimiters(m_value, begin_ix, end_ix, escaped_logtype_value);

        // Add variable delimiter
        escaped_logtype_value += m_value[end_ix];

        // Move begin to start of next portion of logtype between variables
        begin_ix = end_ix + 1;
    }
    // Escape any variable delimiters in remainder of value
    escape_variable_delimiters(m_value, begin_ix, m_value.length(), escaped_logtype_value);
}

void LogTypeDictionaryEntry::escape_variable_delimiters(
        string const& value,
        size_t begin_ix,
        size_t end_ix,
        string& escaped_value
) {
    for (size_t i = begin_ix; i < end_ix; ++i) {
        auto c = value[i];

        // Add escape character if necessary
        if ((char)LogTypeDictionaryEntry::VarDelim::NonDouble == c
            || (char)LogTypeDictionaryEntry::VarDelim::Double == c || cEscapeChar == c)
        {
            escaped_value += cEscapeChar;
        }

        // Add character
        escaped_value += value[i];
    }
}

size_t VariableDictionaryEntry::get_data_size() const {
    return sizeof(m_id) + m_value.length();
}

void VariableDictionaryEntry::write_to_file(ZstdCompressor& compressor) const {
    compressor.write_numeric_value<uint64_t>(m_value.length());
    compressor.write_string(m_value);
}

ErrorCode VariableDictionaryEntry::try_read_from_file(ZstdDecompressor& decompressor, uint64_t id) {
    m_id = id;

    ErrorCode error_code;
    uint64_t value_length;
    error_code = decompressor.try_read_numeric_value(value_length);
    if (ErrorCodeSuccess != error_code) {
        return error_code;
    }
    error_code = decompressor.try_read_string(value_length, m_value);
    if (ErrorCodeSuccess != error_code) {
        return error_code;
    }

    return error_code;
}

void
VariableDictionaryEntry::read_from_file(ZstdDecompressor& decompressor, uint64_t id, bool lazy) {
    auto error_code = try_read_from_file(decompressor, id);
    if (ErrorCodeSuccess != error_code) {
        throw OperationFailed(error_code, __FILENAME__, __LINE__);
    }
}
}  // namespace clp_s

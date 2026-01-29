// Code from CLP

#include "DictionaryEntry.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include <clp/Defs.h>
#include <clp/EncodedVariableInterpreter.hpp>
#include <clp/ir/parsing.hpp>
#include <clp/ir/types.hpp>
#include <clp/type_utils.hpp>

namespace clp_s {
using clp::EncodedVariableInterpreter;
using clp::enum_to_underlying_type;
using clp::ir::append_constant_to_logtype;
using clp::ir::get_bounds_of_next_var;
using clp::ir::VariablePlaceholder;
using std::string;
using std::string_view;

size_t LogTypeDictionaryEntry::get_data_size() const {
    // NOTE: sizeof(vector[0]) is executed at compile time so there's no risk of an exception at
    // runtime
    return sizeof(m_id) + m_value.length()
           + m_placeholder_positions.size() * sizeof(m_placeholder_positions[0]);
}

size_t LogTypeDictionaryEntry::get_placeholder_info(
        size_t placeholder_ix,
        VariablePlaceholder& placeholder
) const {
    if (placeholder_ix >= m_placeholder_positions.size()) {
        return SIZE_MAX;
    }

    auto var_position = m_placeholder_positions[placeholder_ix];
    placeholder = static_cast<VariablePlaceholder>(m_value[var_position]);

    return m_placeholder_positions[placeholder_ix];
}

void LogTypeDictionaryEntry::add_constant(
        string_view value_containing_constant,
        size_t begin_pos,
        size_t length
) {
    m_value.append(value_containing_constant, begin_pos, length);
}

void LogTypeDictionaryEntry::add_dictionary_var() {
    m_placeholder_positions.push_back(m_value.length());
    EncodedVariableInterpreter::add_dict_var(m_value);
}

void LogTypeDictionaryEntry::add_int_var() {
    m_placeholder_positions.push_back(m_value.length());
    EncodedVariableInterpreter::add_int_var(m_value);
}

void LogTypeDictionaryEntry::add_float_var() {
    m_placeholder_positions.push_back(m_value.length());
    EncodedVariableInterpreter::add_float_var(m_value);
}

void LogTypeDictionaryEntry::add_escape() {
    m_placeholder_positions.push_back(m_value.length());
    EncodedVariableInterpreter::add_escape(m_value);
    ++m_num_escaped_placeholders;
}

auto LogTypeDictionaryEntry::add_static_text(string_view static_text) -> void {
    append_constant_to_logtype(
            static_text,
            [&]([[maybe_unused]] string_view constant,
                [[maybe_unused]] size_t char_to_escape_pos,
                [[maybe_unused]] string& logtype) -> void { add_escape(); },
            m_value
    );
}

auto LogTypeDictionaryEntry::parse_next_var(
        string_view msg,
        size_t& var_begin_pos,
        size_t& var_end_pos,
        string_view& var
) -> bool {
    auto const last_var_end_pos{var_end_pos};
    auto escape_handler = [&]([[maybe_unused]] string_view constant,
                              [[maybe_unused]] size_t char_to_escape_pos,
                              [[maybe_unused]] string& logtype) -> void { add_escape(); };
    if (get_bounds_of_next_var(msg, var_begin_pos, var_end_pos)) {
        // Append to log type: from end of last variable to start of current variable
        auto constant = msg.substr(last_var_end_pos, var_begin_pos - last_var_end_pos);
        append_constant_to_logtype(constant, escape_handler, m_value);

        var = msg.substr(var_begin_pos, var_end_pos - var_begin_pos);
        return true;
    }
    if (last_var_end_pos < msg.length()) {
        // Append to log type: from end of last variable to end
        auto constant = msg.substr(last_var_end_pos, msg.length() - last_var_end_pos);
        append_constant_to_logtype(constant, escape_handler, m_value);
    }

    return false;
}

void LogTypeDictionaryEntry::clear() {
    m_value.clear();
    m_placeholder_positions.clear();
    m_num_escaped_placeholders = 0;
    m_init = false;
}

void LogTypeDictionaryEntry::write_to_file(ZstdCompressor& compressor) const {
    compressor.write_numeric_value<uint64_t>(m_value.length());
    compressor.write_string(m_value);
}

ErrorCode LogTypeDictionaryEntry::try_read_from_file(
        ZstdDecompressor& decompressor,
        clp::logtype_dictionary_id_t id,
        bool lazy
) {
    clear();

    m_id = id;
    ErrorCode error_code;
    uint64_t escaped_value_length{};
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

void LogTypeDictionaryEntry::read_from_file(
        ZstdDecompressor& decompressor,
        clp::logtype_dictionary_id_t id,
        bool lazy
) {
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
        } else if (enum_to_underlying_type(VariablePlaceholder::Escape) == c) {
            is_escaped = true;
            add_constant(constant, 0, constant.length());
            constant.clear();
            add_escape();
        } else {
            if (enum_to_underlying_type(VariablePlaceholder::Integer) == c) {
                add_constant(constant, 0, constant.length());
                constant.clear();
                add_int_var();
            } else if (enum_to_underlying_type(VariablePlaceholder::Float) == c) {
                add_constant(constant, 0, constant.length());
                constant.clear();
                add_float_var();
            } else if (enum_to_underlying_type(VariablePlaceholder::Dictionary) == c) {
                add_constant(constant, 0, constant.length());
                constant.clear();
                add_dictionary_var();
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

auto LogTypeDictionaryEntry::encode_constant(std::string_view constant) -> void {
    auto escape_handler = [&]([[maybe_unused]] string_view constant,
                              [[maybe_unused]] size_t char_to_escape_pos,
                              [[maybe_unused]] string& logtype) -> void { add_escape(); };
    append_constant_to_logtype(constant, escape_handler, m_value);
}

size_t VariableDictionaryEntry::get_data_size() const {
    return sizeof(m_id) + m_value.length();
}

void VariableDictionaryEntry::write_to_file(ZstdCompressor& compressor) const {
    compressor.write_numeric_value<uint64_t>(m_value.length());
    compressor.write_string(m_value);
}

ErrorCode VariableDictionaryEntry::try_read_from_file(
        ZstdDecompressor& decompressor,
        clp::variable_dictionary_id_t id
) {
    m_id = id;

    ErrorCode error_code;
    uint64_t value_length{};
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

void VariableDictionaryEntry::read_from_file(
        ZstdDecompressor& decompressor,
        clp::variable_dictionary_id_t id,
        bool lazy
) {
    auto error_code = try_read_from_file(decompressor, id);
    if (ErrorCodeSuccess != error_code) {
        throw OperationFailed(error_code, __FILENAME__, __LINE__);
    }
}
}  // namespace clp_s

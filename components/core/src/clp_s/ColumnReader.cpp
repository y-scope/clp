#include "ColumnReader.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>

#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <ystdlib/error_handling/Result.hpp>

#include <clp/EncodedVariableInterpreter.hpp>
#include <clp/ir/types.hpp>
#include <clp/LogTypeDictionaryEntryReq.hpp>
#include <clp/type_utils.hpp>
#include <clp_s/BufferViewReader.hpp>
#include <clp_s/ColumnWriter.hpp>
#include <clp_s/Defs.hpp>
#include <clp_s/FloatFormatEncoding.hpp>
#include <clp_s/SchemaTree.hpp>
#include <clp_s/Utils.hpp>

namespace clp_s {
auto Int64ColumnReader::load(BufferViewReader& reader, uint64_t num_messages) -> void {
    m_values = reader.read_unaligned_span<int64_t>(num_messages);
}

auto Int64ColumnReader::extract_value(uint64_t cur_message)
        -> std::variant<int64_t, double, std::string, uint8_t> {
    return m_values[cur_message];
}

auto DeltaEncodedInt64ColumnReader::load(BufferViewReader& reader, uint64_t num_messages) -> void {
    m_values = reader.read_unaligned_span<int64_t>(num_messages);
    if (num_messages > 0) {
        m_cur_idx = 0;
        m_cur_value = m_values[0];
    }
}

auto DeltaEncodedInt64ColumnReader::get_value_at_idx(size_t idx) -> int64_t {
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

auto DeltaEncodedInt64ColumnReader::extract_value(uint64_t cur_message)
        -> std::variant<int64_t, double, std::string, uint8_t> {
    return get_value_at_idx(cur_message);
}

auto FloatColumnReader::load(BufferViewReader& reader, uint64_t num_messages) -> void {
    m_values = reader.read_unaligned_span<double>(num_messages);
}

auto FormattedFloatColumnReader::load(BufferViewReader& reader, uint64_t num_messages) -> void {
    m_values = reader.read_unaligned_span<double>(num_messages);
    m_formats = reader.read_unaligned_span<float_format_t>(num_messages);
}

auto Int64ColumnReader::extract_string_value_into_buffer(uint64_t cur_message, std::string& buffer)
        -> void {
    buffer.append(std::to_string(m_values[cur_message]));
}

auto DeltaEncodedInt64ColumnReader::extract_string_value_into_buffer(
        uint64_t cur_message,
        std::string& buffer
) -> void {
    buffer.append(std::to_string(get_value_at_idx(cur_message)));
}

auto FloatColumnReader::extract_value(uint64_t cur_message)
        -> std::variant<int64_t, double, std::string, uint8_t> {
    return m_values[cur_message];
}

auto FormattedFloatColumnReader::extract_value(uint64_t cur_message)
        -> std::variant<int64_t, double, std::string, uint8_t> {
    return m_values[cur_message];
}

auto BooleanColumnReader::load(BufferViewReader& reader, uint64_t num_messages) -> void {
    m_values = reader.read_unaligned_span<uint8_t>(num_messages);
}

auto FloatColumnReader::extract_string_value_into_buffer(uint64_t cur_message, std::string& buffer)
        -> void {
    buffer.append(std::to_string(m_values[cur_message]));
}

auto FormattedFloatColumnReader::extract_string_value_into_buffer(
        uint64_t cur_message,
        std::string& buffer
) -> void {
    buffer.append(restore_encoded_float(m_values[cur_message], m_formats[cur_message]).value());
}

auto BooleanColumnReader::extract_value(uint64_t cur_message)
        -> std::variant<int64_t, double, std::string, uint8_t> {
    return m_values[cur_message];
}

auto DictionaryFloatColumnReader::load(BufferViewReader& reader, uint64_t num_messages) -> void {
    m_var_dict_ids = reader.read_unaligned_span<variable_dictionary_id_t>(num_messages);
}

auto DictionaryFloatColumnReader::extract_value(uint64_t cur_message)
        -> std::variant<int64_t, double, std::string, uint8_t> {
    return std::stod(m_var_dict->get_value(m_var_dict_ids[cur_message]));
}

auto DictionaryFloatColumnReader::extract_string_value_into_buffer(
        uint64_t cur_message,
        std::string& buffer
) -> void {
    buffer.append(m_var_dict->get_value(m_var_dict_ids[cur_message]));
}

auto ClpStringColumnReader::load(BufferViewReader& reader, uint64_t num_messages) -> void {
    m_logtypes = reader.read_unaligned_span<uint64_t>(num_messages);
    auto encoded_vars_length{reader.read_value<size_t>()};
    m_encoded_vars = reader.read_unaligned_span<int64_t>(encoded_vars_length);
}

auto
BooleanColumnReader::extract_string_value_into_buffer(uint64_t cur_message, std::string& buffer)
        -> void {
    buffer.append(0 == m_values[cur_message] ? "false" : "true");
}

auto ClpStringColumnReader::extract_value(uint64_t cur_message)
        -> std::variant<int64_t, double, std::string, uint8_t> {
    std::string message;
    extract_string_value_into_buffer(cur_message, message);
    return message;
}

auto
ClpStringColumnReader::extract_string_value_into_buffer(uint64_t cur_message, std::string& buffer)
        -> void {
    auto const value{m_logtypes[cur_message]};
    auto const logtype_id{ClpStringColumnWriter::get_encoded_log_dict_id(value)};
    auto& entry{m_log_dict->get_entry(logtype_id)};

    if (false == entry.initialized()) {
        entry.decode_log_type();
    }

    auto const encoded_vars_offset{ClpStringColumnWriter::get_encoded_offset(value)};
    auto encoded_vars{m_encoded_vars.sub_span(encoded_vars_offset, entry.get_num_variables())};

    clp::EncodedVariableInterpreter::decode_variables_into_message(
            entry,
            *m_var_dict,
            encoded_vars,
            buffer
    );
}

auto ClpStringColumnReader::extract_escaped_string_value_into_buffer(
        uint64_t cur_message,
        std::string& buffer
) -> void {
    if (false == m_is_array) {
        // TODO: escape while decoding instead of after.
        std::string tmp;
        extract_string_value_into_buffer(cur_message, tmp);
        StringUtils::escape_json_string(buffer, tmp);
    } else {
        extract_string_value_into_buffer(cur_message, buffer);
    }
}

auto ClpStringColumnReader::get_encoded_id(uint64_t cur_message) -> int64_t {
    auto value = m_logtypes[cur_message];
    return ClpStringColumnWriter::get_encoded_log_dict_id(value);
}

auto ClpStringColumnReader::get_encoded_vars(uint64_t cur_message) -> UnalignedMemSpan<int64_t> {
    auto value = m_logtypes[cur_message];
    auto logtype_id = ClpStringColumnWriter::get_encoded_log_dict_id(value);
    auto& entry = m_log_dict->get_entry(logtype_id);

    // It should be initialized before because we are searching on this field
    if (false == entry.initialized()) {
        entry.decode_log_type();
    }

    auto encoded_vars_offset{ClpStringColumnWriter::get_encoded_offset(value)};

    return m_encoded_vars.sub_span(encoded_vars_offset, entry.get_num_variables());
}

void VariableStringColumnReader::load(BufferViewReader& reader, uint64_t num_messages) {
    m_variables = reader.read_unaligned_span<uint64_t>(num_messages);
}

auto VariableStringColumnReader::extract_value(uint64_t cur_message)
        -> std::variant<int64_t, double, std::string, uint8_t> {
    return m_var_dict->get_value(m_variables[cur_message]);
}

void VariableStringColumnReader::extract_string_value_into_buffer(
        uint64_t cur_message,
        std::string& buffer
) {
    buffer.append(m_var_dict->get_value(m_variables[cur_message]));
}

auto VariableStringColumnReader::extract_escaped_string_value_into_buffer(
        uint64_t cur_message,
        std::string& buffer
) -> void {
    StringUtils::escape_json_string(buffer, m_var_dict->get_value(m_variables[cur_message]));
}

auto VariableStringColumnReader::get_variable_id(uint64_t cur_message) -> uint64_t {
    return m_variables[cur_message];
}

auto DateStringColumnReader::load(BufferViewReader& reader, uint64_t num_messages) -> void {
    m_timestamps = reader.read_unaligned_span<int64_t>(num_messages);
    m_timestamp_encodings = reader.read_unaligned_span<int64_t>(num_messages);
}

auto DateStringColumnReader::extract_value(uint64_t cur_message)
        -> std::variant<int64_t, double, std::string, uint8_t> {
    return m_timestamp_dict->get_string_encoding(
            m_timestamps[cur_message],
            m_timestamp_encodings[cur_message]
    );
}

auto
DateStringColumnReader::extract_string_value_into_buffer(uint64_t cur_message, std::string& buffer)
        -> void {
    buffer.append(m_timestamp_dict->get_string_encoding(
            m_timestamps[cur_message],
            m_timestamp_encodings[cur_message]
    ));
}

auto DateStringColumnReader::get_encoded_time(uint64_t cur_message) -> epochtime_t {
    return m_timestamps[cur_message];
}

auto TimestampColumnReader::load(BufferViewReader& reader, uint64_t num_messages) -> void {
    m_timestamps.load(reader, num_messages);
    m_timestamp_encodings = reader.read_unaligned_span<uint64_t>(num_messages);
}

auto TimestampColumnReader::extract_value(uint64_t cur_message)
        -> std::variant<int64_t, double, std::string, uint8_t> {
    std::string ret;
    m_timestamp_dict->append_timestamp_to_buffer(
            m_timestamps.get_value_at_idx(cur_message),
            m_timestamp_encodings[cur_message],
            ret
    );
    return ret;
}

auto
TimestampColumnReader::extract_string_value_into_buffer(uint64_t cur_message, std::string& buffer)
        -> void {
    m_timestamp_dict->append_timestamp_to_buffer(
            m_timestamps.get_value_at_idx(cur_message),
            m_timestamp_encodings[cur_message],
            buffer
    );
}

auto TimestampColumnReader::get_encoded_time(uint64_t cur_message) -> epochtime_t {
    return m_timestamps.get_value_at_idx(cur_message);
}
}  // namespace clp_s

#include "ColumnReader.hpp"

#include "BufferViewReader.hpp"
#include "ColumnWriter.hpp"
#include "Utils.hpp"
#include "VariableDecoder.hpp"

namespace clp_s {
void Int64ColumnReader::load(BufferViewReader& reader, uint64_t num_messages) {
    m_values = reader.read_unaligned_span<int64_t>(num_messages);
}

std::variant<int64_t, double, std::string, uint8_t> Int64ColumnReader::extract_value(
        uint64_t cur_message
) {
    return m_values[cur_message];
}

void FloatColumnReader::load(BufferViewReader& reader, uint64_t num_messages) {
    m_values = reader.read_unaligned_span<double>(num_messages);
}

void
Int64ColumnReader::extract_string_value_into_buffer(uint64_t cur_message, std::string& buffer) {
    buffer.append(std::to_string(m_values[cur_message]));
}

std::variant<int64_t, double, std::string, uint8_t> FloatColumnReader::extract_value(
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
    auto encoded_vars = m_encoded_vars.sub_span(encoded_vars_offset, entry.get_num_vars());

    VariableDecoder::decode_variables_into_message(entry, *m_var_dict, encoded_vars, buffer);
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

    return m_encoded_vars.sub_span(encoded_vars_offset, entry.get_num_vars());
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

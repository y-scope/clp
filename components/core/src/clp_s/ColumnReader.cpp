#include "ColumnReader.hpp"

#include "ColumnWriter.hpp"
#include "Utils.hpp"
#include "VariableDecoder.hpp"

namespace clp_s {
char* Int64ColumnReader::load(char* buffer, uint64_t num_messages) {
    m_values = {buffer, num_messages};
    return buffer + m_values.size_in_bytes();
}

std::variant<int64_t, double, std::string, uint8_t> Int64ColumnReader::extract_value(
        uint64_t cur_message
) {
    return m_values[cur_message];
}

char* FloatColumnReader::load(char* buffer, uint64_t num_messages) {
    m_values = {buffer, num_messages};
    return buffer + m_values.size_in_bytes();
}

std::variant<int64_t, double, std::string, uint8_t> FloatColumnReader::extract_value(
        uint64_t cur_message
) {
    return m_values[cur_message];
}

char* BooleanColumnReader::load(char* buffer, uint64_t num_messages) {
    m_values = {buffer, num_messages};
    return buffer + m_values.size_in_bytes();
}

std::variant<int64_t, double, std::string, uint8_t> BooleanColumnReader::extract_value(
        uint64_t cur_message
) {
    return m_values[cur_message];
}

char* ClpStringColumnReader::load(char* buffer, uint64_t num_messages) {
    size_t encoded_vars_length;

    m_logtypes = {buffer, num_messages};
    buffer += m_logtypes.size_in_bytes();
    memcpy(&encoded_vars_length, buffer, sizeof(encoded_vars_length));
    buffer += sizeof(encoded_vars_length);
    m_encoded_vars = {buffer, encoded_vars_length};
    return buffer + m_encoded_vars.size_in_bytes();
}

std::variant<int64_t, double, std::string, uint8_t> ClpStringColumnReader::extract_value(
        uint64_t cur_message
) {
    std::string message;

    auto value = m_logtypes[cur_message];
    int64_t logtype_id = ClpStringColumnWriter::get_encoded_log_dict_id(value);
    auto& entry = m_log_dict->get_entry(logtype_id);

    if (false == entry.initialized()) {
        entry.decode_log_type();
    }

    int64_t encoded_vars_offset = ClpStringColumnWriter::get_encoded_offset(value);
    auto encoded_vars = m_encoded_vars.sub_span(encoded_vars_offset, entry.get_num_vars());

    VariableDecoder::decode_variables_into_message(entry, *m_var_dict, encoded_vars, message);

    return message;
}

int64_t ClpStringColumnReader::get_encoded_id(uint64_t cur_message) {
    auto value = m_logtypes[cur_message];
    return ClpStringColumnWriter::get_encoded_log_dict_id(value);
}

UnalignedSpan<int64_t> ClpStringColumnReader::get_encoded_vars(uint64_t cur_message) {
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

char* VariableStringColumnReader::load(char* buffer, uint64_t num_messages) {
    m_variables = {buffer, num_messages};
    return buffer + m_variables.size_in_bytes();
}

std::variant<int64_t, double, std::string, uint8_t> VariableStringColumnReader::extract_value(
        uint64_t cur_message
) {
    return m_var_dict->get_value(m_variables[cur_message]);
}

int64_t VariableStringColumnReader::get_variable_id(uint64_t cur_message) {
    return m_variables[cur_message];
}

char* DateStringColumnReader::load(char* buffer, uint64_t num_messages) {
    m_timestamps = {buffer, num_messages};
    buffer += m_timestamps.size_in_bytes();
    m_timestamp_encodings = {buffer, num_messages};
    return buffer + m_timestamp_encodings.size_in_bytes();
}

std::variant<int64_t, double, std::string, uint8_t> DateStringColumnReader::extract_value(
        uint64_t cur_message
) {
    return m_timestamp_dict->get_string_encoding(
            m_timestamps[cur_message],
            m_timestamp_encodings[cur_message]
    );
}

epochtime_t DateStringColumnReader::get_encoded_time(uint64_t cur_message) {
    return m_timestamps[cur_message];
}
}  // namespace clp_s

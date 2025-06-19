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

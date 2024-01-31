#include "ColumnWriter.hpp"

namespace clp_s {
void Int64ColumnWriter::add_value(
        std::variant<int64_t, double, std::string, bool>& value,
        size_t& size
) {
    size = sizeof(int64_t);
    m_values.push_back(std::get<int64_t>(value));
}

void Int64ColumnWriter::store(ZstdCompressor& compressor) {
    compressor.write(
            reinterpret_cast<char const*>(m_values.data()),
            m_values.size() * sizeof(int64_t)
    );
}

void FloatColumnWriter::add_value(
        std::variant<int64_t, double, std::string, bool>& value,
        size_t& size
) {
    size = sizeof(double);
    m_values.push_back(std::get<double>(value));
}

void FloatColumnWriter::store(ZstdCompressor& compressor) {
    compressor.write(
            reinterpret_cast<char const*>(m_values.data()),
            m_values.size() * sizeof(double)
    );
}

void BooleanColumnWriter::add_value(
        std::variant<int64_t, double, std::string, bool>& value,
        size_t& size
) {
    size = sizeof(uint8_t);
    m_values.push_back(std::get<bool>(value) ? 1 : 0);
}

void BooleanColumnWriter::store(ZstdCompressor& compressor) {
    compressor.write(
            reinterpret_cast<char const*>(m_values.data()),
            m_values.size() * sizeof(uint8_t)
    );
}

void ClpStringColumnWriter::add_value(
        std::variant<int64_t, double, std::string, bool>& value,
        size_t& size
) {
    size = sizeof(int64_t);
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
    size += sizeof(int64_t) * (m_encoded_vars.size() - offset);
}

void ClpStringColumnWriter::store(ZstdCompressor& compressor) {
    compressor.write(
            reinterpret_cast<char const*>(m_logtypes.data()),
            m_logtypes.size() * sizeof(int64_t)
    );
    compressor.write_numeric_value(m_encoded_vars.size());
    compressor.write(
            reinterpret_cast<char const*>(m_encoded_vars.data()),
            m_encoded_vars.size() * sizeof(int64_t)
    );
}

void VariableStringColumnWriter::add_value(
        std::variant<int64_t, double, std::string, bool>& value,
        size_t& size
) {
    size = sizeof(int64_t);
    std::string string_var = std::get<std::string>(value);
    uint64_t id;
    m_var_dict->add_entry(string_var, id);
    m_variables.push_back(id);
}

void VariableStringColumnWriter::store(ZstdCompressor& compressor) {
    compressor.write(
            reinterpret_cast<char const*>(m_variables.data()),
            m_variables.size() * sizeof(int64_t)
    );
}

void DateStringColumnWriter::add_value(
        std::variant<int64_t, double, std::string, bool>& value,
        size_t& size
) {
    size = 2 * sizeof(int64_t);
    std::string string_timestamp = std::get<std::string>(value);

    uint64_t encoding_id;
    epochtime_t timestamp
            = m_timestamp_dict->ingest_entry(m_name, m_id, string_timestamp, encoding_id);

    m_timestamps.push_back(timestamp);
    m_timestamp_encodings.push_back(encoding_id);
}

void DateStringColumnWriter::store(ZstdCompressor& compressor) {
    compressor.write(
            reinterpret_cast<char const*>(m_timestamps.data()),
            m_timestamps.size() * sizeof(int64_t)
    );
    compressor.write(
            reinterpret_cast<char const*>(m_timestamp_encodings.data()),
            m_timestamp_encodings.size() * sizeof(int64_t)
    );
}

void FloatDateStringColumnWriter::add_value(
        std::variant<int64_t, double, std::string, bool>& value,
        size_t& size
) {
    size = sizeof(double);
    double timestamp = std::get<double>(value);

    m_timestamp_dict->ingest_entry(m_name, m_id, timestamp);

    m_timestamps.push_back(timestamp);
}

void FloatDateStringColumnWriter::store(ZstdCompressor& compressor) {
    compressor.write(
            reinterpret_cast<char const*>(m_timestamps.data()),
            m_timestamps.size() * sizeof(double)
    );
}
}  // namespace clp_s

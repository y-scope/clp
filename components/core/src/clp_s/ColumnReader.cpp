#include "ColumnReader.hpp"

#include "ColumnWriter.hpp"
#include "Utils.hpp"
#include "VariableDecoder.hpp"

namespace clp_s {
void Int64ColumnReader::load(ZstdDecompressor& decompressor, uint64_t num_messages) {
    m_values = std::make_unique<int64_t[]>(num_messages);

    decompressor.try_read_exact_length(
            reinterpret_cast<char*>(m_values.get()),
            num_messages * sizeof(int64_t)
    );
}

std::variant<int64_t, double, std::string, uint8_t> Int64ColumnReader::extract_value(
        uint64_t cur_message
) {
    return m_values[cur_message];
}

void FloatColumnReader::load(ZstdDecompressor& decompressor, uint64_t num_messages) {
    m_values = std::make_unique<double[]>(num_messages);

    decompressor.try_read_exact_length(
            reinterpret_cast<char*>(m_values.get()),
            num_messages * sizeof(double)
    );
}

std::variant<int64_t, double, std::string, uint8_t> FloatColumnReader::extract_value(
        uint64_t cur_message
) {
    return m_values[cur_message];
}

void BooleanColumnReader::load(ZstdDecompressor& decompressor, uint64_t num_messages) {
    m_values = std::make_unique<uint8_t[]>(num_messages);

    decompressor.try_read_exact_length(
            reinterpret_cast<char*>(m_values.get()),
            num_messages * sizeof(uint8_t)
    );
}

std::variant<int64_t, double, std::string, uint8_t> BooleanColumnReader::extract_value(
        uint64_t cur_message
) {
    return m_values[cur_message];
}

void ClpStringColumnReader::load(ZstdDecompressor& decompressor, uint64_t num_messages) {
    size_t encoded_vars_length;

    m_logtypes = std::make_unique<int64_t[]>(num_messages);
    decompressor.try_read_exact_length(
            reinterpret_cast<char*>(m_logtypes.get()),
            num_messages * sizeof(int64_t)
    );

    auto error_code = decompressor.try_read_numeric_value(encoded_vars_length);
    if (ErrorCodeSuccess != error_code) {
        throw OperationFailed(error_code, __FILENAME__, __LINE__);
    }

    m_encoded_vars = std::make_unique<int64_t[]>(encoded_vars_length);
    decompressor.try_read_exact_length(
            reinterpret_cast<char*>(m_encoded_vars.get()),
            encoded_vars_length * sizeof(int64_t)
    );
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
    Span<int64_t> encoded_vars(&m_encoded_vars[encoded_vars_offset], entry.get_num_vars());

    VariableDecoder::decode_variables_into_message(entry, *m_var_dict, encoded_vars, message);

    return message;
}

int64_t ClpStringColumnReader::get_encoded_id(uint64_t cur_message) {
    auto value = m_logtypes[cur_message];
    return ClpStringColumnWriter::get_encoded_log_dict_id(value);
}

Span<int64_t> ClpStringColumnReader::get_encoded_vars(uint64_t cur_message) {
    auto value = m_logtypes[cur_message];
    int64_t logtype_id = ClpStringColumnWriter::get_encoded_log_dict_id(value);
    auto& entry = m_log_dict->get_entry(logtype_id);

    // It should be initialized before because we are searching on this field
    if (false == entry.initialized()) {
        entry.decode_log_type();
    }

    int64_t encoded_vars_offset = ClpStringColumnWriter::get_encoded_offset(value);

    return {&m_encoded_vars[encoded_vars_offset], entry.get_num_vars()};
}

void VariableStringColumnReader::load(ZstdDecompressor& decompressor, uint64_t num_messages) {
    m_variables = std::make_unique<int64_t[]>(num_messages);
    decompressor.try_read_exact_length(
            reinterpret_cast<char*>(m_variables.get()),
            num_messages * sizeof(int64_t)
    );
}

std::variant<int64_t, double, std::string, uint8_t> VariableStringColumnReader::extract_value(
        uint64_t cur_message
) {
    return m_var_dict->get_value(m_variables[cur_message]);
}

int64_t VariableStringColumnReader::get_variable_id(uint64_t cur_message) {
    return m_variables[cur_message];
}

void DateStringColumnReader::load(ZstdDecompressor& decompressor, uint64_t num_messages) {
    m_timestamps = std::make_unique<int64_t[]>(num_messages);
    m_timestamp_encodings = std::make_unique<int64_t[]>(num_messages);

    decompressor.try_read_exact_length(
            reinterpret_cast<char*>(m_timestamps.get()),
            num_messages * sizeof(int64_t)
    );
    decompressor.try_read_exact_length(
            reinterpret_cast<char*>(m_timestamp_encodings.get()),
            num_messages * sizeof(int64_t)
    );
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

void FloatDateStringColumnReader::load(ZstdDecompressor& decompressor, uint64_t num_messages) {
    m_timestamps = std::make_unique<double[]>(num_messages);
    decompressor.try_read_exact_length(
            reinterpret_cast<char*>(m_timestamps.get()),
            num_messages * sizeof(double)
    );
}

std::variant<int64_t, double, std::string, uint8_t> FloatDateStringColumnReader::extract_value(
        uint64_t cur_message
) {
    return std::to_string(m_timestamps[cur_message]);
}

double FloatDateStringColumnReader::get_encoded_time(uint64_t cur_message) {
    return m_timestamps[cur_message];
}
}  // namespace clp_s

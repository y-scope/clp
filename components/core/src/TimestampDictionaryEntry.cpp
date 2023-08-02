#include "TimestampDictionaryEntry.hpp"

size_t TimestampDictionaryEntry::get_data_size () const {
    return sizeof(m_id) + m_value.length();
}

void TimestampDictionaryEntry::write_to_file (streaming_compression::Compressor& compressor) const {
    compressor.write_numeric_value(m_id);
    compressor.write_numeric_value<uint64_t>(m_value.length());
    compressor.write_string(m_value);
}

ErrorCode TimestampDictionaryEntry::try_read_from_file (streaming_compression::Decompressor& decompressor) {
    ErrorCode error_code;

    error_code = decompressor.try_read_numeric_value<timestamp_dictionary_id_t>(m_id);
    if (ErrorCode_Success != error_code) {
        return error_code;
    }

    uint64_t value_length;
    error_code = decompressor.try_read_numeric_value(value_length);
    if (ErrorCode_Success != error_code) {
        return error_code;
    }
    error_code = decompressor.try_read_string(value_length, m_value);
    if (ErrorCode_Success != error_code) {
        return error_code;
    }

    return error_code;
}

void TimestampDictionaryEntry::read_from_file (streaming_compression::Decompressor& decompressor) {
    auto error_code = try_read_from_file(decompressor);
    if (ErrorCode_Success != error_code) {
        throw OperationFailed(error_code, __FILENAME__, __LINE__);
    }
}

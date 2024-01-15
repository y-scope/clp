#include "VariableDictionaryEntry.hpp"

namespace clp {
size_t VariableDictionaryEntry::get_data_size() const {
    return sizeof(m_id) + m_value.length()
           + m_ids_of_segments_containing_entry.size() * sizeof(segment_id_t);
}

void VariableDictionaryEntry::write_to_file(streaming_compression::Compressor& compressor) const {
    compressor.write_numeric_value(m_id);
    compressor.write_numeric_value<uint64_t>(m_value.length());
    compressor.write_string(m_value);
}

ErrorCode VariableDictionaryEntry::try_read_from_file(
        streaming_compression::Decompressor& decompressor
) {
    ErrorCode error_code;

    error_code = decompressor.try_read_numeric_value<variable_dictionary_id_t>(m_id);
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

void VariableDictionaryEntry::read_from_file(streaming_compression::Decompressor& decompressor) {
    auto error_code = try_read_from_file(decompressor);
    if (ErrorCode_Success != error_code) {
        throw OperationFailed(error_code, __FILENAME__, __LINE__);
    }
}
}  // namespace clp

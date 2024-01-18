#include "GLTSegment.hpp"

#include "Message.hpp"

namespace glt::streaming_archive::reader {
ErrorCode GLTSegment::try_open(std::string const& segment_dir_path, segment_id_t segment_id) {
    std::string segment_path = segment_dir_path + std::to_string(segment_id);
    m_logtype_tables_manager.open(segment_path);

    return ErrorCode_Success;
}

void GLTSegment::close() {
    m_logtype_tables_manager.close();
}

epochtime_t GLTSegment::get_timestamp_at_offset(logtype_dictionary_id_t logtype_id, size_t offset) {
    if (!m_logtype_tables_manager.check_variable_column(logtype_id)) {
        m_logtype_tables_manager.load_variable_columns(logtype_id);
    }
    return m_logtype_tables_manager.get_timestamp_at_offset(logtype_id, offset);
}

void GLTSegment::get_variable_row_at_offset(
        logtype_dictionary_id_t logtype_id,
        size_t offset,
        Message& msg
) {
    if (!m_logtype_tables_manager.check_variable_column(logtype_id)) {
        m_logtype_tables_manager.load_variable_columns(logtype_id);
    }
    m_logtype_tables_manager.get_variable_row_at_offset(logtype_id, offset, msg);
}
}  // namespace glt::streaming_archive::reader

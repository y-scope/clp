#ifndef GLT_STREAMING_ARCHIVE_READER_GLTSEGMENT_HPP
#define GLT_STREAMING_ARCHIVE_READER_GLTSEGMENT_HPP

#include "MultiLogtypeTablesManager.hpp"
#include "Segment.hpp"

namespace glt::streaming_archive::reader {
class GLTSegment {
public:
    ErrorCode try_open(std::string const& segment_dir_path, segment_id_t segment_id);
    void close();

    void
    get_variable_row_at_offset(logtype_dictionary_id_t logtype_id, size_t offset, Message& msg);
    epochtime_t get_timestamp_at_offset(logtype_dictionary_id_t logtype_id, size_t offset);

private:
    MultiLogtypeTablesManager m_logtype_tables_manager;
};
}  // namespace glt::streaming_archive::reader

#endif  // GLT_STREAMING_ARCHIVE_READER_GLTSEGMENT_HPP

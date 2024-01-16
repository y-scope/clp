#ifndef STREAMING_ARCHIVE_READER_GLT_SEGMENT_HPP
#define STREAMING_ARCHIVE_READER_GLT_SEGMENT_HPP

#include "Segment.hpp"
#include "MultiLogtypeTablesManager.hpp"

namespace glt::streaming_archive::reader {
    class GLTSegment {
    public:
        ErrorCode try_open (const std::string& segment_dir_path, segment_id_t segment_id);
        void close ();

        void get_variable_row_at_offset (logtype_dictionary_id_t logtype_id, size_t offset, Message& msg);
        epochtime_t get_timestamp_at_offset (logtype_dictionary_id_t logtype_id, size_t offset);
    private:
        MultiLogtypeTablesManager m_logtype_tables_manager;
    };
}

#endif //STREAMING_ARCHIVE_READER_GLT_SEGMENT_HPP
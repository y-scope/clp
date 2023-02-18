#include "GLTMetadataDB.hpp"
// C++ standard libraries
#include <vector>

// Project headers
#include "../database_utils.hpp"
#include "Constants.hpp"
#include "writer/GLT/GLTFile.hpp"

namespace streaming_archive {
    void GLTMetadataDB::add_storage_specific_fields (std::vector<std::string>& field_names) {
        field_names[enum_to_underlying_type(GLTFilesTableFieldIndexes::SegmentLogtypesPosition)] = streaming_archive::cMetadataDB::GLTFile::SegmentLogtypesPosition;
        field_names[enum_to_underlying_type(GLTFilesTableFieldIndexes::SegmentOffsetPosition)] = streaming_archive::cMetadataDB::GLTFile::SegmentOffsetPosition;
    }

    GLTMetadataDB::GLTFileIterator::GLTFileIterator (MetadataDB* mdb_ptr, SQLiteDB& db, epochtime_t begin_timestamp, epochtime_t end_timestamp, const std::string& file_path,
                                                     bool in_specific_segment, segment_id_t segment_id) :
            FileIterator(mdb_ptr, db, begin_timestamp, end_timestamp, file_path, in_specific_segment, segment_id) {}

    size_t GLTMetadataDB::GLTFileIterator::get_segment_offset_pos () const {
        return m_statement.column_int64(enum_to_underlying_type(GLTFilesTableFieldIndexes::SegmentOffsetPosition));
    }

    size_t GLTMetadataDB::GLTFileIterator::get_segment_logtypes_pos () const {
        return m_statement.column_int64(enum_to_underlying_type(GLTFilesTableFieldIndexes::SegmentLogtypesPosition));
    }

    void GLTMetadataDB::create_storage_specific_index (
            std::back_insert_iterator<fmt::memory_buffer> statement_buffer_ix) {
        fmt::format_to(statement_buffer_ix, "CREATE INDEX IF NOT EXISTS files_segment_order ON {} ({},{})", streaming_archive::cMetadataDB::FilesTableName,
                       streaming_archive::cMetadataDB::File::SegmentId, streaming_archive::cMetadataDB::GLTFile::SegmentLogtypesPosition);
        SPDLOG_DEBUG("{:.{}}", statement_buffer.data(), statement_buffer.size());
    }

    void GLTMetadataDB::add_storage_specific_ordering (
            std::back_insert_iterator<fmt::memory_buffer> statement_buffer_ix) {
        fmt::format_to(statement_buffer_ix, " ORDER BY {} ASC, {} ASC", streaming_archive::cMetadataDB::File::SegmentId,
                       streaming_archive::cMetadataDB::GLTFile::SegmentLogtypesPosition);
    }

    void GLTMetadataDB::add_storage_specific_field_names_and_types (
            std::vector<std::pair<std::string, std::string>>& file_field_names_and_types) {

        file_field_names_and_types[enum_to_underlying_type(GLTFilesTableFieldIndexes::SegmentLogtypesPosition)].first =
                streaming_archive::cMetadataDB::GLTFile::SegmentLogtypesPosition;
        file_field_names_and_types[enum_to_underlying_type(GLTFilesTableFieldIndexes::SegmentLogtypesPosition)].second = "INTEGER";

        file_field_names_and_types[enum_to_underlying_type(GLTFilesTableFieldIndexes::SegmentOffsetPosition)].first =
                streaming_archive::cMetadataDB::GLTFile::SegmentOffsetPosition;
        file_field_names_and_types[enum_to_underlying_type(GLTFilesTableFieldIndexes::SegmentOffsetPosition)].second = "INTEGER";
    }

    void GLTMetadataDB::bind_storage_specific_fields (writer::File* file) {

        writer::GLTFile* glt_file_ptr = dynamic_cast<writer::GLTFile*>(file);

        m_upsert_file_statement->bind_int64(enum_to_underlying_type(GLTFilesTableFieldIndexes::SegmentLogtypesPosition) + 1,
                                            (int64_t)glt_file_ptr->get_segment_logtypes_pos());
        m_upsert_file_statement->bind_int64(enum_to_underlying_type(GLTFilesTableFieldIndexes::SegmentOffsetPosition) + 1,
                                            (int64_t)glt_file_ptr->get_segment_offset_pos());
    }
}
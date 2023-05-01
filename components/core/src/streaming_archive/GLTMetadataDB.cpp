#include "GLTMetadataDB.hpp"

// Project headers
#include "writer/GLT/GLTFile.hpp"

using std::back_insert_iterator;
using std::make_pair;
using std::string;
using std::vector;

namespace streaming_archive {
    void GLTMetadataDB::add_storage_specific_fields (vector<string>& field_names) {
        field_names[
                enum_to_underlying_type(GLTFilesTableFieldIndexes::SegmentLogtypesPosition)
        ] = streaming_archive::cMetadataDB::GLTFile::SegmentLogtypesPosition;
        field_names[
                enum_to_underlying_type(GLTFilesTableFieldIndexes::SegmentOffsetPosition)
        ] = streaming_archive::cMetadataDB::GLTFile::SegmentOffsetPosition;
    }

    GLTMetadataDB::GLTFileIterator::GLTFileIterator (MetadataDB* mdb_ptr, SQLiteDB& db,
                                                     epochtime_t begin_timestamp,
                                                     epochtime_t end_timestamp,
                                                     const string& file_path,
                                                     bool in_specific_segment,
                                                     segment_id_t segment_id) :
            FileIterator(mdb_ptr, db, begin_timestamp, end_timestamp, file_path,
                         in_specific_segment, segment_id) {}

    size_t GLTMetadataDB::GLTFileIterator::get_segment_offset_pos () const {
        return m_statement.column_int64(
                enum_to_underlying_type(GLTFilesTableFieldIndexes::SegmentOffsetPosition));
    }

    size_t GLTMetadataDB::GLTFileIterator::get_segment_logtypes_pos () const {
        return m_statement.column_int64(
                enum_to_underlying_type(GLTFilesTableFieldIndexes::SegmentLogtypesPosition));
    }

    void GLTMetadataDB::create_storage_specific_index (
            back_insert_iterator<fmt::memory_buffer> statement_buffer_ix) {
        fmt::format_to(statement_buffer_ix,
                       "CREATE INDEX IF NOT EXISTS files_segment_order ON {} ({},{})",
                       streaming_archive::cMetadataDB::FilesTableName,
                       streaming_archive::cMetadataDB::File::SegmentId,
                       streaming_archive::cMetadataDB::GLTFile::SegmentLogtypesPosition);
    }

    void GLTMetadataDB::add_storage_specific_ordering (
            back_insert_iterator<fmt::memory_buffer> statement_buffer_ix) {
        fmt::format_to(statement_buffer_ix, " ORDER BY {} ASC, {} ASC",
                       streaming_archive::cMetadataDB::File::SegmentId,
                       streaming_archive::cMetadataDB::GLTFile::SegmentLogtypesPosition);
    }

    void GLTMetadataDB::add_storage_specific_field_names_and_types (
            vector<std::pair<string, string>>& file_field_names_and_types) {

        file_field_names_and_types[
                enum_to_underlying_type(GLTFilesTableFieldIndexes::SegmentLogtypesPosition)
        ] = make_pair(streaming_archive::cMetadataDB::GLTFile::SegmentLogtypesPosition,
                      streaming_archive::cMetadataDB::DataType::Integer);

        file_field_names_and_types[
                enum_to_underlying_type(GLTFilesTableFieldIndexes::SegmentOffsetPosition)
        ] = make_pair(streaming_archive::cMetadataDB::GLTFile::SegmentOffsetPosition,
                      streaming_archive::cMetadataDB::DataType::Integer);
    }

    void GLTMetadataDB::bind_storage_specific_fields (writer::File* file) {

        writer::GLTFile* glt_file_ptr = dynamic_cast<writer::GLTFile*>(file);

        m_upsert_file_statement->bind_int64(
                enum_to_underlying_type(GLTFilesTableFieldIndexes::SegmentLogtypesPosition) + 1,
                (int64_t)glt_file_ptr->get_segment_logtypes_pos());
        m_upsert_file_statement->bind_int64(
                enum_to_underlying_type(GLTFilesTableFieldIndexes::SegmentOffsetPosition) + 1,
                (int64_t)glt_file_ptr->get_segment_offset_pos());
    }
}
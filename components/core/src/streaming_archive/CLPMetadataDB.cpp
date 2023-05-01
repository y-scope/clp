#include "CLPMetadataDB.hpp"

// Project headers
#include "writer/CLP/CLPFile.hpp"

using std::back_insert_iterator;
using std::make_pair;
using std::string;
using std::vector;

namespace streaming_archive::clp {
    void CLPMetadataDB::add_storage_specific_fields (vector<string>& field_names) {
        field_names[enum_to_underlying_type(CLPFilesTableFieldIndexes::SegmentTimestampsPosition)]
            = streaming_archive::cMetadataDB::CLPFile::SegmentTimestampsPosition;
        field_names[enum_to_underlying_type(CLPFilesTableFieldIndexes::SegmentLogtypesPosition)] =
                streaming_archive::cMetadataDB::CLPFile::SegmentLogtypesPosition;
        field_names[enum_to_underlying_type(CLPFilesTableFieldIndexes::SegmentVariablesPosition)] =
                streaming_archive::cMetadataDB::CLPFile::SegmentVariablesPosition;
    }

    CLPMetadataDB::CLPFileIterator::CLPFileIterator (MetadataDB* mdb_ptr, SQLiteDB& db,
                                                     epochtime_t begin_timestamp,
                                                     epochtime_t end_timestamp,
                                                     const string& file_path,
                                                     bool in_specific_segment,
                                                     segment_id_t segment_id) :
            FileIterator(mdb_ptr, db, begin_timestamp, end_timestamp, file_path,
                         in_specific_segment, segment_id) {}

    size_t CLPMetadataDB::CLPFileIterator::get_segment_timestamps_pos () const {
        return m_statement.column_int64(
                enum_to_underlying_type(CLPFilesTableFieldIndexes::SegmentTimestampsPosition));
    }

    size_t CLPMetadataDB::CLPFileIterator::get_segment_logtypes_pos () const {
        return m_statement.column_int64(
                enum_to_underlying_type(CLPFilesTableFieldIndexes::SegmentLogtypesPosition));
    }

    size_t CLPMetadataDB::CLPFileIterator::get_segment_variables_pos () const {
        return m_statement.column_int64(
                enum_to_underlying_type(CLPFilesTableFieldIndexes::SegmentVariablesPosition));
    }

    void CLPMetadataDB::create_storage_specific_index (
            back_insert_iterator<fmt::memory_buffer> statement_buffer_ix) {
        fmt::format_to(statement_buffer_ix,
                       "CREATE INDEX IF NOT EXISTS files_segment_order ON {} ({},{})",
                       streaming_archive::cMetadataDB::FilesTableName,
                       streaming_archive::cMetadataDB::File::SegmentId,
                       streaming_archive::cMetadataDB::CLPFile::SegmentTimestampsPosition);
    }

    void CLPMetadataDB::add_storage_specific_ordering (
            back_insert_iterator<fmt::memory_buffer> statement_buffer_ix) {
        fmt::format_to(statement_buffer_ix, " ORDER BY {} ASC, {} ASC",
                       streaming_archive::cMetadataDB::File::SegmentId,
                       streaming_archive::cMetadataDB::CLPFile::SegmentTimestampsPosition);
    }

    void CLPMetadataDB::add_storage_specific_field_names_and_types (
            vector<std::pair<string, string>>& file_field_names_and_types) {
        // timestamp position
        file_field_names_and_types[
            enum_to_underlying_type(CLPFilesTableFieldIndexes::SegmentTimestampsPosition)
        ] = make_pair(streaming_archive::cMetadataDB::CLPFile::SegmentTimestampsPosition,
                      streaming_archive::cMetadataDB::DataType::Integer);

        // logtype position
        file_field_names_and_types[
            enum_to_underlying_type(CLPFilesTableFieldIndexes::SegmentLogtypesPosition)
        ] = make_pair(streaming_archive::cMetadataDB::CLPFile::SegmentLogtypesPosition,
                      streaming_archive::cMetadataDB::DataType::Integer);

        // variable position
        file_field_names_and_types[
                enum_to_underlying_type(CLPFilesTableFieldIndexes::SegmentVariablesPosition)
        ] = make_pair(streaming_archive::cMetadataDB::CLPFile::SegmentVariablesPosition,
                      streaming_archive::cMetadataDB::DataType::Integer);
    }

    void CLPMetadataDB::bind_storage_specific_fields (writer::File* file) {

        writer::CLPFile* clp_file_ptr = dynamic_cast<writer::CLPFile*>(file);

        m_upsert_file_statement->bind_int64(
                enum_to_underlying_type(CLPFilesTableFieldIndexes::SegmentTimestampsPosition) + 1,
                (int64_t)clp_file_ptr->get_segment_timestamps_pos()
        );
        m_upsert_file_statement->bind_int64(
                enum_to_underlying_type(CLPFilesTableFieldIndexes::SegmentLogtypesPosition) + 1,
                (int64_t)clp_file_ptr->get_segment_logtypes_pos()
        );
        m_upsert_file_statement->bind_int64(
                enum_to_underlying_type(CLPFilesTableFieldIndexes::SegmentVariablesPosition) + 1,
                (int64_t)clp_file_ptr->get_segment_variables_pos()
        );
    }
}
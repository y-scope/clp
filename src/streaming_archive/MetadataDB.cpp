#include "MetadataDB.hpp"

// C++ standard libraries
#include <vector>

// Project headers
#include "../Defs.h"
#include "../Utils.hpp"
#include "Constants.hpp"

// Types
enum class FilesTableFieldIndexes : uint16_t {
    Id = 0,  // NOTE: This needs to be the first item in the list
    OrigFileId,
    Path,
    BeginTimestamp,
    EndTimestamp,
    TimestampPatterns,
    NumUncompressedBytes,
    NumMessages,
    NumVariables,
    IsSplit,
    SplitIx,
    SegmentId,
    SegmentTimestampsPosition,
    SegmentLogtypesPosition,
    SegmentVariablesPosition,
    Length,
};

using std::make_unique;
using std::string;
using std::to_string;
using std::vector;

namespace streaming_archive {
    static void create_tables (const vector<std::pair<string, string>>& file_field_names_and_types, SQLiteDB& db) {
        string statement_string = "CREATE TABLE IF NOT EXISTS " STREAMING_ARCHIVE_METADATA_DB_FILES_TABLE_NAME " (";
        for (const auto& field_name_and_type : file_field_names_and_types) {
            statement_string += field_name_and_type.first;
            statement_string += ' ';
            statement_string += field_name_and_type.second;
            statement_string += ',';
        }
        // Trim extra trailing comma
        statement_string.resize(statement_string.length() - 1);
        statement_string += ") WITHOUT ROWID";
        auto create_files_table = db.prepare_statement(statement_string);
        create_files_table.step();

        statement_string = "CREATE INDEX IF NOT EXISTS files_segment_order ON " STREAMING_ARCHIVE_METADATA_DB_FILES_TABLE_NAME " ("
                           STREAMING_ARCHIVE_METADATA_DB_FILE_SEGMENT_ID "," STREAMING_ARCHIVE_METADATA_DB_FILE_SEGMENT_TIMESTAMPS_POSITION
                           ")";
        auto create_index_statement = db.prepare_statement(statement_string);
        create_index_statement.step();

        statement_string = "CREATE INDEX IF NOT EXISTS files_begin_timestamp ON " STREAMING_ARCHIVE_METADATA_DB_FILES_TABLE_NAME " ("
                           STREAMING_ARCHIVE_METADATA_DB_FILE_BEGIN_TIMESTAMP
                           ")";
        create_index_statement = db.prepare_statement(statement_string);
        create_index_statement.step();

        statement_string = "CREATE INDEX IF NOT EXISTS files_end_timestamp ON " STREAMING_ARCHIVE_METADATA_DB_FILES_TABLE_NAME " ("
                           STREAMING_ARCHIVE_METADATA_DB_FILE_END_TIMESTAMP
                           ")";
        create_index_statement = db.prepare_statement(statement_string);
        create_index_statement.step();

        statement_string = "CREATE INDEX IF NOT EXISTS files_path ON " STREAMING_ARCHIVE_METADATA_DB_FILES_TABLE_NAME " ("
                           STREAMING_ARCHIVE_METADATA_DB_FILE_PATH
                           ")";
        create_index_statement = db.prepare_statement(statement_string);
        create_index_statement.step();

        statement_string = "CREATE INDEX IF NOT EXISTS files_segment_id ON " STREAMING_ARCHIVE_METADATA_DB_FILES_TABLE_NAME " ("
                           STREAMING_ARCHIVE_METADATA_DB_FILE_SEGMENT_ID
                           ")";
        create_index_statement = db.prepare_statement(statement_string);
        create_index_statement.step();

        statement_string = "CREATE TABLE IF NOT EXISTS " STREAMING_ARCHIVE_METADATA_DB_EMPTY_DIRECTORIES_TABLE_NAME
                                  " (" STREAMING_ARCHIVE_METADATA_DB_EMPTY_DIRECTORY_PATH " TEXT PRIMARY KEY) WITHOUT ROWID";
        auto create_empty_directories_table = db.prepare_statement(statement_string);
        create_empty_directories_table.step();
    }

    MetadataDB::Iterator::Iterator (SQLitePreparedStatement statement) : m_statement(std::move(statement)) {
        m_statement.step();
    }

    void MetadataDB::Iterator::reset () {
        m_statement.reset();
        m_statement.step();
    }

    static SQLitePreparedStatement get_files_select_statement (SQLiteDB& db, epochtime_t ts_begin, epochtime_t ts_end, const std::string& file_path,
                                                               bool in_specific_segment, segment_id_t segment_id)
    {
        vector<string> field_names(enum_to_underlying_type(FilesTableFieldIndexes::Length));
        field_names[enum_to_underlying_type(FilesTableFieldIndexes::Id)] = STREAMING_ARCHIVE_METADATA_DB_FILE_ID;
        field_names[enum_to_underlying_type(FilesTableFieldIndexes::OrigFileId)] = STREAMING_ARCHIVE_METADATA_DB_FILE_ORIG_FILE_ID;
        field_names[enum_to_underlying_type(FilesTableFieldIndexes::Path)] = STREAMING_ARCHIVE_METADATA_DB_FILE_PATH;
        field_names[enum_to_underlying_type(FilesTableFieldIndexes::BeginTimestamp)] = STREAMING_ARCHIVE_METADATA_DB_FILE_BEGIN_TIMESTAMP;
        field_names[enum_to_underlying_type(FilesTableFieldIndexes::EndTimestamp)] = STREAMING_ARCHIVE_METADATA_DB_FILE_END_TIMESTAMP;
        field_names[enum_to_underlying_type(FilesTableFieldIndexes::TimestampPatterns)] = STREAMING_ARCHIVE_METADATA_DB_FILE_TIMESTAMP_PATTERNS;
        field_names[enum_to_underlying_type(FilesTableFieldIndexes::NumUncompressedBytes)] = STREAMING_ARCHIVE_METADATA_DB_FILE_NUM_UNCOMPRESSED_BYTES;
        field_names[enum_to_underlying_type(FilesTableFieldIndexes::NumMessages)] = STREAMING_ARCHIVE_METADATA_DB_FILE_NUM_MESSAGES;
        field_names[enum_to_underlying_type(FilesTableFieldIndexes::NumVariables)] = STREAMING_ARCHIVE_METADATA_DB_FILE_NUM_VARIABLES;
        field_names[enum_to_underlying_type(FilesTableFieldIndexes::IsSplit)] = STREAMING_ARCHIVE_METADATA_DB_FILE_IS_SPLIT;
        field_names[enum_to_underlying_type(FilesTableFieldIndexes::SplitIx)] = STREAMING_ARCHIVE_METADATA_DB_FILE_SPLIT_IX;
        field_names[enum_to_underlying_type(FilesTableFieldIndexes::SegmentId)] = STREAMING_ARCHIVE_METADATA_DB_FILE_SEGMENT_ID;
        field_names[enum_to_underlying_type(FilesTableFieldIndexes::SegmentTimestampsPosition)] =
                STREAMING_ARCHIVE_METADATA_DB_FILE_SEGMENT_TIMESTAMPS_POSITION;
        field_names[enum_to_underlying_type(FilesTableFieldIndexes::SegmentLogtypesPosition)] = STREAMING_ARCHIVE_METADATA_DB_FILE_SEGMENT_LOGTYPES_POSITION;
        field_names[enum_to_underlying_type(FilesTableFieldIndexes::SegmentVariablesPosition)] = STREAMING_ARCHIVE_METADATA_DB_FILE_SEGMENT_VARIABLES_POSITION;

        string file_select_statement_string = "SELECT ";
        for (const auto& field_name : field_names) {
            file_select_statement_string += field_name;
            file_select_statement_string += ',';
        }
        // Remove trailing comma
        file_select_statement_string.resize(file_select_statement_string.length() - 1);
        file_select_statement_string += " FROM " STREAMING_ARCHIVE_METADATA_DB_FILES_TABLE_NAME;

        // Add clauses
        bool clause_exists = false;
        if (cEpochTimeMin != ts_begin) {
            file_select_statement_string += " WHERE " STREAMING_ARCHIVE_METADATA_DB_FILE_BEGIN_TIMESTAMP " >= ?";
            file_select_statement_string += to_string(enum_to_underlying_type(FilesTableFieldIndexes::BeginTimestamp) + 1);
            clause_exists = true;
        }
        if (cEpochTimeMax != ts_end) {
            file_select_statement_string += clause_exists ? " AND " : " WHERE ";
            file_select_statement_string += STREAMING_ARCHIVE_METADATA_DB_FILE_END_TIMESTAMP " <= ?";
            file_select_statement_string += to_string(enum_to_underlying_type(FilesTableFieldIndexes::EndTimestamp) + 1);
            clause_exists = true;
        }
        if (false == file_path.empty()) {
            file_select_statement_string += clause_exists ? " AND " : " WHERE ";
            file_select_statement_string += STREAMING_ARCHIVE_METADATA_DB_FILE_PATH " = ?";
            file_select_statement_string += to_string(enum_to_underlying_type(FilesTableFieldIndexes::Path) + 1);
            clause_exists = true;
        }
        if (in_specific_segment) {
            file_select_statement_string += clause_exists ? " AND " : " WHERE ";
            file_select_statement_string += STREAMING_ARCHIVE_METADATA_DB_FILE_SEGMENT_ID " = ?";
            file_select_statement_string += to_string(enum_to_underlying_type(FilesTableFieldIndexes::SegmentId) + 1);
            clause_exists = true;
        }

        // Add ordering
        file_select_statement_string += " ORDER BY " STREAMING_ARCHIVE_METADATA_DB_FILE_SEGMENT_ID " ASC, "
                                        STREAMING_ARCHIVE_METADATA_DB_FILE_SEGMENT_TIMESTAMPS_POSITION " ASC";

        auto statement = db.prepare_statement(file_select_statement_string);
        if (cEpochTimeMin != ts_begin) {
            statement.bind_int64(enum_to_underlying_type(FilesTableFieldIndexes::BeginTimestamp) + 1, ts_begin);
        }
        if (cEpochTimeMax != ts_end) {
            statement.bind_int64(enum_to_underlying_type(FilesTableFieldIndexes::EndTimestamp) + 1, ts_end);
        }
        if (false == file_path.empty()) {
            statement.bind_text(enum_to_underlying_type(FilesTableFieldIndexes::Path) + 1, file_path, true);
        }
        if (in_specific_segment) {
            statement.bind_int64(enum_to_underlying_type(FilesTableFieldIndexes::SegmentId) + 1, (int64_t)segment_id);
        }

        return statement;
    }

    static SQLitePreparedStatement get_empty_directories_select_statement (SQLiteDB& db) {
        string statement_string = "SELECT " STREAMING_ARCHIVE_METADATA_DB_EMPTY_DIRECTORY_PATH
                " FROM " STREAMING_ARCHIVE_METADATA_DB_EMPTY_DIRECTORIES_TABLE_NAME;
        return db.prepare_statement(statement_string);
    }

    MetadataDB::FileIterator::FileIterator (SQLiteDB& db, epochtime_t begin_timestamp, epochtime_t end_timestamp, const std::string& file_path,
                                            bool in_specific_segment, segment_id_t segment_id) :
                                            Iterator(get_files_select_statement(db, begin_timestamp, end_timestamp, file_path, in_specific_segment,
                                                                                segment_id)) {}

    MetadataDB::EmptyDirectoryIterator::EmptyDirectoryIterator (SQLiteDB& db) : Iterator(get_empty_directories_select_statement(db)) {}

    void MetadataDB::FileIterator::set_segment_id (segment_id_t segment_id) {
        m_statement.reset();

        m_statement.bind_int64(enum_to_underlying_type(FilesTableFieldIndexes::SegmentId) + 1, (int64_t)segment_id);

        m_statement.step();
    }

    void MetadataDB::FileIterator::get_id (string& id) const {
        m_statement.column_string(enum_to_underlying_type(FilesTableFieldIndexes::Id), id);
    }

    void MetadataDB::FileIterator::get_orig_file_id (string& id) const {
        m_statement.column_string(enum_to_underlying_type(FilesTableFieldIndexes::OrigFileId), id);
    }

    void MetadataDB::FileIterator::get_path (string& path) const {
        m_statement.column_string(enum_to_underlying_type(FilesTableFieldIndexes::Path), path);
    }

    epochtime_t MetadataDB::FileIterator::get_begin_ts () const {
        return m_statement.column_int64(enum_to_underlying_type(FilesTableFieldIndexes::BeginTimestamp));
    }

    epochtime_t MetadataDB::FileIterator::get_end_ts () const {
        return m_statement.column_int64(enum_to_underlying_type(FilesTableFieldIndexes::EndTimestamp));
    }

    void MetadataDB::FileIterator::get_timestamp_patterns (string& timestamp_patterns) const {
        m_statement.column_string(enum_to_underlying_type(FilesTableFieldIndexes::TimestampPatterns), timestamp_patterns);
    }

    size_t MetadataDB::FileIterator::get_num_uncompressed_bytes () const {
        return m_statement.column_int64(enum_to_underlying_type(FilesTableFieldIndexes::NumUncompressedBytes));
    }

    size_t MetadataDB::FileIterator::get_num_messages () const {
        return m_statement.column_int64(enum_to_underlying_type(FilesTableFieldIndexes::NumMessages));
    }

    size_t MetadataDB::FileIterator::get_num_variables () const {
        return m_statement.column_int64(enum_to_underlying_type(FilesTableFieldIndexes::NumVariables));
    }

    bool MetadataDB::FileIterator::is_split () const {
        return m_statement.column_int(enum_to_underlying_type(FilesTableFieldIndexes::IsSplit));
    }

    size_t MetadataDB::FileIterator::get_split_ix () const {
        return m_statement.column_int64(enum_to_underlying_type(FilesTableFieldIndexes::SplitIx));
    }

    segment_id_t MetadataDB::FileIterator::get_segment_id () const {
        return m_statement.column_int64(enum_to_underlying_type(FilesTableFieldIndexes::SegmentId));
    }

    size_t MetadataDB::FileIterator::get_segment_timestamps_pos () const {
        return m_statement.column_int64(enum_to_underlying_type(FilesTableFieldIndexes::SegmentTimestampsPosition));
    }

    size_t MetadataDB::FileIterator::get_segment_logtypes_pos () const {
        return m_statement.column_int64(enum_to_underlying_type(FilesTableFieldIndexes::SegmentLogtypesPosition));
    }

    size_t MetadataDB::FileIterator::get_segment_variables_pos () const {
        return m_statement.column_int64(enum_to_underlying_type(FilesTableFieldIndexes::SegmentVariablesPosition));
    }

    void MetadataDB::open (const string& path) {
        if (m_is_open) {
            throw OperationFailed(ErrorCode_NotReady, __FILENAME__, __LINE__);
        }

        m_db.open(path);

        vector<std::pair<string, string>> file_field_names_and_types(enum_to_underlying_type(FilesTableFieldIndexes::Length));
        file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::Id)].first = STREAMING_ARCHIVE_METADATA_DB_FILE_ID;
        file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::Id)].second = "TEXT PRIMARY KEY";

        file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::OrigFileId)].first = STREAMING_ARCHIVE_METADATA_DB_FILE_ORIG_FILE_ID;
        file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::OrigFileId)].second = "TEXT";

        file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::Path)].first = STREAMING_ARCHIVE_METADATA_DB_FILE_PATH;
        file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::Path)].second = "TEXT";

        file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::BeginTimestamp)].first = STREAMING_ARCHIVE_METADATA_DB_FILE_BEGIN_TIMESTAMP;
        file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::BeginTimestamp)].second = "INTEGER";

        file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::EndTimestamp)].first = STREAMING_ARCHIVE_METADATA_DB_FILE_END_TIMESTAMP;
        file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::EndTimestamp)].second = "INTEGER";

        file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::TimestampPatterns)].first =
                STREAMING_ARCHIVE_METADATA_DB_FILE_TIMESTAMP_PATTERNS;
        file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::TimestampPatterns)].second = "TEXT";

        file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::NumUncompressedBytes)].first =
                STREAMING_ARCHIVE_METADATA_DB_FILE_NUM_UNCOMPRESSED_BYTES;
        file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::NumUncompressedBytes)].second = "INTEGER";

        file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::NumMessages)].first = STREAMING_ARCHIVE_METADATA_DB_FILE_NUM_MESSAGES;
        file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::NumMessages)].second = "INTEGER";

        file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::NumVariables)].first = STREAMING_ARCHIVE_METADATA_DB_FILE_NUM_VARIABLES;
        file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::NumVariables)].second = "INTEGER";

        file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::IsSplit)].first = STREAMING_ARCHIVE_METADATA_DB_FILE_IS_SPLIT;
        file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::IsSplit)].second = "INTEGER";

        file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::SplitIx)].first = STREAMING_ARCHIVE_METADATA_DB_FILE_SPLIT_IX;
        file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::SplitIx)].second = "INTEGER";

        file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::SegmentId)].first = STREAMING_ARCHIVE_METADATA_DB_FILE_SEGMENT_ID;
        file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::SegmentId)].second = "INTEGER";

        file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::SegmentTimestampsPosition)].first =
                STREAMING_ARCHIVE_METADATA_DB_FILE_SEGMENT_TIMESTAMPS_POSITION;
        file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::SegmentTimestampsPosition)].second = "INTEGER";

        file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::SegmentLogtypesPosition)].first =
                STREAMING_ARCHIVE_METADATA_DB_FILE_SEGMENT_LOGTYPES_POSITION;
        file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::SegmentLogtypesPosition)].second = "INTEGER";

        file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::SegmentVariablesPosition)].first =
                STREAMING_ARCHIVE_METADATA_DB_FILE_SEGMENT_VARIABLES_POSITION;
        file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::SegmentVariablesPosition)].second = "INTEGER";

        create_tables(file_field_names_and_types, m_db);

        string statement_string = "INSERT INTO " STREAMING_ARCHIVE_METADATA_DB_FILES_TABLE_NAME " (";
        for (const auto& field_name_and_type : file_field_names_and_types) {
            statement_string += field_name_and_type.first;
            statement_string += ',';
        }
        // Remove trailing comma
        statement_string.resize(statement_string.length() - 1);

        statement_string += ") VALUES (";
        for (size_t i = 0; i < file_field_names_and_types.size(); ++i) {
            statement_string += '?';
            statement_string += to_string(i + 1);
            statement_string += ',';
        }
        // Remove trailing comma
        statement_string.resize(statement_string.length() - 1);
        statement_string += ") ON CONFLICT (" STREAMING_ARCHIVE_METADATA_DB_FILE_ID ") DO UPDATE SET ";
        // Set all fields except the ID
        for (size_t i = enum_to_underlying_type(FilesTableFieldIndexes::Id) + 1; i < file_field_names_and_types.size(); ++i) {
            const auto& field_name = file_field_names_and_types[i].first;
            statement_string += field_name;
            statement_string += " = ?";
            statement_string += to_string(i + 1);
            statement_string += ',';
        }
        // Remove trailing comma
        statement_string.resize(statement_string.length() - 1);

        m_transaction_begin_statement = make_unique<SQLitePreparedStatement>(m_db.prepare_statement("BEGIN TRANSACTION"));
        m_transaction_end_statement = make_unique<SQLitePreparedStatement>(m_db.prepare_statement("END TRANSACTION"));
        m_upsert_file_statement = make_unique<SQLitePreparedStatement>(m_db.prepare_statement(statement_string));

        statement_string = "INSERT INTO " STREAMING_ARCHIVE_METADATA_DB_EMPTY_DIRECTORIES_TABLE_NAME
                " (" STREAMING_ARCHIVE_METADATA_DB_EMPTY_DIRECTORY_PATH ") VALUES (?) ON CONFLICT DO NOTHING";
        m_insert_empty_directories_statement = make_unique<SQLitePreparedStatement>(m_db.prepare_statement(statement_string));
    }

    void MetadataDB::close () {
        m_transaction_begin_statement.reset(nullptr);
        m_transaction_end_statement.reset(nullptr);
        m_upsert_file_statement.reset(nullptr);
        m_insert_empty_directories_statement.reset(nullptr);
        if (false == m_db.close()) {
            SPDLOG_ERROR("streaming_archive::MetadataDB: Failed to close database - {}", m_db.get_error_message());
            throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
        }
        m_is_open = false;
    }

    void MetadataDB::update_files (const vector<writer::File*>& files) {
        m_transaction_begin_statement->step();
        for (auto file : files) {
            m_upsert_file_statement->bind_text(enum_to_underlying_type(FilesTableFieldIndexes::Id) + 1, file->get_id_as_string(), false);
            m_upsert_file_statement->bind_text(enum_to_underlying_type(FilesTableFieldIndexes::OrigFileId) + 1, file->get_orig_file_id_as_string(), false);
            m_upsert_file_statement->bind_text(enum_to_underlying_type(FilesTableFieldIndexes::Path) + 1, file->get_orig_path(), false);
            m_upsert_file_statement->bind_int64(enum_to_underlying_type(FilesTableFieldIndexes::BeginTimestamp) + 1, file->get_begin_ts());
            m_upsert_file_statement->bind_int64(enum_to_underlying_type(FilesTableFieldIndexes::EndTimestamp) + 1, file->get_end_ts());
            m_upsert_file_statement->bind_text(enum_to_underlying_type(FilesTableFieldIndexes::TimestampPatterns) + 1, file->get_encoded_timestamp_patterns(),
                                               true);
            m_upsert_file_statement->bind_int64(enum_to_underlying_type(FilesTableFieldIndexes::NumUncompressedBytes) + 1,
                                                (int64_t)file->get_num_uncompressed_bytes());
            m_upsert_file_statement->bind_int64(enum_to_underlying_type(FilesTableFieldIndexes::NumMessages) + 1, (int64_t)file->get_num_messages());
            m_upsert_file_statement->bind_int64(enum_to_underlying_type(FilesTableFieldIndexes::NumVariables) + 1, (int64_t)file->get_num_variables());
            m_upsert_file_statement->bind_int64(enum_to_underlying_type(FilesTableFieldIndexes::IsSplit) + 1, (int64_t)file->is_split());
            m_upsert_file_statement->bind_int64(enum_to_underlying_type(FilesTableFieldIndexes::SplitIx) + 1, (int64_t)file->get_split_ix());
            m_upsert_file_statement->bind_int64(enum_to_underlying_type(FilesTableFieldIndexes::SegmentId) + 1, (int64_t)file->get_segment_id());
            m_upsert_file_statement->bind_int64(enum_to_underlying_type(FilesTableFieldIndexes::SegmentTimestampsPosition) + 1,
                                                (int64_t)file->get_segment_timestamps_pos());
            m_upsert_file_statement->bind_int64(enum_to_underlying_type(FilesTableFieldIndexes::SegmentLogtypesPosition) + 1,
                                                (int64_t)file->get_segment_logtypes_pos());
            m_upsert_file_statement->bind_int64(enum_to_underlying_type(FilesTableFieldIndexes::SegmentVariablesPosition) + 1,
                                                (int64_t)file->get_segment_variables_pos());

            m_upsert_file_statement->step();
            m_upsert_file_statement->reset();
        }
        m_transaction_end_statement->step();

        m_transaction_begin_statement->reset();
        m_transaction_end_statement->reset();
    }

    void MetadataDB::add_empty_directories (const vector<string>& empty_directory_paths) {
        for (const auto& path : empty_directory_paths) {
            m_insert_empty_directories_statement->bind_text(1, path, false);
            m_insert_empty_directories_statement->step();
            m_insert_empty_directories_statement->reset();
        }
    }
}

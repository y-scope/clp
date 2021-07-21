#include "GlobalMetadataDB.hpp"

// C++ standard libraries
#include <tuple>
#include <utility>

// spdlog
#include <spdlog/spdlog.h>

// Project headers
#include "Defs.h"
#include "streaming_archive/Constants.hpp"
#include "Utils.hpp"

// Types
enum class ArchivesTableFieldIndexes : uint16_t {
    Id = 0,
    StorageId,
    UncompressedSize,
    Size,
    CreatorId,
    CreationIx,
    Length,
};
enum class UpdateArchiveSizeStmtFieldIndexes : uint16_t {
    UncompressedSize = 0,
    Size,
    Id,
    Length,
};
enum class FilesTableFieldIndexes : uint16_t {
    Id = 0,  // NOTE: This needs to be the first item in the list
    OrigFileId,
    Path,
    BeginTimestamp,
    EndTimestamp,
    NumUncompressedBytes,
    NumMessages,
    ArchiveId,
    Length,
};

using std::pair;
using std::string;
using std::to_string;
using std::unordered_set;
using std::vector;

static void create_tables (const vector<std::pair<string, string>>& archive_field_names_and_types,
                           const vector<pair<string, string>>& file_field_names_and_types, SQLiteDB& db)
{
    string statement_string = "CREATE TABLE IF NOT EXISTS " STREAMING_ARCHIVE_METADATA_DB_ARCHIVES_TABLE_NAME " (";
    for (const auto& field_name_and_type : archive_field_names_and_types) {
        statement_string += field_name_and_type.first;
        statement_string += ' ';
        statement_string += field_name_and_type.second;
        statement_string += ',';
    }
    // Trim extra trailing comma
    statement_string.resize(statement_string.length() - 1);
    statement_string += ") WITHOUT ROWID";
    auto create_archives_table = db.prepare_statement(statement_string);
    create_archives_table.step();

    statement_string = "CREATE INDEX IF NOT EXISTS archives_creation_order ON " STREAMING_ARCHIVE_METADATA_DB_ARCHIVES_TABLE_NAME " ("
            STREAMING_ARCHIVE_METADATA_DB_ARCHIVE_CREATOR_ID "," STREAMING_ARCHIVE_METADATA_DB_ARCHIVE_CREATION_IX
            ")";
    auto create_archives_index = db.prepare_statement(statement_string);
    create_archives_index.step();

    statement_string = "CREATE TABLE IF NOT EXISTS " STREAMING_ARCHIVE_METADATA_DB_FILES_TABLE_NAME " (";
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

    statement_string = "CREATE INDEX IF NOT EXISTS files_path ON " STREAMING_ARCHIVE_METADATA_DB_FILES_TABLE_NAME " ("
                       STREAMING_ARCHIVE_METADATA_DB_FILE_PATH
                       ")";
    auto create_files_path_index = db.prepare_statement(statement_string);
    create_files_path_index.step();

    statement_string = "CREATE INDEX IF NOT EXISTS files_archive_id ON " STREAMING_ARCHIVE_METADATA_DB_FILES_TABLE_NAME " ("
                       STREAMING_ARCHIVE_METADATA_DB_FILE_ARCHIVE_ID
                       ")";
    auto create_files_archive_id_index = db.prepare_statement(statement_string);
    create_files_archive_id_index.step();
}

static SQLitePreparedStatement get_archives_select_statement (SQLiteDB& db) {
    string statement_string = "SELECT " STREAMING_ARCHIVE_METADATA_DB_ARCHIVE_ID
            " FROM " STREAMING_ARCHIVE_METADATA_DB_ARCHIVES_TABLE_NAME;

    return db.prepare_statement(statement_string);
}

static SQLitePreparedStatement get_archives_for_file_select_statement (SQLiteDB& db, const string& file_path) {
    string statement_string = "SELECT DISTINCT " STREAMING_ARCHIVE_METADATA_DB_ARCHIVES_TABLE_NAME "." STREAMING_ARCHIVE_METADATA_DB_ARCHIVE_ID
            " FROM " STREAMING_ARCHIVE_METADATA_DB_ARCHIVES_TABLE_NAME
            " JOIN " STREAMING_ARCHIVE_METADATA_DB_FILES_TABLE_NAME
            " ON " STREAMING_ARCHIVE_METADATA_DB_ARCHIVES_TABLE_NAME "." STREAMING_ARCHIVE_METADATA_DB_ARCHIVE_ID
            " = " STREAMING_ARCHIVE_METADATA_DB_FILES_TABLE_NAME "." STREAMING_ARCHIVE_METADATA_DB_FILE_ARCHIVE_ID
            " WHERE " STREAMING_ARCHIVE_METADATA_DB_FILES_TABLE_NAME "." STREAMING_ARCHIVE_METADATA_DB_FILE_PATH
            " = ?"
            " ORDER BY " STREAMING_ARCHIVE_METADATA_DB_ARCHIVE_CREATOR_ID " ASC, " STREAMING_ARCHIVE_METADATA_DB_ARCHIVE_CREATION_IX " ASC";

    auto statement = db.prepare_statement(statement_string);
    statement.bind_text(1, file_path, true);

    return statement;
}

GlobalMetadataDB::Iterator::Iterator (SQLitePreparedStatement statement) : m_statement(std::move(statement)) {
    m_statement.step();
}

bool GlobalMetadataDB::Iterator::has_next () {
    return m_statement.is_row_ready();
}

void GlobalMetadataDB::Iterator::next () {
    m_statement.step();
}

GlobalMetadataDB::ArchiveIterator::ArchiveIterator (SQLiteDB& db) : GlobalMetadataDB::Iterator::Iterator(get_archives_select_statement(db)) {}

GlobalMetadataDB::ArchiveIterator::ArchiveIterator (SQLiteDB& db, const string& file_path) :
        GlobalMetadataDB::Iterator::Iterator(get_archives_for_file_select_statement(db, file_path)) {}

void GlobalMetadataDB::ArchiveIterator::get_id (string& id) const {
    m_statement.column_string(0, id);
}

void GlobalMetadataDB::open (const string& path) {
    if (m_is_open) {
        throw OperationFailed(ErrorCode_NotReady, __FILENAME__, __LINE__);
    }

    m_db.open(path);

    vector<pair<string, string>> archive_field_names_and_types(enum_to_underlying_type(ArchivesTableFieldIndexes::Length));
    archive_field_names_and_types[enum_to_underlying_type(ArchivesTableFieldIndexes::Id)].first = STREAMING_ARCHIVE_METADATA_DB_ARCHIVE_ID;
    archive_field_names_and_types[enum_to_underlying_type(ArchivesTableFieldIndexes::Id)].second = "TEXT PRIMARY KEY";

    archive_field_names_and_types[enum_to_underlying_type(ArchivesTableFieldIndexes::StorageId)].first =
            STREAMING_ARCHIVE_METADATA_DB_ARCHIVE_STORAGE_ID;
    archive_field_names_and_types[enum_to_underlying_type(ArchivesTableFieldIndexes::StorageId)].second = "TEXT";

    archive_field_names_and_types[enum_to_underlying_type(ArchivesTableFieldIndexes::UncompressedSize)].first =
            STREAMING_ARCHIVE_METADATA_DB_ARCHIVE_UNCOMPRESSED_SIZE;
    archive_field_names_and_types[enum_to_underlying_type(ArchivesTableFieldIndexes::UncompressedSize)].second = "INTEGER";

    archive_field_names_and_types[enum_to_underlying_type(ArchivesTableFieldIndexes::Size)].first = STREAMING_ARCHIVE_METADATA_DB_ARCHIVE_SIZE;
    archive_field_names_and_types[enum_to_underlying_type(ArchivesTableFieldIndexes::Size)].second = "INTEGER";

    archive_field_names_and_types[enum_to_underlying_type(ArchivesTableFieldIndexes::CreatorId)].first =
            STREAMING_ARCHIVE_METADATA_DB_ARCHIVE_CREATOR_ID;
    archive_field_names_and_types[enum_to_underlying_type(ArchivesTableFieldIndexes::CreatorId)].second = "TEXT";

    archive_field_names_and_types[enum_to_underlying_type(ArchivesTableFieldIndexes::CreationIx)].first =
            STREAMING_ARCHIVE_METADATA_DB_ARCHIVE_CREATION_IX;
    archive_field_names_and_types[enum_to_underlying_type(ArchivesTableFieldIndexes::CreationIx)].second = "INTEGER";

    vector<pair<string, string>> file_field_names_and_types(enum_to_underlying_type(FilesTableFieldIndexes::Length));
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

    file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::NumUncompressedBytes)].first =
            STREAMING_ARCHIVE_METADATA_DB_FILE_NUM_UNCOMPRESSED_BYTES;
    file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::NumUncompressedBytes)].second = "INTEGER";

    file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::NumMessages)].first = STREAMING_ARCHIVE_METADATA_DB_FILE_NUM_MESSAGES;
    file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::NumMessages)].second = "INTEGER";

    file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::ArchiveId)].first = STREAMING_ARCHIVE_METADATA_DB_FILE_ARCHIVE_ID;
    file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::ArchiveId)].second = "TEXT";

    create_tables(archive_field_names_and_types, file_field_names_and_types, m_db);

    string statement_string = "INSERT INTO " STREAMING_ARCHIVE_METADATA_DB_ARCHIVES_TABLE_NAME " (";
    for (const auto& field_name_and_type : archive_field_names_and_types) {
        statement_string += field_name_and_type.first;
        statement_string += ',';
    }
    // Remove trailing comma
    statement_string.resize(statement_string.length() - 1);
    statement_string += ") VALUES (";
    for (size_t i = 0; i < archive_field_names_and_types.size(); ++i) {
        statement_string += "?,";
    }
    // Remove trailing comma
    statement_string.resize(statement_string.length() - 1);
    statement_string += ")";

    m_insert_archive_statement = std::make_unique<SQLitePreparedStatement>(m_db.prepare_statement(statement_string));

    vector<string> update_archive_size_stmt_field_names(enum_to_underlying_type(UpdateArchiveSizeStmtFieldIndexes::Length));
    update_archive_size_stmt_field_names[enum_to_underlying_type(UpdateArchiveSizeStmtFieldIndexes::Id)] =
            STREAMING_ARCHIVE_METADATA_DB_ARCHIVE_ID;
    update_archive_size_stmt_field_names[enum_to_underlying_type(UpdateArchiveSizeStmtFieldIndexes::UncompressedSize)] =
            STREAMING_ARCHIVE_METADATA_DB_ARCHIVE_UNCOMPRESSED_SIZE;
    update_archive_size_stmt_field_names[enum_to_underlying_type(UpdateArchiveSizeStmtFieldIndexes::Size)] =
            STREAMING_ARCHIVE_METADATA_DB_ARCHIVE_SIZE;
    statement_string = "UPDATE " STREAMING_ARCHIVE_METADATA_DB_ARCHIVES_TABLE_NAME " SET ";
    for (size_t i = 0; i < update_archive_size_stmt_field_names.size() - 1; ++i) {
        const auto& field_name = update_archive_size_stmt_field_names[i];
        statement_string += field_name;
        statement_string += " = ?";
        statement_string += to_string(i + 1);
        statement_string += ',';
    }
    // Remove trailing comma
    statement_string.resize(statement_string.length() - 1);
    statement_string += " WHERE ";
    statement_string += update_archive_size_stmt_field_names[enum_to_underlying_type(UpdateArchiveSizeStmtFieldIndexes::Id)];
    statement_string += " = ?";
    statement_string += to_string(enum_to_underlying_type(UpdateArchiveSizeStmtFieldIndexes::Id) + 1);

    m_update_archive_size_statement = std::make_unique<SQLitePreparedStatement>(m_db.prepare_statement(statement_string));

    statement_string = "INSERT INTO " STREAMING_ARCHIVE_METADATA_DB_FILES_TABLE_NAME " (";
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

    m_upsert_file_statement = std::make_unique<SQLitePreparedStatement>(m_db.prepare_statement(statement_string));
    m_upsert_files_transaction_begin_statement = std::make_unique<SQLitePreparedStatement>(m_db.prepare_statement("BEGIN TRANSACTION"));
    m_upsert_files_transaction_end_statement = std::make_unique<SQLitePreparedStatement>(m_db.prepare_statement("END TRANSACTION"));

    m_is_open = true;
}

void GlobalMetadataDB::close () {
    m_insert_archive_statement.reset(nullptr);
    m_update_archive_size_statement.reset(nullptr);
    m_upsert_file_statement.reset(nullptr);
    m_upsert_files_transaction_begin_statement.reset(nullptr);
    m_upsert_files_transaction_end_statement.reset(nullptr);
    if (false == m_db.close()) {
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }
    m_is_open = false;
}

void GlobalMetadataDB::add_archive (const string& id, const string& storage_id, size_t uncompressed_size, size_t size, const string& creator_id,
                                    size_t creation_num)
{
    if (false == m_is_open) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    m_insert_archive_statement->bind_text(enum_to_underlying_type(ArchivesTableFieldIndexes::Id) + 1, id, false);
    m_insert_archive_statement->bind_text(enum_to_underlying_type(ArchivesTableFieldIndexes::StorageId) + 1, storage_id, false);
    m_insert_archive_statement->bind_int64(enum_to_underlying_type(ArchivesTableFieldIndexes::UncompressedSize) + 1, (int64_t)uncompressed_size);
    m_insert_archive_statement->bind_int64(enum_to_underlying_type(ArchivesTableFieldIndexes::Size) + 1, (int64_t)size);
    m_insert_archive_statement->bind_text(enum_to_underlying_type(ArchivesTableFieldIndexes::CreatorId) + 1, creator_id, false);
    m_insert_archive_statement->bind_int64(enum_to_underlying_type(ArchivesTableFieldIndexes::CreationIx) + 1, (int64_t)creation_num);
    m_insert_archive_statement->step();
    m_insert_archive_statement->reset();
}

void GlobalMetadataDB::update_archive_size (const string& archive_id, size_t uncompressed_size, size_t size) {
    if (false == m_is_open) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    m_update_archive_size_statement->bind_int64(enum_to_underlying_type(UpdateArchiveSizeStmtFieldIndexes::UncompressedSize) + 1, (int64_t)uncompressed_size);
    m_update_archive_size_statement->bind_int64(enum_to_underlying_type(UpdateArchiveSizeStmtFieldIndexes::Size) + 1, (int64_t)size);
    m_update_archive_size_statement->bind_text(enum_to_underlying_type(UpdateArchiveSizeStmtFieldIndexes::Id) + 1, archive_id, false);
    m_update_archive_size_statement->step();
    m_update_archive_size_statement->reset();
}

void GlobalMetadataDB::update_files (const string& archive_id, const vector<streaming_archive::writer::File*>& files) {
    if (false == m_is_open) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    m_upsert_files_transaction_begin_statement->step();
    for (auto file : files) {
        m_upsert_file_statement->bind_text(enum_to_underlying_type(FilesTableFieldIndexes::Id) + 1, file->get_id_as_string(), false);
        m_upsert_file_statement->bind_text(enum_to_underlying_type(FilesTableFieldIndexes::OrigFileId) + 1, file->get_orig_file_id_as_string(), false);
        m_upsert_file_statement->bind_text(enum_to_underlying_type(FilesTableFieldIndexes::Path) + 1, file->get_orig_path(), false);
        m_upsert_file_statement->bind_int64(enum_to_underlying_type(FilesTableFieldIndexes::BeginTimestamp) + 1, file->get_begin_ts());
        m_upsert_file_statement->bind_int64(enum_to_underlying_type(FilesTableFieldIndexes::EndTimestamp) + 1, file->get_end_ts());
        m_upsert_file_statement->bind_int64(enum_to_underlying_type(FilesTableFieldIndexes::NumUncompressedBytes) + 1,
                                            (int64_t)file->get_num_uncompressed_bytes());
        m_upsert_file_statement->bind_int64(enum_to_underlying_type(FilesTableFieldIndexes::NumMessages) + 1, (int64_t)file->get_num_messages());
        m_upsert_file_statement->bind_text(enum_to_underlying_type(FilesTableFieldIndexes::ArchiveId) + 1, archive_id, false);

        m_upsert_file_statement->step();
        m_upsert_file_statement->reset();
    }
    m_upsert_files_transaction_end_statement->step();

    m_upsert_files_transaction_begin_statement->reset();
    m_upsert_files_transaction_end_statement->reset();
}

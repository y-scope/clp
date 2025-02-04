#include "GlobalSQLiteMetadataDB.hpp"

#include <tuple>
#include <utility>

#include <fmt/core.h>

#include "database_utils.hpp"
#include "spdlog_with_specializations.hpp"
#include "streaming_archive/Constants.hpp"
#include "type_utils.hpp"

// Types
enum class ArchivesTableFieldIndexes : uint16_t {
    Id = 0,
    BeginTimestamp,
    EndTimestamp,
    UncompressedSize,
    Size,
    CreatorId,
    CreationIx,
    Length,
};
enum class UpdateArchiveSizeStmtFieldIndexes : uint16_t {
    BeginTimestamp = 0,
    EndTimestamp,
    UncompressedSize,
    Size,
    Length,
};
enum class FilesTableFieldIndexes : uint16_t {
    Id = 0,  // NOTE: This needs to be the first item in the list
    OrigFileId,
    Path,
    BeginTimestamp,
    EndTimestamp,
    NumUncompressedBytes,
    BeginMessageIx,
    NumMessages,
    ArchiveId,
    Length,
};

using std::pair;
using std::string;
using std::to_string;
using std::unordered_set;
using std::vector;

namespace clp {
namespace {
void create_tables(
        vector<pair<string, string>> const& archive_field_names_and_types,
        vector<pair<string, string>> const& file_field_names_and_types,
        SQLiteDB& db
) {
    fmt::memory_buffer statement_buffer;
    auto statement_buffer_ix = std::back_inserter(statement_buffer);

    fmt::format_to(
            statement_buffer_ix,
            "CREATE TABLE IF NOT EXISTS {} ({}) WITHOUT ROWID",
            streaming_archive::cMetadataDB::ArchivesTableName,
            get_field_names_and_types_sql(archive_field_names_and_types)
    );
    SPDLOG_DEBUG("{:.{}}", statement_buffer.data(), statement_buffer.size());
    auto create_archives_table
            = db.prepare_statement(statement_buffer.data(), statement_buffer.size());
    create_archives_table.step();
    statement_buffer.clear();

    fmt::format_to(
            statement_buffer_ix,
            "CREATE INDEX IF NOT EXISTS archives_creation_order ON {} ({},{})",
            streaming_archive::cMetadataDB::ArchivesTableName,
            streaming_archive::cMetadataDB::Archive::CreatorId,
            streaming_archive::cMetadataDB::Archive::CreationIx
    );
    SPDLOG_DEBUG("{:.{}}", statement_buffer.data(), statement_buffer.size());
    auto create_archives_index
            = db.prepare_statement(statement_buffer.data(), statement_buffer.size());
    create_archives_index.step();
    statement_buffer.clear();

    fmt::format_to(
            statement_buffer_ix,
            "CREATE TABLE IF NOT EXISTS {} ({}) WITHOUT ROWID",
            streaming_archive::cMetadataDB::FilesTableName,
            get_field_names_and_types_sql(file_field_names_and_types)
    );
    SPDLOG_DEBUG("{:.{}}", statement_buffer.data(), statement_buffer.size());
    auto create_files_table
            = db.prepare_statement(statement_buffer.data(), statement_buffer.size());
    create_files_table.step();
    statement_buffer.clear();

    fmt::format_to(
            statement_buffer_ix,
            "CREATE INDEX IF NOT EXISTS files_path ON {} ({})",
            streaming_archive::cMetadataDB::FilesTableName,
            streaming_archive::cMetadataDB::File::Path
    );
    SPDLOG_DEBUG("{:.{}}", statement_buffer.data(), statement_buffer.size());
    auto create_files_path_index
            = db.prepare_statement(statement_buffer.data(), statement_buffer.size());
    create_files_path_index.step();
    statement_buffer.clear();

    fmt::format_to(
            statement_buffer_ix,
            "CREATE INDEX IF NOT EXISTS files_archive_id ON {} ({})",
            streaming_archive::cMetadataDB::FilesTableName,
            streaming_archive::cMetadataDB::File::ArchiveId
    );
    SPDLOG_DEBUG("{:.{}}", statement_buffer.data(), statement_buffer.size());
    auto create_files_archive_id_index
            = db.prepare_statement(statement_buffer.data(), statement_buffer.size());
    create_files_archive_id_index.step();
}

SQLitePreparedStatement get_archives_select_statement(SQLiteDB& db) {
    auto statement_string = fmt::format(
            "SELECT {} FROM {} ORDER BY {} ASC, {} ASC",
            streaming_archive::cMetadataDB::Archive::Id,
            streaming_archive::cMetadataDB::ArchivesTableName,
            streaming_archive::cMetadataDB::Archive::CreatorId,
            streaming_archive::cMetadataDB::Archive::CreationIx
    );
    SPDLOG_DEBUG("{}", statement_string);
    return db.prepare_statement(statement_string.c_str(), statement_string.length());
}

SQLitePreparedStatement get_archives_for_time_window_select_statement(
        SQLiteDB& db,
        epochtime_t begin_ts,
        epochtime_t end_ts
) {
    auto statement_string = fmt::format(
            "SELECT {} FROM {} WHERE {} <= ? AND {} >= ? ORDER BY {} ASC, {} ASC",
            streaming_archive::cMetadataDB::Archive::Id,
            streaming_archive::cMetadataDB::ArchivesTableName,
            streaming_archive::cMetadataDB::File::BeginTimestamp,
            streaming_archive::cMetadataDB::File::EndTimestamp,
            streaming_archive::cMetadataDB::Archive::CreatorId,
            streaming_archive::cMetadataDB::Archive::CreationIx
    );
    SPDLOG_DEBUG("{}", statement_string);
    auto statement = db.prepare_statement(statement_string.c_str(), statement_string.length());
    statement.bind_int64(1, end_ts);
    statement.bind_int64(2, begin_ts);

    return statement;
}

SQLitePreparedStatement
get_archives_for_file_select_statement(SQLiteDB& db, string const& file_path) {
    auto statement_string = fmt::format(
            "SELECT DISTINCT {}.{} FROM {} JOIN {} ON {}.{} = {}.{} WHERE {}.{} = ? ORDER BY {} "
            "ASC, {} ASC",
            streaming_archive::cMetadataDB::ArchivesTableName,
            streaming_archive::cMetadataDB::Archive::Id,
            streaming_archive::cMetadataDB::ArchivesTableName,
            streaming_archive::cMetadataDB::FilesTableName,
            streaming_archive::cMetadataDB::ArchivesTableName,
            streaming_archive::cMetadataDB::Archive::Id,
            streaming_archive::cMetadataDB::FilesTableName,
            streaming_archive::cMetadataDB::File::ArchiveId,
            streaming_archive::cMetadataDB::FilesTableName,
            streaming_archive::cMetadataDB::File::Path,
            streaming_archive::cMetadataDB::Archive::CreatorId,
            streaming_archive::cMetadataDB::Archive::CreationIx
    );
    SPDLOG_DEBUG("{}", statement_string);
    auto statement = db.prepare_statement(statement_string.c_str(), statement_string.length());
    statement.bind_text(1, file_path, true);

    return statement;
}

SQLitePreparedStatement
get_file_split_statement(SQLiteDB& db, string const& orig_file_id, size_t message_ix) {
    auto statement_string = fmt::format(
            "SELECT DISTINCT {}.{}, {}.{} FROM {} JOIN {} ON {}.{} = {}.{} "
            "WHERE {}.{} = ?1 AND ?2 >= {}.{} AND ?2 < ({}.{} + {}.{}) "
            "ORDER BY {} ASC, {} ASC",
            streaming_archive::cMetadataDB::ArchivesTableName,
            streaming_archive::cMetadataDB::Archive::Id,
            streaming_archive::cMetadataDB::FilesTableName,
            streaming_archive::cMetadataDB::File::Id,
            streaming_archive::cMetadataDB::ArchivesTableName,
            streaming_archive::cMetadataDB::FilesTableName,
            streaming_archive::cMetadataDB::ArchivesTableName,
            streaming_archive::cMetadataDB::Archive::Id,
            streaming_archive::cMetadataDB::FilesTableName,
            streaming_archive::cMetadataDB::File::ArchiveId,
            streaming_archive::cMetadataDB::FilesTableName,
            streaming_archive::cMetadataDB::File::OrigFileId,
            streaming_archive::cMetadataDB::FilesTableName,
            streaming_archive::cMetadataDB::File::BeginMessageIx,
            streaming_archive::cMetadataDB::FilesTableName,
            streaming_archive::cMetadataDB::File::NumMessages,
            streaming_archive::cMetadataDB::FilesTableName,
            streaming_archive::cMetadataDB::File::BeginMessageIx,
            streaming_archive::cMetadataDB::Archive::CreatorId,
            streaming_archive::cMetadataDB::Archive::CreationIx
    );
    SPDLOG_DEBUG("{}", statement_string);
    auto statement = db.prepare_statement(statement_string.c_str(), statement_string.length());
    statement.bind_text(1, orig_file_id, true);
    statement.bind_int64(2, static_cast<int64_t>(message_ix));

    return statement;
}
}  // namespace

GlobalSQLiteMetadataDB::ArchiveIterator::ArchiveIterator(SQLiteDB& db)
        : m_statement(get_archives_select_statement(db)) {
    m_statement.step();
}

GlobalSQLiteMetadataDB::ArchiveIterator::ArchiveIterator(
        SQLiteDB& db,
        epochtime_t begin_ts,
        epochtime_t end_ts
)
        : m_statement(get_archives_for_time_window_select_statement(db, begin_ts, end_ts)) {
    m_statement.step();
}

GlobalSQLiteMetadataDB::ArchiveIterator::ArchiveIterator(SQLiteDB& db, string const& file_path)
        : m_statement(get_archives_for_file_select_statement(db, file_path)) {
    m_statement.step();
}

bool GlobalSQLiteMetadataDB::ArchiveIterator::contains_element() const {
    return m_statement.is_row_ready();
}

void GlobalSQLiteMetadataDB::ArchiveIterator::get_next() {
    m_statement.step();
}

void GlobalSQLiteMetadataDB::ArchiveIterator::get_id(string& id) const {
    m_statement.column_string(0, id);
}

void GlobalSQLiteMetadataDB::open() {
    if (m_is_open) {
        throw OperationFailed(ErrorCode_NotReady, __FILENAME__, __LINE__);
    }

    m_db.open(m_path);

    vector<pair<string, string>> archive_field_names_and_types(
            enum_to_underlying_type(ArchivesTableFieldIndexes::Length)
    );
    archive_field_names_and_types[enum_to_underlying_type(ArchivesTableFieldIndexes::Id)].first
            = streaming_archive::cMetadataDB::Archive::Id;
    archive_field_names_and_types[enum_to_underlying_type(ArchivesTableFieldIndexes::Id)].second
            = "TEXT PRIMARY KEY";

    archive_field_names_and_types[enum_to_underlying_type(ArchivesTableFieldIndexes::BeginTimestamp
                                  )]
            .first
            = streaming_archive::cMetadataDB::Archive::BeginTimestamp;
    archive_field_names_and_types[enum_to_underlying_type(ArchivesTableFieldIndexes::BeginTimestamp
                                  )]
            .second
            = "INTEGER";

    archive_field_names_and_types[enum_to_underlying_type(ArchivesTableFieldIndexes::EndTimestamp)]
            .first
            = streaming_archive::cMetadataDB::Archive::EndTimestamp;
    archive_field_names_and_types[enum_to_underlying_type(ArchivesTableFieldIndexes::EndTimestamp)]
            .second
            = "INTEGER";

    archive_field_names_and_types
            [enum_to_underlying_type(ArchivesTableFieldIndexes::UncompressedSize)]
                    .first
            = streaming_archive::cMetadataDB::Archive::UncompressedSize;
    archive_field_names_and_types
            [enum_to_underlying_type(ArchivesTableFieldIndexes::UncompressedSize)]
                    .second
            = "INTEGER";

    archive_field_names_and_types[enum_to_underlying_type(ArchivesTableFieldIndexes::Size)].first
            = streaming_archive::cMetadataDB::Archive::Size;
    archive_field_names_and_types[enum_to_underlying_type(ArchivesTableFieldIndexes::Size)].second
            = "INTEGER";

    archive_field_names_and_types[enum_to_underlying_type(ArchivesTableFieldIndexes::CreatorId)]
            .first
            = streaming_archive::cMetadataDB::Archive::CreatorId;
    archive_field_names_and_types[enum_to_underlying_type(ArchivesTableFieldIndexes::CreatorId)]
            .second
            = "TEXT";

    archive_field_names_and_types[enum_to_underlying_type(ArchivesTableFieldIndexes::CreationIx)]
            .first
            = streaming_archive::cMetadataDB::Archive::CreationIx;
    archive_field_names_and_types[enum_to_underlying_type(ArchivesTableFieldIndexes::CreationIx)]
            .second
            = "INTEGER";

    vector<pair<string, string>> file_field_names_and_types(
            enum_to_underlying_type(FilesTableFieldIndexes::Length)
    );
    file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::Id)].first
            = streaming_archive::cMetadataDB::File::Id;
    file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::Id)].second
            = "TEXT PRIMARY KEY";

    file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::OrigFileId)].first
            = streaming_archive::cMetadataDB::File::OrigFileId;
    file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::OrigFileId)].second
            = "TEXT";

    file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::Path)].first
            = streaming_archive::cMetadataDB::File::Path;
    file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::Path)].second
            = "TEXT";

    file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::BeginTimestamp)]
            .first
            = streaming_archive::cMetadataDB::File::BeginTimestamp;
    file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::BeginTimestamp)]
            .second
            = "INTEGER";

    file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::EndTimestamp)].first
            = streaming_archive::cMetadataDB::File::EndTimestamp;
    file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::EndTimestamp)].second
            = "INTEGER";

    file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::NumUncompressedBytes
                               )]
            .first
            = streaming_archive::cMetadataDB::File::NumUncompressedBytes;
    file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::NumUncompressedBytes
                               )]
            .second
            = "INTEGER";

    file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::BeginMessageIx)]
            .first
            = streaming_archive::cMetadataDB::File::BeginMessageIx;
    file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::BeginMessageIx)]
            .second
            = "INTEGER";

    file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::NumMessages)].first
            = streaming_archive::cMetadataDB::File::NumMessages;
    file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::NumMessages)].second
            = "INTEGER";

    file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::ArchiveId)].first
            = streaming_archive::cMetadataDB::File::ArchiveId;
    file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::ArchiveId)].second
            = "TEXT";

    create_tables(archive_field_names_and_types, file_field_names_and_types, m_db);

    fmt::memory_buffer statement_buffer;
    auto statement_buffer_ix = std::back_inserter(statement_buffer);

    fmt::format_to(
            statement_buffer_ix,
            "INSERT INTO {} ({}) VALUES ({})",
            streaming_archive::cMetadataDB::ArchivesTableName,
            get_field_names_sql(archive_field_names_and_types),
            get_placeholders_sql(archive_field_names_and_types.size())
    );
    SPDLOG_DEBUG("{:.{}}", statement_buffer.data(), statement_buffer.size());
    m_insert_archive_statement = std::make_unique<SQLitePreparedStatement>(
            m_db.prepare_statement(statement_buffer.data(), statement_buffer.size())
    );
    statement_buffer.clear();

    vector<string> update_archive_size_stmt_field_names(
            enum_to_underlying_type(UpdateArchiveSizeStmtFieldIndexes::Length)
    );
    update_archive_size_stmt_field_names[enum_to_underlying_type(
            UpdateArchiveSizeStmtFieldIndexes::BeginTimestamp
    )] = streaming_archive::cMetadataDB::Archive::BeginTimestamp;
    update_archive_size_stmt_field_names[enum_to_underlying_type(
            UpdateArchiveSizeStmtFieldIndexes::EndTimestamp
    )] = streaming_archive::cMetadataDB::Archive::EndTimestamp;
    update_archive_size_stmt_field_names[enum_to_underlying_type(
            UpdateArchiveSizeStmtFieldIndexes::UncompressedSize
    )] = streaming_archive::cMetadataDB::Archive::UncompressedSize;
    update_archive_size_stmt_field_names[enum_to_underlying_type(
            UpdateArchiveSizeStmtFieldIndexes::Size
    )] = streaming_archive::cMetadataDB::Archive::Size;

    fmt::format_to(
            statement_buffer_ix,
            "UPDATE {} SET {} WHERE {} = ?{}",
            streaming_archive::cMetadataDB::ArchivesTableName,
            get_numbered_set_field_sql(update_archive_size_stmt_field_names, 0),
            streaming_archive::cMetadataDB::Archive::Id,
            enum_to_underlying_type(UpdateArchiveSizeStmtFieldIndexes::Length) + 1
    );
    SPDLOG_DEBUG("{:.{}}", statement_buffer.data(), statement_buffer.size());
    m_update_archive_size_statement = std::make_unique<SQLitePreparedStatement>(
            m_db.prepare_statement(statement_buffer.data(), statement_buffer.size())
    );
    statement_buffer.clear();

    // Insert or on conflict, set all fields except the ID
    fmt::format_to(
            statement_buffer_ix,
            "INSERT INTO {} ({}) VALUES ({}) ON CONFLICT ({}) DO UPDATE SET {}",
            streaming_archive::cMetadataDB::FilesTableName,
            get_field_names_sql(file_field_names_and_types),
            get_numbered_placeholders_sql(file_field_names_and_types.size()),
            streaming_archive::cMetadataDB::File::Id,
            get_numbered_set_field_sql(
                    file_field_names_and_types,
                    enum_to_underlying_type(FilesTableFieldIndexes::Id) + 1
            )
    );
    SPDLOG_DEBUG("{:.{}}", statement_buffer.data(), statement_buffer.size());
    m_upsert_file_statement = std::make_unique<SQLitePreparedStatement>(
            m_db.prepare_statement(statement_buffer.data(), statement_buffer.size())
    );

    m_upsert_files_transaction_begin_statement
            = std::make_unique<SQLitePreparedStatement>(m_db.prepare_statement("BEGIN TRANSACTION")
            );
    m_upsert_files_transaction_end_statement
            = std::make_unique<SQLitePreparedStatement>(m_db.prepare_statement("END TRANSACTION"));

    m_is_open = true;
}

void GlobalSQLiteMetadataDB::close() {
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

void GlobalSQLiteMetadataDB::add_archive(
        string const& id,
        streaming_archive::ArchiveMetadata const& metadata
) {
    if (false == m_is_open) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    m_insert_archive_statement
            ->bind_text(enum_to_underlying_type(ArchivesTableFieldIndexes::Id) + 1, id, false);
    m_insert_archive_statement->bind_int64(
            enum_to_underlying_type(ArchivesTableFieldIndexes::BeginTimestamp) + 1,
            (int64_t)metadata.get_begin_timestamp()
    );
    m_insert_archive_statement->bind_int64(
            enum_to_underlying_type(ArchivesTableFieldIndexes::EndTimestamp) + 1,
            (int64_t)metadata.get_end_timestamp()
    );
    m_insert_archive_statement->bind_int64(
            enum_to_underlying_type(ArchivesTableFieldIndexes::UncompressedSize) + 1,
            (int64_t)metadata.get_uncompressed_size_bytes()
    );
    m_insert_archive_statement->bind_int64(
            enum_to_underlying_type(ArchivesTableFieldIndexes::Size) + 1,
            (int64_t)metadata.get_compressed_size_bytes()
    );
    m_insert_archive_statement->bind_text(
            enum_to_underlying_type(ArchivesTableFieldIndexes::CreatorId) + 1,
            metadata.get_creator_id(),
            false
    );
    m_insert_archive_statement->bind_int64(
            enum_to_underlying_type(ArchivesTableFieldIndexes::CreationIx) + 1,
            (int64_t)metadata.get_creation_idx()
    );
    m_insert_archive_statement->step();
    m_insert_archive_statement->reset();
}

void GlobalSQLiteMetadataDB::update_archive_metadata(
        string const& archive_id,
        streaming_archive::ArchiveMetadata const& metadata
) {
    if (false == m_is_open) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    m_update_archive_size_statement->bind_int64(
            enum_to_underlying_type(UpdateArchiveSizeStmtFieldIndexes::BeginTimestamp) + 1,
            (int64_t)metadata.get_begin_timestamp()
    );
    m_update_archive_size_statement->bind_int64(
            enum_to_underlying_type(UpdateArchiveSizeStmtFieldIndexes::EndTimestamp) + 1,
            (int64_t)metadata.get_end_timestamp()
    );
    m_update_archive_size_statement->bind_int64(
            enum_to_underlying_type(UpdateArchiveSizeStmtFieldIndexes::UncompressedSize) + 1,
            (int64_t)metadata.get_uncompressed_size_bytes()
    );
    m_update_archive_size_statement->bind_int64(
            enum_to_underlying_type(UpdateArchiveSizeStmtFieldIndexes::Size) + 1,
            (int64_t)metadata.get_compressed_size_bytes()
    );
    m_update_archive_size_statement->bind_text(
            enum_to_underlying_type(UpdateArchiveSizeStmtFieldIndexes::Length) + 1,
            archive_id,
            false
    );
    m_update_archive_size_statement->step();
    m_update_archive_size_statement->reset();
}

void GlobalSQLiteMetadataDB::update_metadata_for_files(
        string const& archive_id,
        vector<streaming_archive::writer::File*> const& files
) {
    if (false == m_is_open) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    m_upsert_files_transaction_begin_statement->step();
    for (auto file : files) {
        auto const id_as_string = file->get_id_as_string();
        auto const orig_file_id_as_string = file->get_orig_file_id_as_string();
        m_upsert_file_statement->bind_text(
                enum_to_underlying_type(FilesTableFieldIndexes::Id) + 1,
                id_as_string,
                false
        );
        m_upsert_file_statement->bind_text(
                enum_to_underlying_type(FilesTableFieldIndexes::OrigFileId) + 1,
                orig_file_id_as_string,
                false
        );
        m_upsert_file_statement->bind_text(
                enum_to_underlying_type(FilesTableFieldIndexes::Path) + 1,
                file->get_orig_path(),
                false
        );
        m_upsert_file_statement->bind_int64(
                enum_to_underlying_type(FilesTableFieldIndexes::BeginTimestamp) + 1,
                file->get_begin_ts()
        );
        m_upsert_file_statement->bind_int64(
                enum_to_underlying_type(FilesTableFieldIndexes::EndTimestamp) + 1,
                file->get_end_ts()
        );
        m_upsert_file_statement->bind_int64(
                enum_to_underlying_type(FilesTableFieldIndexes::NumUncompressedBytes) + 1,
                (int64_t)file->get_num_uncompressed_bytes()
        );
        m_upsert_file_statement->bind_int64(
                enum_to_underlying_type(FilesTableFieldIndexes::BeginMessageIx) + 1,
                (int64_t)file->get_begin_message_ix()
        );
        m_upsert_file_statement->bind_int64(
                enum_to_underlying_type(FilesTableFieldIndexes::NumMessages) + 1,
                (int64_t)file->get_num_messages()
        );
        m_upsert_file_statement->bind_text(
                enum_to_underlying_type(FilesTableFieldIndexes::ArchiveId) + 1,
                archive_id,
                false
        );

        m_upsert_file_statement->step();
        m_upsert_file_statement->reset();
    }
    m_upsert_files_transaction_end_statement->step();

    m_upsert_files_transaction_begin_statement->reset();
    m_upsert_files_transaction_end_statement->reset();
}

bool GlobalSQLiteMetadataDB::get_file_split(
        string const& orig_file_id,
        size_t msg_ix,
        string& archive_id,
        string& file_split_id
) {
    auto statement = get_file_split_statement(m_db, orig_file_id, msg_ix);
    statement.step();
    if (false == statement.is_row_ready()) {
        return false;
    }

    statement.column_string(0, archive_id);
    statement.column_string(1, file_split_id);

    return true;
}

}  // namespace clp

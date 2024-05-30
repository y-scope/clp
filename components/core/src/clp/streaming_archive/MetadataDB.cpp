#include "MetadataDB.hpp"

#include <vector>

#include <fmt/core.h>

#include "../database_utils.hpp"
#include "../Defs.h"
#include "../type_utils.hpp"
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
    BeginMessageIx,
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

namespace clp::streaming_archive {
static void
create_tables(vector<std::pair<string, string>> const& file_field_names_and_types, SQLiteDB& db) {
    fmt::memory_buffer statement_buffer;
    auto statement_buffer_ix = std::back_inserter(statement_buffer);
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
            "CREATE INDEX IF NOT EXISTS files_segment_order ON {} ({},{})",
            streaming_archive::cMetadataDB::FilesTableName,
            streaming_archive::cMetadataDB::File::SegmentId,
            streaming_archive::cMetadataDB::File::SegmentTimestampsPosition
    );
    SPDLOG_DEBUG("{:.{}}", statement_buffer.data(), statement_buffer.size());
    auto create_index_statement
            = db.prepare_statement(statement_buffer.data(), statement_buffer.size());
    create_index_statement.step();
    statement_buffer.clear();

    fmt::format_to(
            statement_buffer_ix,
            "CREATE INDEX IF NOT EXISTS files_begin_timestamp ON {} ({})",
            streaming_archive::cMetadataDB::FilesTableName,
            streaming_archive::cMetadataDB::File::BeginTimestamp
    );
    SPDLOG_DEBUG("{:.{}}", statement_buffer.data(), statement_buffer.size());
    create_index_statement = db.prepare_statement(statement_buffer.data(), statement_buffer.size());
    create_index_statement.step();
    statement_buffer.clear();

    fmt::format_to(
            statement_buffer_ix,
            "CREATE INDEX IF NOT EXISTS files_end_timestamp ON {} ({})",
            streaming_archive::cMetadataDB::FilesTableName,
            streaming_archive::cMetadataDB::File::EndTimestamp
    );
    SPDLOG_DEBUG("{:.{}}", statement_buffer.data(), statement_buffer.size());
    create_index_statement = db.prepare_statement(statement_buffer.data(), statement_buffer.size());
    create_index_statement.step();
    statement_buffer.clear();

    fmt::format_to(
            statement_buffer_ix,
            "CREATE INDEX IF NOT EXISTS files_path ON {} ({})",
            streaming_archive::cMetadataDB::FilesTableName,
            streaming_archive::cMetadataDB::File::Path
    );
    SPDLOG_DEBUG("{:.{}}", statement_buffer.data(), statement_buffer.size());
    create_index_statement = db.prepare_statement(statement_buffer.data(), statement_buffer.size());
    create_index_statement.step();
    statement_buffer.clear();

    fmt::format_to(
            statement_buffer_ix,
            "CREATE INDEX IF NOT EXISTS files_segment_id ON {} ({})",
            streaming_archive::cMetadataDB::FilesTableName,
            streaming_archive::cMetadataDB::File::SegmentId
    );
    SPDLOG_DEBUG("{:.{}}", statement_buffer.data(), statement_buffer.size());
    create_index_statement = db.prepare_statement(statement_buffer.data(), statement_buffer.size());
    create_index_statement.step();
    statement_buffer.clear();

    fmt::format_to(
            statement_buffer_ix,
            "CREATE TABLE IF NOT EXISTS {} ({} TEXT PRIMARY KEY) WITHOUT ROWID",
            streaming_archive::cMetadataDB::EmptyDirectoriesTableName,
            streaming_archive::cMetadataDB::EmptyDirectory::Path
    );
    SPDLOG_DEBUG("{:.{}}", statement_buffer.data(), statement_buffer.size());
    auto create_empty_directories_table
            = db.prepare_statement(statement_buffer.data(), statement_buffer.size());
    create_empty_directories_table.step();
}

MetadataDB::Iterator::Iterator(SQLitePreparedStatement statement)
        : m_statement(std::move(statement)) {
    m_statement.step();
}

void MetadataDB::Iterator::reset() {
    m_statement.reset();
    m_statement.step();
}

static SQLitePreparedStatement get_files_select_statement(
        SQLiteDB& db,
        epochtime_t ts_begin,
        epochtime_t ts_end,
        string const& file_path,
        string const& file_split_id,
        bool in_specific_segment,
        segment_id_t segment_id,
        bool order_by_segment_end_ts
) {
    vector<string> field_names(enum_to_underlying_type(FilesTableFieldIndexes::Length));
    field_names[enum_to_underlying_type(FilesTableFieldIndexes::Id)]
            = streaming_archive::cMetadataDB::File::Id;
    field_names[enum_to_underlying_type(FilesTableFieldIndexes::OrigFileId)]
            = streaming_archive::cMetadataDB::File::OrigFileId;
    field_names[enum_to_underlying_type(FilesTableFieldIndexes::Path)]
            = streaming_archive::cMetadataDB::File::Path;
    field_names[enum_to_underlying_type(FilesTableFieldIndexes::BeginTimestamp)]
            = streaming_archive::cMetadataDB::File::BeginTimestamp;
    field_names[enum_to_underlying_type(FilesTableFieldIndexes::EndTimestamp)]
            = streaming_archive::cMetadataDB::File::EndTimestamp;
    field_names[enum_to_underlying_type(FilesTableFieldIndexes::TimestampPatterns)]
            = streaming_archive::cMetadataDB::File::TimestampPatterns;
    field_names[enum_to_underlying_type(FilesTableFieldIndexes::NumUncompressedBytes)]
            = streaming_archive::cMetadataDB::File::NumUncompressedBytes;
    field_names[enum_to_underlying_type(FilesTableFieldIndexes::BeginMessageIx)]
            = streaming_archive::cMetadataDB::File::BeginMessageIx;
    field_names[enum_to_underlying_type(FilesTableFieldIndexes::NumMessages)]
            = streaming_archive::cMetadataDB::File::NumMessages;
    field_names[enum_to_underlying_type(FilesTableFieldIndexes::NumVariables)]
            = streaming_archive::cMetadataDB::File::NumVariables;
    field_names[enum_to_underlying_type(FilesTableFieldIndexes::IsSplit)]
            = streaming_archive::cMetadataDB::File::IsSplit;
    field_names[enum_to_underlying_type(FilesTableFieldIndexes::SplitIx)]
            = streaming_archive::cMetadataDB::File::SplitIx;
    field_names[enum_to_underlying_type(FilesTableFieldIndexes::SegmentId)]
            = streaming_archive::cMetadataDB::File::SegmentId;
    field_names[enum_to_underlying_type(FilesTableFieldIndexes::SegmentTimestampsPosition)]
            = streaming_archive::cMetadataDB::File::SegmentTimestampsPosition;
    field_names[enum_to_underlying_type(FilesTableFieldIndexes::SegmentLogtypesPosition)]
            = streaming_archive::cMetadataDB::File::SegmentLogtypesPosition;
    field_names[enum_to_underlying_type(FilesTableFieldIndexes::SegmentVariablesPosition)]
            = streaming_archive::cMetadataDB::File::SegmentVariablesPosition;

    fmt::memory_buffer statement_buffer;
    auto statement_buffer_ix = std::back_inserter(statement_buffer);

    fmt::format_to(
            statement_buffer_ix,
            "SELECT {} FROM {}",
            get_field_names_sql(field_names),
            streaming_archive::cMetadataDB::FilesTableName
    );

    // Add clauses
    bool clause_exists = false;
    if (cEpochTimeMin != ts_begin) {
        // If the end-timestamp of the file is less than the given begin-timestamp, messages within
        // the file are guaranteed to be outside the timestamp range. So this filters for the
        // opposite.
        fmt::format_to(
                statement_buffer_ix,
                " WHERE {} >= ?{}",
                streaming_archive::cMetadataDB::File::EndTimestamp,
                enum_to_underlying_type(FilesTableFieldIndexes::BeginTimestamp) + 1
        );
        clause_exists = true;
    }
    if (cEpochTimeMax != ts_end) {
        // If the begin-timestamp of the file is greater than the given end-timestamp, messages
        // within the file are guaranteed to be outside the timestamp range. So this filters for the
        // opposite.
        fmt::format_to(
                statement_buffer_ix,
                " {} {} <= ?{}",
                clause_exists ? "AND" : "WHERE",
                streaming_archive::cMetadataDB::File::BeginTimestamp,
                enum_to_underlying_type(FilesTableFieldIndexes::EndTimestamp) + 1
        );
        clause_exists = true;
    }
    if (false == file_path.empty()) {
        fmt::format_to(
                statement_buffer_ix,
                " {} {} = ?{}",
                clause_exists ? "AND" : "WHERE",
                streaming_archive::cMetadataDB::File::Path,
                enum_to_underlying_type(FilesTableFieldIndexes::Path) + 1
        );
        clause_exists = true;
    }
    if (false == file_split_id.empty()) {
        fmt::format_to(
                statement_buffer_ix,
                " {} {} = ?{}",
                clause_exists ? "AND" : "WHERE",
                streaming_archive::cMetadataDB::File::Id,
                enum_to_underlying_type(FilesTableFieldIndexes::Id) + 1
        );
        clause_exists = true;
    }
    if (in_specific_segment) {
        fmt::format_to(
                statement_buffer_ix,
                " {} {} = ?{}",
                clause_exists ? "AND" : "WHERE",
                streaming_archive::cMetadataDB::File::SegmentId,
                enum_to_underlying_type(FilesTableFieldIndexes::SegmentId) + 1
        );
        clause_exists = true;
    }

    // Add ordering
    if (order_by_segment_end_ts) {
        fmt::format_to(
                statement_buffer_ix,
                " ORDER BY MAX({}) OVER (PARTITION by {}) DESC, {} ASC",
                streaming_archive::cMetadataDB::File::EndTimestamp,
                streaming_archive::cMetadataDB::File::SegmentId,
                streaming_archive::cMetadataDB::File::SegmentTimestampsPosition
        );
    } else {
        fmt::format_to(
                statement_buffer_ix,
                " ORDER BY {} ASC, {} ASC",
                streaming_archive::cMetadataDB::File::SegmentId,
                streaming_archive::cMetadataDB::File::SegmentTimestampsPosition
        );
    }

    auto statement = db.prepare_statement(statement_buffer.data(), statement_buffer.size());
    if (cEpochTimeMin != ts_begin) {
        statement.bind_int64(
                enum_to_underlying_type(FilesTableFieldIndexes::BeginTimestamp) + 1,
                ts_begin
        );
    }
    if (cEpochTimeMax != ts_end) {
        statement.bind_int64(
                enum_to_underlying_type(FilesTableFieldIndexes::EndTimestamp) + 1,
                ts_end
        );
    }
    if (false == file_path.empty()) {
        statement.bind_text(
                enum_to_underlying_type(FilesTableFieldIndexes::Path) + 1,
                file_path,
                true
        );
    }
    if (false == file_split_id.empty()) {
        statement.bind_text(
                enum_to_underlying_type(FilesTableFieldIndexes::Id) + 1,
                file_split_id,
                true
        );
    }
    if (in_specific_segment) {
        statement.bind_int64(
                enum_to_underlying_type(FilesTableFieldIndexes::SegmentId) + 1,
                (int64_t)segment_id
        );
    }

    SPDLOG_DEBUG("{:.{}}", statement_buffer.data(), statement_buffer.size());

    return statement;
}

static SQLitePreparedStatement get_empty_directories_select_statement(SQLiteDB& db) {
    fmt::memory_buffer statement_buffer;
    auto statement_buffer_ix = std::back_inserter(statement_buffer);

    fmt::format_to(
            statement_buffer_ix,
            "SELECT {} FROM {}",
            streaming_archive::cMetadataDB::EmptyDirectory::Path,
            streaming_archive::cMetadataDB::EmptyDirectoriesTableName
    );
    SPDLOG_DEBUG("{:.{}}", statement_buffer.data(), statement_buffer.size());
    return db.prepare_statement(statement_buffer.data(), statement_buffer.size());
}

MetadataDB::FileIterator::FileIterator(
        SQLiteDB& db,
        epochtime_t begin_timestamp,
        epochtime_t end_timestamp,
        string const& file_path,
        string const& file_split_id,
        bool in_specific_segment,
        segment_id_t segment_id,
        bool order_by_segment_end_ts
)
        : Iterator(get_files_select_statement(
                  db,
                  begin_timestamp,
                  end_timestamp,
                  file_path,
                  file_split_id,
                  in_specific_segment,
                  segment_id,
                  order_by_segment_end_ts
          )) {}

MetadataDB::EmptyDirectoryIterator::EmptyDirectoryIterator(SQLiteDB& db)
        : Iterator(get_empty_directories_select_statement(db)) {}

void MetadataDB::FileIterator::set_segment_id(segment_id_t segment_id) {
    m_statement.reset();

    m_statement.bind_int64(
            enum_to_underlying_type(FilesTableFieldIndexes::SegmentId) + 1,
            (int64_t)segment_id
    );

    m_statement.step();
}

void MetadataDB::FileIterator::get_id(string& id) const {
    m_statement.column_string(enum_to_underlying_type(FilesTableFieldIndexes::Id), id);
}

void MetadataDB::FileIterator::get_orig_file_id(string& id) const {
    m_statement.column_string(enum_to_underlying_type(FilesTableFieldIndexes::OrigFileId), id);
}

void MetadataDB::FileIterator::get_path(string& path) const {
    m_statement.column_string(enum_to_underlying_type(FilesTableFieldIndexes::Path), path);
}

epochtime_t MetadataDB::FileIterator::get_begin_ts() const {
    return m_statement.column_int64(enum_to_underlying_type(FilesTableFieldIndexes::BeginTimestamp)
    );
}

epochtime_t MetadataDB::FileIterator::get_end_ts() const {
    return m_statement.column_int64(enum_to_underlying_type(FilesTableFieldIndexes::EndTimestamp));
}

void MetadataDB::FileIterator::get_timestamp_patterns(string& timestamp_patterns) const {
    m_statement.column_string(
            enum_to_underlying_type(FilesTableFieldIndexes::TimestampPatterns),
            timestamp_patterns
    );
}

size_t MetadataDB::FileIterator::get_num_uncompressed_bytes() const {
    return m_statement.column_int64(
            enum_to_underlying_type(FilesTableFieldIndexes::NumUncompressedBytes)
    );
}

size_t MetadataDB::FileIterator::get_begin_message_ix() const {
    return m_statement.column_int64(enum_to_underlying_type(FilesTableFieldIndexes::BeginMessageIx)
    );
}

size_t MetadataDB::FileIterator::get_num_messages() const {
    return m_statement.column_int64(enum_to_underlying_type(FilesTableFieldIndexes::NumMessages));
}

size_t MetadataDB::FileIterator::get_num_variables() const {
    return m_statement.column_int64(enum_to_underlying_type(FilesTableFieldIndexes::NumVariables));
}

bool MetadataDB::FileIterator::is_split() const {
    return m_statement.column_int(enum_to_underlying_type(FilesTableFieldIndexes::IsSplit));
}

size_t MetadataDB::FileIterator::get_split_ix() const {
    return m_statement.column_int64(enum_to_underlying_type(FilesTableFieldIndexes::SplitIx));
}

segment_id_t MetadataDB::FileIterator::get_segment_id() const {
    return m_statement.column_int64(enum_to_underlying_type(FilesTableFieldIndexes::SegmentId));
}

size_t MetadataDB::FileIterator::get_segment_timestamps_pos() const {
    return m_statement.column_int64(
            enum_to_underlying_type(FilesTableFieldIndexes::SegmentTimestampsPosition)
    );
}

size_t MetadataDB::FileIterator::get_segment_logtypes_pos() const {
    return m_statement.column_int64(
            enum_to_underlying_type(FilesTableFieldIndexes::SegmentLogtypesPosition)
    );
}

size_t MetadataDB::FileIterator::get_segment_variables_pos() const {
    return m_statement.column_int64(
            enum_to_underlying_type(FilesTableFieldIndexes::SegmentVariablesPosition)
    );
}

void MetadataDB::open(string const& path) {
    if (m_is_open) {
        throw OperationFailed(ErrorCode_NotReady, __FILENAME__, __LINE__);
    }

    m_db.open(path);

    vector<std::pair<string, string>> file_field_names_and_types(
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

    file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::TimestampPatterns)]
            .first
            = streaming_archive::cMetadataDB::File::TimestampPatterns;
    file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::TimestampPatterns)]
            .second
            = "TEXT";

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

    file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::NumVariables)].first
            = streaming_archive::cMetadataDB::File::NumVariables;
    file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::NumVariables)].second
            = "INTEGER";

    file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::IsSplit)].first
            = streaming_archive::cMetadataDB::File::IsSplit;
    file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::IsSplit)].second
            = "INTEGER";

    file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::SplitIx)].first
            = streaming_archive::cMetadataDB::File::SplitIx;
    file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::SplitIx)].second
            = "INTEGER";

    file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::SegmentId)].first
            = streaming_archive::cMetadataDB::File::SegmentId;
    file_field_names_and_types[enum_to_underlying_type(FilesTableFieldIndexes::SegmentId)].second
            = "INTEGER";

    file_field_names_and_types
            [enum_to_underlying_type(FilesTableFieldIndexes::SegmentTimestampsPosition)]
                    .first
            = streaming_archive::cMetadataDB::File::SegmentTimestampsPosition;
    file_field_names_and_types
            [enum_to_underlying_type(FilesTableFieldIndexes::SegmentTimestampsPosition)]
                    .second
            = "INTEGER";

    file_field_names_and_types
            [enum_to_underlying_type(FilesTableFieldIndexes::SegmentLogtypesPosition)]
                    .first
            = streaming_archive::cMetadataDB::File::SegmentLogtypesPosition;
    file_field_names_and_types
            [enum_to_underlying_type(FilesTableFieldIndexes::SegmentLogtypesPosition)]
                    .second
            = "INTEGER";

    file_field_names_and_types
            [enum_to_underlying_type(FilesTableFieldIndexes::SegmentVariablesPosition)]
                    .first
            = streaming_archive::cMetadataDB::File::SegmentVariablesPosition;
    file_field_names_and_types
            [enum_to_underlying_type(FilesTableFieldIndexes::SegmentVariablesPosition)]
                    .second
            = "INTEGER";

    create_tables(file_field_names_and_types, m_db);

    fmt::memory_buffer statement_buffer;
    auto statement_buffer_ix = std::back_inserter(statement_buffer);

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
    m_upsert_file_statement = make_unique<SQLitePreparedStatement>(
            m_db.prepare_statement(statement_buffer.data(), statement_buffer.size())
    );
    statement_buffer.clear();

    m_transaction_begin_statement
            = make_unique<SQLitePreparedStatement>(m_db.prepare_statement("BEGIN TRANSACTION"));
    m_transaction_end_statement
            = make_unique<SQLitePreparedStatement>(m_db.prepare_statement("END TRANSACTION"));

    fmt::format_to(
            statement_buffer_ix,
            "INSERT INTO {} ({}) VALUES (?) ON CONFLICT DO NOTHING",
            streaming_archive::cMetadataDB::EmptyDirectoriesTableName,
            streaming_archive::cMetadataDB::EmptyDirectory::Path
    );
    SPDLOG_DEBUG("{:.{}}", statement_buffer.data(), statement_buffer.size());
    m_insert_empty_directories_statement = make_unique<SQLitePreparedStatement>(
            m_db.prepare_statement(statement_buffer.data(), statement_buffer.size())
    );
    m_is_open = true;
}

void MetadataDB::close() {
    m_transaction_begin_statement.reset(nullptr);
    m_transaction_end_statement.reset(nullptr);
    m_upsert_file_statement.reset(nullptr);
    m_insert_empty_directories_statement.reset(nullptr);
    if (false == m_db.close()) {
        SPDLOG_ERROR(
                "streaming_archive::MetadataDB: Failed to close database - {}",
                m_db.get_error_message()
        );
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }
    m_is_open = false;
}

void MetadataDB::update_files(vector<writer::File*> const& files) {
    m_transaction_begin_statement->step();
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
        m_upsert_file_statement->bind_text(
                enum_to_underlying_type(FilesTableFieldIndexes::TimestampPatterns) + 1,
                file->get_encoded_timestamp_patterns(),
                true
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
        m_upsert_file_statement->bind_int64(
                enum_to_underlying_type(FilesTableFieldIndexes::NumVariables) + 1,
                (int64_t)file->get_num_variables()
        );
        m_upsert_file_statement->bind_int64(
                enum_to_underlying_type(FilesTableFieldIndexes::IsSplit) + 1,
                (int64_t)file->is_split()
        );
        m_upsert_file_statement->bind_int64(
                enum_to_underlying_type(FilesTableFieldIndexes::SplitIx) + 1,
                (int64_t)file->get_split_ix()
        );
        m_upsert_file_statement->bind_int64(
                enum_to_underlying_type(FilesTableFieldIndexes::SegmentId) + 1,
                (int64_t)file->get_segment_id()
        );
        m_upsert_file_statement->bind_int64(
                enum_to_underlying_type(FilesTableFieldIndexes::SegmentTimestampsPosition) + 1,
                (int64_t)file->get_segment_timestamps_pos()
        );
        m_upsert_file_statement->bind_int64(
                enum_to_underlying_type(FilesTableFieldIndexes::SegmentLogtypesPosition) + 1,
                (int64_t)file->get_segment_logtypes_pos()
        );
        m_upsert_file_statement->bind_int64(
                enum_to_underlying_type(FilesTableFieldIndexes::SegmentVariablesPosition) + 1,
                (int64_t)file->get_segment_variables_pos()
        );

        m_upsert_file_statement->step();
        m_upsert_file_statement->reset();
    }
    m_transaction_end_statement->step();

    m_transaction_begin_statement->reset();
    m_transaction_end_statement->reset();
}

void MetadataDB::add_empty_directories(vector<string> const& empty_directory_paths) {
    for (auto const& path : empty_directory_paths) {
        m_insert_empty_directories_statement->bind_text(1, path, false);
        m_insert_empty_directories_statement->step();
        m_insert_empty_directories_statement->reset();
    }
}
}  // namespace clp::streaming_archive

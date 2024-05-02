#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <Catch2/single_include/catch2/catch.hpp>
#include <fmt/core.h>
#include <fmt/format.h>

#include "../src/clp/database_utils.hpp"
#include "../src/clp/Defs.h"
#include "../src/clp/SQLiteDB.hpp"
#include "../src/clp/SQLitePreparedStatement.hpp"
#include "../src/clp/streaming_archive/Constants.hpp"

using clp::epochtime_t;
using clp::SQLiteDB;
using clp::SQLitePreparedStatement;

namespace {
class TestTable {
public:
    TestTable() {
        auto add_column = [&](std::string_view column_name, std::string type) -> void {
            m_columns.emplace_back(column_name);
            m_column_types.emplace_back(column_name, type);
            m_column_idx_map.emplace(column_name, static_cast<int>(m_column_idx_map.size() + 1));
        };

        add_column(clp::streaming_archive::cMetadataDB::File::Path, "TEXT PRIMARY KEY");
        add_column(clp::streaming_archive::cMetadataDB::File::BeginTimestamp, "INTEGER");
        add_column(clp::streaming_archive::cMetadataDB::File::EndTimestamp, "INTEGER");
        add_column(clp::streaming_archive::cMetadataDB::File::SegmentId, "INTEGER");
        add_column(clp::streaming_archive::cMetadataDB::File::SegmentTimestampsPosition, "INTEGER");
    }

    [[nodiscard]] auto get_column_idx(std::string const& column_name) const -> int {
        auto const it{m_column_idx_map.find(column_name)};
        REQUIRE(m_column_idx_map.cend() != it);
        return it->second;
    }

    [[nodiscard]] auto get_columns() const -> std::vector<std::string> const& { return m_columns; }

    [[nodiscard]] auto get_column_types(
    ) const -> std::vector<std::pair<std::string, std::string>> const& {
        return m_column_types;
    }

    [[nodiscard]] auto get_column_idx_map() const -> std::unordered_map<std::string, int> const& {
        return m_column_idx_map;
    }

    [[nodiscard]] auto get_name() const -> std::string_view { return m_name; }

private:
    std::string m_name{"CLP_TEST_TABLE"};
    std::vector<std::string> m_columns;
    std::vector<std::pair<std::string, std::string>> m_column_types;
    std::unordered_map<std::string, int> m_column_idx_map;
};

class TestRow {
public:
    TestRow(std::string path,
            epochtime_t begin_ts,
            epochtime_t end_ts,
            size_t segment_id,
            size_t segment_ts_pos)
            : m_path{std::move(path)},
              m_begin_ts{begin_ts},
              m_end_ts{end_ts},
              m_segment_id{segment_id},
              m_segment_ts_pos{segment_ts_pos} {}

    [[nodiscard]] auto get_path() const -> std::string_view { return m_path; }

    [[nodiscard]] auto get_begin_ts() const -> epochtime_t { return m_begin_ts; }

    [[nodiscard]] auto get_end_ts() const -> epochtime_t { return m_end_ts; }

    [[nodiscard]] auto get_segment_id() const -> size_t { return m_segment_id; }

    [[nodiscard]] auto get_segment_ts_pos() const -> size_t { return m_segment_ts_pos; }

    auto
    insert_to_table(SQLitePreparedStatement& insert_stmt, TestTable const& table) const -> void {
        insert_stmt.bind_text(
                table.get_column_idx(clp::streaming_archive::cMetadataDB::File::Path),
                m_path,
                false
        );
        insert_stmt.bind_int64(
                table.get_column_idx(clp::streaming_archive::cMetadataDB::File::BeginTimestamp),
                m_begin_ts
        );
        insert_stmt.bind_int64(
                table.get_column_idx(clp::streaming_archive::cMetadataDB::File::EndTimestamp),
                m_end_ts
        );
        insert_stmt.bind_int64(
                table.get_column_idx(clp::streaming_archive::cMetadataDB::File::SegmentId),
                m_segment_id
        );
        insert_stmt.bind_int64(
                table.get_column_idx(
                        clp::streaming_archive::cMetadataDB::File::SegmentTimestampsPosition
                ),
                m_segment_ts_pos
        );
        insert_stmt.step();
        insert_stmt.reset();
    }

private:
    std::string m_path;
    epochtime_t m_begin_ts;
    epochtime_t m_end_ts;
    size_t m_segment_id;
    size_t m_segment_ts_pos;
};

[[nodiscard]] auto get_test_table() -> TestTable const& {
    static std::unique_ptr<TestTable> table_ptr{nullptr};
    if (nullptr == table_ptr) {
        table_ptr = std::make_unique<TestTable>();
    }
    return *table_ptr;
}

auto create_table_and_indices(SQLiteDB& db) -> void {
    fmt::memory_buffer statement_buffer;
    auto statement_buffer_ix{std::back_inserter(statement_buffer)};
    auto const& test_table{get_test_table()};

    fmt::format_to(
            statement_buffer_ix,
            "CREATE TABLE IF NOT EXISTS {} ({}) WITHOUT ROWID",
            test_table.get_name(),
            clp::get_field_names_and_types_sql(test_table.get_column_types())
    );
    auto create_table_stmt{db.prepare_statement(statement_buffer.data(), statement_buffer.size())};
    create_table_stmt.step();
    statement_buffer.clear();

    fmt::format_to(
            statement_buffer_ix,
            "CREATE INDEX IF NOT EXISTS files_begin_timestamp ON {} ({})",
            test_table.get_name(),
            clp::streaming_archive::cMetadataDB::File::BeginTimestamp
    );
    auto create_begin_ts_idx_stmt{
            db.prepare_statement(statement_buffer.data(), statement_buffer.size())
    };
    create_begin_ts_idx_stmt.step();
    statement_buffer.clear();

    fmt::format_to(
            statement_buffer_ix,
            "CREATE INDEX IF NOT EXISTS files_begin_timestamp ON {} ({})",
            test_table.get_name(),
            clp::streaming_archive::cMetadataDB::File::BeginTimestamp
    );
    auto create_end_ts_idx_stmt{
            db.prepare_statement(statement_buffer.data(), statement_buffer.size())
    };
    create_end_ts_idx_stmt.step();
    statement_buffer.clear();

    fmt::format_to(
            statement_buffer_ix,
            "CREATE INDEX IF NOT EXISTS files_segment_order ON {} ({},{})",
            test_table.get_name(),
            clp::streaming_archive::cMetadataDB::File::SegmentId,
            clp::streaming_archive::cMetadataDB::File::SegmentTimestampsPosition
    );
    auto create_segment_order_idx_stmt{
            db.prepare_statement(statement_buffer.data(), statement_buffer.size())
    };
    create_segment_order_idx_stmt.step();
    statement_buffer.clear();
}

[[nodiscard]] auto get_test_sqlite_db_abs_path() -> std::filesystem::path {
    std::filesystem::path const file_path{__FILE__};
    auto const test_root_path{file_path.parent_path()};
    return test_root_path / "test_sqlite_db/test.db";
}

auto sqlite_db_create(std::vector<TestRow> const& rows) -> void {
    auto const db_path{get_test_sqlite_db_abs_path()};
    if (std::filesystem::exists(db_path)) {
        REQUIRE(std::filesystem::remove(db_path));
    }

    SQLiteDB sqlite_db;
    sqlite_db.open(db_path.string());

    // Create a new table and indices.
    create_table_and_indices(sqlite_db);

    // Insert rows to the table.
    auto const& test_table{get_test_table()};
    auto transaction_begin_stmt{sqlite_db.prepare_statement("BEGIN TRANSACTION")};
    auto transaction_end_stmt{sqlite_db.prepare_statement("END TRANSACTION")};
    fmt::memory_buffer stmt_buf;
    auto statement_buffer_ix{std::back_inserter(stmt_buf)};
    fmt::format_to(
            statement_buffer_ix,
            "INSERT INTO {} ({}) VALUES ({})",
            test_table.get_name(),
            clp::get_field_names_sql(test_table.get_columns()),
            clp::get_numbered_placeholders_sql(test_table.get_columns().size())
    );
    auto insert_stmt{sqlite_db.prepare_statement(stmt_buf.data(), stmt_buf.size())};
    stmt_buf.clear();

    auto insert_row = [&](TestRow const& row) -> void {
        transaction_begin_stmt.step();
        row.insert_to_table(insert_stmt, test_table);
        transaction_end_stmt.step();
        transaction_begin_stmt.reset();
        transaction_end_stmt.reset();
    };
    std::for_each(rows.cbegin(), rows.cend(), insert_row);

    sqlite_db.close();
}
}  // namespace

TEST_CASE("sqlite_db_create", "[SQLiteDB]") {
    std::vector<TestRow> rows{
            {"0.log", 1000, 2000, 0, 0},
            {"1.log", 1200, 1800, 0, 30},
            {"2.log", 800, 3800, 1, 0}
    };
    sqlite_db_create(rows);
}

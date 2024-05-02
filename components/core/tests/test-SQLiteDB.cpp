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
/**
 * This class defines a SQlite table schema for testing purpose, It includes the data column name
 * and types.
 */
class TestTableSchema {
public:
    static constexpr auto* cPath{
            static_cast<char const*>(clp::streaming_archive::cMetadataDB::File::Path)
    };
    static constexpr auto* cBeginTs{
            static_cast<char const*>(clp::streaming_archive::cMetadataDB::File::BeginTimestamp)
    };
    static constexpr auto* cEndTs{
            static_cast<char const*>(clp::streaming_archive::cMetadataDB::File::EndTimestamp)
    };
    static constexpr auto* cSegmentId{
            static_cast<char const*>(clp::streaming_archive::cMetadataDB::File::SegmentId)
    };
    static constexpr auto* cSegmentTsPos{static_cast<char const*>(
            clp::streaming_archive::cMetadataDB::File::SegmentTimestampsPosition
    )};

    /**
     * This class represents a data row of the test table.
     */
    class Row {
    public:
        Row(std::string path,
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

        /**
         * Inserts a row into the table, using a prepared insert statement.
         * @param insert_stmt
         * @param table_schema
         */
        auto insert_to_table(
                SQLitePreparedStatement& insert_stmt,
                TestTableSchema const& table_schema
        ) const -> void {
            insert_stmt.bind_text(
                    table_schema.get_column_idx(TestTableSchema::cPath) + 1,
                    m_path,
                    false
            );
            insert_stmt.bind_int64(
                    table_schema.get_column_idx(TestTableSchema::cBeginTs) + 1,
                    m_begin_ts
            );
            insert_stmt.bind_int64(
                    table_schema.get_column_idx(TestTableSchema::cEndTs) + 1,
                    m_end_ts
            );
            insert_stmt.bind_int64(
                    table_schema.get_column_idx(TestTableSchema::cSegmentId) + 1,
                    static_cast<int64_t>(m_segment_id)
            );
            insert_stmt.bind_int64(
                    table_schema.get_column_idx(TestTableSchema::cSegmentTsPos) + 1,
                    static_cast<int64_t>(m_segment_ts_pos)
            );
            insert_stmt.step();
            insert_stmt.reset();
        }

        [[nodiscard]] auto operator==(Row const& rhs) const -> bool {
            return rhs.m_path == m_path && rhs.m_begin_ts == m_begin_ts && rhs.m_end_ts == m_end_ts
                   && rhs.m_segment_id == m_segment_id && rhs.m_segment_ts_pos == m_segment_ts_pos;
        }

    private:
        std::string m_path;
        epochtime_t m_begin_ts;
        epochtime_t m_end_ts;
        size_t m_segment_id;
        size_t m_segment_ts_pos;
    };

    TestTableSchema() {
        auto add_column = [&](std::string_view column_name, std::string_view type) -> void {
            m_columns.emplace_back(column_name);
            m_column_types.emplace_back(column_name, type);
            m_column_idx_map.emplace(column_name, static_cast<int>(m_column_idx_map.size()));
        };

        add_column(cPath, "TEXT PRIMARY KEY");
        add_column(cBeginTs, "INTEGER");
        add_column(cEndTs, "INTEGER");
        add_column(cSegmentId, "INTEGER");
        add_column(cSegmentTsPos, "INTEGER");
    }

    /**
     * Gets the index of a given column in the table. If the given column doesn't exist, this method
     * will fail the test infrastructure insertion.
     * @param column_name
     * @return The index of the column.
     */
    [[nodiscard]] auto get_column_idx(std::string const& column_name) const -> int {
        auto const it{m_column_idx_map.find(column_name)};
        REQUIRE((m_column_idx_map.cend() != it));
        return it->second;
    }

    [[nodiscard]] auto get_name() const -> std::string_view { return m_name; }

    [[nodiscard]] auto get_columns() const -> std::vector<std::string> const& { return m_columns; }

    [[nodiscard]] auto get_column_types() const
            -> std::vector<std::pair<std::string, std::string>> const& {
        return m_column_types;
    }

    [[nodiscard]] auto get_column_idx_map() const -> std::unordered_map<std::string, int> const& {
        return m_column_idx_map;
    }

private:
    std::string m_name{"CLP_TEST_TABLE"};
    std::vector<std::string> m_columns;
    std::vector<std::pair<std::string, std::string>> m_column_types;
    std::unordered_map<std::string, int> m_column_idx_map;
};

/**
 * Gets the test table schema which has a global storage. It will be initialized only once.
 * @return A const reference to the test table schema.
 */
[[nodiscard]] auto get_test_table_schema() -> TestTableSchema const& {
    static std::unique_ptr<TestTableSchema> table_ptr{nullptr};
    if (nullptr == table_ptr) {
        table_ptr = std::make_unique<TestTableSchema>();
    }
    return *table_ptr;
}

/**
 * Creates the table using the test table schemas.
 * @param db A SQLite database.
 */
auto create_table(SQLiteDB& db) -> void {
    fmt::memory_buffer statement_buffer;
    auto stmt_buf_ix{std::back_inserter(statement_buffer)};
    auto const& test_table_schema{get_test_table_schema()};

    fmt::format_to(
            stmt_buf_ix,
            "CREATE TABLE IF NOT EXISTS {} ({}) WITHOUT ROWID",
            test_table_schema.get_name(),
            clp::get_field_names_and_types_sql(test_table_schema.get_column_types())
    );
    auto create_table_stmt{db.prepare_statement(statement_buffer.data(), statement_buffer.size())};
    create_table_stmt.step();
    statement_buffer.clear();
}

/**
 * Creates indices in the table.
 * @param db A SQLite database.
 */
auto create_indices(SQLiteDB& db) -> void {
    fmt::memory_buffer statement_buffer;
    auto stmt_buf_ix{std::back_inserter(statement_buffer)};
    auto const& test_table_schema{get_test_table_schema()};

    fmt::format_to(
            stmt_buf_ix,
            "CREATE INDEX IF NOT EXISTS files_begin_timestamp ON {} ({})",
            test_table_schema.get_name(),
            clp::streaming_archive::cMetadataDB::File::BeginTimestamp
    );
    auto create_begin_ts_idx_stmt{
            db.prepare_statement(statement_buffer.data(), statement_buffer.size())
    };
    create_begin_ts_idx_stmt.step();
    statement_buffer.clear();

    fmt::format_to(
            stmt_buf_ix,
            "CREATE INDEX IF NOT EXISTS files_end_timestamp ON {} ({})",
            test_table_schema.get_name(),
            clp::streaming_archive::cMetadataDB::File::EndTimestamp
    );
    auto create_end_ts_idx_stmt{
            db.prepare_statement(statement_buffer.data(), statement_buffer.size())
    };
    create_end_ts_idx_stmt.step();
    statement_buffer.clear();

    fmt::format_to(
            stmt_buf_ix,
            "CREATE INDEX IF NOT EXISTS files_segment_order ON {} ({},{})",
            test_table_schema.get_name(),
            clp::streaming_archive::cMetadataDB::File::SegmentId,
            clp::streaming_archive::cMetadataDB::File::SegmentTimestampsPosition
    );
    auto create_segment_order_idx_stmt{
            db.prepare_statement(statement_buffer.data(), statement_buffer.size())
    };
    create_segment_order_idx_stmt.step();
    statement_buffer.clear();
}

/**
 * @return The absolute path of the generated test sqlite database.
 */
[[nodiscard]] auto get_test_sqlite_db_abs_path() -> std::filesystem::path {
    return std::filesystem::current_path() / "sqlite_test.db";
}

/**
 * Creates a SQLite database with a table specified in `TestTableSchema`, and insert all the rows
 * to the table.
 * @param rows
 */
auto sqlite_db_create(std::vector<TestTableSchema::Row> const& rows) -> void {
    auto const db_path{get_test_sqlite_db_abs_path()};
    if (std::filesystem::exists(db_path)) {
        REQUIRE((std::filesystem::remove(db_path)));
    }

    SQLiteDB sqlite_db;
    sqlite_db.open(db_path.string());

    // Create a new table and indices.
    create_table(sqlite_db);

    // Insert rows to the table.
    auto const& test_table_schema{get_test_table_schema()};
    auto transaction_begin_stmt{sqlite_db.prepare_statement("BEGIN TRANSACTION")};
    auto transaction_end_stmt{sqlite_db.prepare_statement("END TRANSACTION")};
    fmt::memory_buffer stmt_buf;
    auto stmt_buf_ix{std::back_inserter(stmt_buf)};
    fmt::format_to(
            stmt_buf_ix,
            "INSERT INTO {} ({}) VALUES ({})",
            test_table_schema.get_name(),
            clp::get_field_names_sql(test_table_schema.get_columns()),
            clp::get_numbered_placeholders_sql(test_table_schema.get_columns().size())
    );
    auto insert_stmt{sqlite_db.prepare_statement(stmt_buf.data(), stmt_buf.size())};
    stmt_buf.clear();

    auto insert_row = [&](TestTableSchema::Row const& row) -> void {
        transaction_begin_stmt.step();
        row.insert_to_table(insert_stmt, test_table_schema);
        transaction_end_stmt.step();
        transaction_begin_stmt.reset();
        transaction_end_stmt.reset();
    };
    std::for_each(rows.cbegin(), rows.cend(), insert_row);

    sqlite_db.close();
}
}  // namespace

TEST_CASE("sqlite_db_basic", "[SQLiteDB]") {
    std::vector<TestTableSchema::Row> ref_rows{
            // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
            {"0.log", 1000, 2000, 0, 0},
            {"1.log", 1200, 1800, 0, 30},
            {"2.log", 800, 3800, 1, 0}
            // NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
    };
    sqlite_db_create(ref_rows);

    auto const test_db_path{get_test_sqlite_db_abs_path()};
    SQLiteDB sqlite_db;
    sqlite_db.open(test_db_path.string());
    create_indices(sqlite_db);

    // Read all the columns from db, sorted by the begin ts
    auto const& test_table_schema{get_test_table_schema()};
    fmt::memory_buffer stmt_buf;
    auto stmt_buf_ix{std::back_inserter(stmt_buf)};
    fmt::format_to(
            stmt_buf_ix,
            "SELECT {} FROM {} ORDER BY {} ASC",
            clp::get_field_names_sql(test_table_schema.get_columns()),
            test_table_schema.get_name(),
            TestTableSchema::cBeginTs
    );
    auto select_stmt{sqlite_db.prepare_statement(stmt_buf.data(), stmt_buf.size())};
    stmt_buf.clear();

    std::vector<TestTableSchema::Row> rows;
    while (true) {
        select_stmt.step();
        if (false == select_stmt.is_row_ready()) {
            break;
        }
        std::string path;
        select_stmt.column_string(test_table_schema.get_column_idx(TestTableSchema::cPath), path);
        rows.emplace_back(
                path,
                select_stmt.column_int64(test_table_schema.get_column_idx(TestTableSchema::cBeginTs)
                ),
                select_stmt.column_int64(test_table_schema.get_column_idx(TestTableSchema::cEndTs)),
                select_stmt.column_int64(
                        test_table_schema.get_column_idx(TestTableSchema::cSegmentId)
                ),
                select_stmt.column_int64(
                        test_table_schema.get_column_idx(TestTableSchema::cSegmentTsPos)
                )
        );
    }

    // Sort the reference rows by the begin ts
    std::sort(
            ref_rows.begin(),
            ref_rows.end(),
            [](TestTableSchema::Row const& lhs, TestTableSchema::Row const& rhs) -> bool {
                if (lhs.get_begin_ts() == rhs.get_begin_ts()) {
                    return lhs.get_path() < rhs.get_path();
                }
                return lhs.get_begin_ts() < rhs.get_begin_ts();
            }
    );

    REQUIRE((ref_rows == rows));
    sqlite_db.close();

    // Cleanup
    REQUIRE((std::filesystem::remove(test_db_path)));
}

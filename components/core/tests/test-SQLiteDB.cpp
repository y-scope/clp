#include <algorithm>
#include <cstddef>
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
#include "../src/clp/SQLiteDB.hpp"
#include "../src/clp/SQLitePreparedStatement.hpp"

using epochtime_t = int64_t;
using clp::SQLiteDB;

namespace {
/**
 * A class for inserting and retrieving rows from the test table.
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

    [[nodiscard]] auto get_path() const -> std::string const& { return m_path; }

    [[nodiscard]] auto get_begin_ts() const -> epochtime_t { return m_begin_ts; }

    [[nodiscard]] auto get_end_ts() const -> epochtime_t { return m_end_ts; }

    [[nodiscard]] auto get_segment_id() const -> size_t { return m_segment_id; }

    [[nodiscard]] auto get_segment_ts_pos() const -> size_t { return m_segment_ts_pos; }

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

/**
 * A class that defines a sqlite table schema (column names and types) for testing purposes.
 */
class TestTableSchema {
public:
    static constexpr char const* cPath{"path"};
    static constexpr char const* cBeginTs{"begin_timestamp"};
    static constexpr char const* cEndTs{"end_timestamp"};
    static constexpr char const* cSegmentId{"segment_id"};
    static constexpr char const* cSegmentTsPos{"segment_timestamp_position"};

    TestTableSchema() {
        auto add_column = [&](std::string_view column_name, std::string_view type) -> void {
            m_columns.emplace_back(column_name);
            m_column_types.emplace_back(column_name, type);
        };

        add_column(cPath, "TEXT PRIMARY KEY");
        add_column(cBeginTs, "INTEGER");
        add_column(cEndTs, "INTEGER");
        add_column(cSegmentId, "INTEGER");
        add_column(cSegmentTsPos, "INTEGER");
    }

    [[nodiscard]] auto get_name() const -> std::string_view { return m_name; }

    [[nodiscard]] auto get_columns() const -> std::vector<std::string> const& { return m_columns; }

    [[nodiscard]] auto get_column_types(
    ) const -> std::vector<std::pair<std::string, std::string>> const& {
        return m_column_types;
    }

private:
    std::string m_name{"CLP_TEST_TABLE"};
    std::vector<std::string> m_columns;
    std::vector<std::pair<std::string, std::string>> m_column_types;
};

/**
 * Creates the table using the test table schemas.
 * @param db A SQLite database.
 * @param table_schema
 */
auto create_table(SQLiteDB& db, TestTableSchema const& table_schema) -> void {
    fmt::memory_buffer statement_buffer;
    auto stmt_buf_it{std::back_inserter(statement_buffer)};

    fmt::format_to(
            stmt_buf_it,
            "CREATE TABLE IF NOT EXISTS {} ({}) WITHOUT ROWID",
            table_schema.get_name(),
            clp::get_field_names_and_types_sql(table_schema.get_column_types())
    );
    auto create_table_stmt{db.prepare_statement(statement_buffer.data(), statement_buffer.size())};
    create_table_stmt.step();
    statement_buffer.clear();
}

/**
 * Creates indices in the table.
 * @param db A SQLite database.
 * @param table_schema
 */
auto create_indices(SQLiteDB& db, TestTableSchema const& table_schema) -> void {
    fmt::memory_buffer statement_buffer;
    auto stmt_buf_it{std::back_inserter(statement_buffer)};

    fmt::format_to(
            stmt_buf_it,
            "CREATE INDEX IF NOT EXISTS files_begin_timestamp ON {} ({})",
            table_schema.get_name(),
            TestTableSchema::cBeginTs
    );
    auto create_begin_ts_idx_stmt{
            db.prepare_statement(statement_buffer.data(), statement_buffer.size())
    };
    create_begin_ts_idx_stmt.step();
    statement_buffer.clear();

    fmt::format_to(
            stmt_buf_it,
            "CREATE INDEX IF NOT EXISTS files_end_timestamp ON {} ({})",
            table_schema.get_name(),
            TestTableSchema::cEndTs
    );
    auto create_end_ts_idx_stmt{
            db.prepare_statement(statement_buffer.data(), statement_buffer.size())
    };
    create_end_ts_idx_stmt.step();
    statement_buffer.clear();

    fmt::format_to(
            stmt_buf_it,
            "CREATE INDEX IF NOT EXISTS files_segment_order ON {} ({},{})",
            table_schema.get_name(),
            TestTableSchema::cSegmentId,
            TestTableSchema::cSegmentTsPos
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
[[nodiscard]] auto get_test_db_abs_path() -> std::filesystem::path {
    return std::filesystem::current_path() / "sqlite-test.db";
}

/**
 * Creates a SQLite database using the given table schema, and inserts the rows into the table.
 * @param table_schema
 * @param rows
 */
auto create_db(TestTableSchema const& table_schema, std::vector<Row> const& rows) -> void {
    auto const db_path{get_test_db_abs_path()};
    if (std::filesystem::exists(db_path)) {
        REQUIRE((std::filesystem::remove(db_path)));
    }

    SQLiteDB sqlite_db;
    sqlite_db.open(db_path.string());

    create_table(sqlite_db, table_schema);
    create_indices(sqlite_db, table_schema);

    // Insert rows into the table
    auto transaction_begin_stmt{sqlite_db.prepare_statement("BEGIN TRANSACTION")};
    auto transaction_end_stmt{sqlite_db.prepare_statement("END TRANSACTION")};
    fmt::memory_buffer stmt_buf;
    auto stmt_buf_it{std::back_inserter(stmt_buf)};
    auto const& table_columns{table_schema.get_columns()};
    fmt::format_to(
            stmt_buf_it,
            "INSERT INTO {} ({}) VALUES ({})",
            table_schema.get_name(),
            clp::get_field_names_sql(table_columns),
            clp::get_numbered_placeholders_sql(table_columns.size())
    );
    auto insert_stmt{sqlite_db.prepare_statement(stmt_buf.data(), stmt_buf.size())};
    stmt_buf.clear();

    // Create indicies for each column in the order they are defined in the insert statement,
    // counting from 1.
    std::unordered_map<std::string, int> placeholder_idx_map;
    int idx{1};
    for (auto const& column : table_columns) {
        placeholder_idx_map.emplace(column, idx++);
    }

    for (auto const& row : rows) {
        transaction_begin_stmt.step();

        // Bind values to the columns with the corresponded placeholder index.
        auto const path_placeholder_idx_it{placeholder_idx_map.find(TestTableSchema::cPath)};
        REQUIRE((placeholder_idx_map.cend() != path_placeholder_idx_it));
        insert_stmt.bind_text(path_placeholder_idx_it->second, row.get_path(), false);

        auto const begin_ts_placeholder_idx_it{placeholder_idx_map.find(TestTableSchema::cBeginTs)};
        REQUIRE((placeholder_idx_map.cend() != begin_ts_placeholder_idx_it));
        insert_stmt.bind_int64(begin_ts_placeholder_idx_it->second, row.get_begin_ts());

        auto const end_ts_placeholder_idx_it{placeholder_idx_map.find(TestTableSchema::cEndTs)};
        REQUIRE((placeholder_idx_map.cend() != end_ts_placeholder_idx_it));
        insert_stmt.bind_int64(end_ts_placeholder_idx_it->second, row.get_end_ts());

        auto const seg_id_placeholder_idx_it{placeholder_idx_map.find(TestTableSchema::cSegmentId)};
        REQUIRE((placeholder_idx_map.cend() != seg_id_placeholder_idx_it));
        insert_stmt.bind_int64(
                seg_id_placeholder_idx_it->second,
                static_cast<int64_t>(row.get_segment_id())
        );

        auto const seg_ts_pos_placeholder_idx_it{
                placeholder_idx_map.find(TestTableSchema::cSegmentTsPos)
        };
        REQUIRE((placeholder_idx_map.cend() != seg_ts_pos_placeholder_idx_it));
        insert_stmt.bind_int64(
                seg_ts_pos_placeholder_idx_it->second,
                static_cast<int64_t>(row.get_segment_ts_pos())
        );

        insert_stmt.step();
        insert_stmt.reset();

        transaction_end_stmt.step();
        transaction_begin_stmt.reset();
        transaction_end_stmt.reset();
    }

    sqlite_db.close();
}
}  // namespace

TEST_CASE("sqlite_db_basic", "[SQLiteDB]") {
    std::vector<Row> ref_rows{
            // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
            {"0.log", 1000, 2000, 0, 0},
            {"1.log", 1200, 1800, 0, 30},
            {"2.log", 800, 3800, 1, 0}
            // NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
    };
    TestTableSchema const table_schema;
    create_db(table_schema, ref_rows);

    auto test_db_path{get_test_db_abs_path()};
    SQLiteDB sqlite_db;
    sqlite_db.open(test_db_path.string());

    // Read all the columns from db, sorted by the begin ts
    fmt::memory_buffer stmt_buf;
    auto stmt_buf_it{std::back_inserter(stmt_buf)};
    auto const& table_columns{table_schema.get_columns()};
    fmt::format_to(
            stmt_buf_it,
            "SELECT {} FROM {} ORDER BY {} ASC",
            clp::get_field_names_sql(table_columns),
            table_schema.get_name(),
            TestTableSchema::cBeginTs
    );
    auto select_stmt{sqlite_db.prepare_statement(stmt_buf.data(), stmt_buf.size())};
    stmt_buf.clear();

    // Create indicies for each column in the order they are defined in the insert statement,
    // counting from 0.
    std::unordered_map<std::string, int> selected_column_idx;
    size_t idx{0};
    for (auto const& column : table_columns) {
        selected_column_idx.emplace(column, idx++);
    }

    std::vector<Row> rows;
    while (true) {
        select_stmt.step();
        if (false == select_stmt.is_row_ready()) {
            break;
        }

        std::string path;
        auto const path_idx_it{selected_column_idx.find(TestTableSchema::cPath)};
        REQUIRE((selected_column_idx.cend() != path_idx_it));
        select_stmt.column_string(path_idx_it->second, path);

        auto const begin_ts_idx_it{selected_column_idx.find(TestTableSchema::cBeginTs)};
        REQUIRE((selected_column_idx.cend() != begin_ts_idx_it));
        epochtime_t const begin_ts{select_stmt.column_int64(begin_ts_idx_it->second)};

        auto const end_ts_idx_it{selected_column_idx.find(TestTableSchema::cEndTs)};
        REQUIRE((selected_column_idx.cend() != end_ts_idx_it));
        epochtime_t const end_ts{select_stmt.column_int64(end_ts_idx_it->second)};

        auto const seg_id_idx_it{selected_column_idx.find(TestTableSchema::cSegmentId)};
        REQUIRE((selected_column_idx.cend() != seg_id_idx_it));
        size_t const seg_id{static_cast<size_t>(select_stmt.column_int64(seg_id_idx_it->second))};

        auto const seg_ts_pos_idx_it{selected_column_idx.find(TestTableSchema::cSegmentTsPos)};
        REQUIRE((selected_column_idx.cend() != seg_ts_pos_idx_it));
        size_t const seg_ts_pos{
                static_cast<size_t>(select_stmt.column_int64(seg_ts_pos_idx_it->second))
        };

        rows.emplace_back(path, begin_ts, end_ts, seg_id, seg_ts_pos);
    }

    // Sort the reference rows by the begin ts
    std::sort(ref_rows.begin(), ref_rows.end(), [](Row const& lhs, Row const& rhs) -> bool {
        if (lhs.get_begin_ts() == rhs.get_begin_ts()) {
            return lhs.get_path() < rhs.get_path();
        }
        return lhs.get_begin_ts() < rhs.get_begin_ts();
    });

    REQUIRE((ref_rows == rows));
    sqlite_db.close();

    // Cleanup
    REQUIRE(std::filesystem::remove(test_db_path));
}

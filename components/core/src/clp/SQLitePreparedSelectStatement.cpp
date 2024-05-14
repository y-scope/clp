#include "SQLitePreparedSelectStatement.hpp"

#include <iterator>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <fmt/core.h>
#include <fmt/fmt.h>
#include <sqlite3/sqlite3.h>

#include "database_utils.hpp"
#include "ErrorCode.hpp"
#include "SQLitePreparedStatement.hpp"

namespace clp {
auto SQLitePreparedSelectStatement::create_sqlite_prepared_select_statement(
        std::vector<std::string> const& columns_to_select,
        std::string_view table,
        std::vector<std::string> const& where_clause,
        std::vector<std::string> const& ordering_clause,
        sqlite3* db_handle
) -> SQLitePreparedSelectStatement {
    if (columns_to_select.empty()) {
        throw OperationFailed(
                ErrorCode_Failure,
                __FILENAME__,
                __LINE__,
                "clp::SQLitePreparedSelectStatement Failed: no column to select."
        );
    }

    std::unordered_map<std::string, size_t> idx_map;
    fmt::memory_buffer statement_buffer;
    auto statement_buffer_ix{std::back_inserter(statement_buffer)};

    fmt::format_to(
            statement_buffer_ix,
            "SELECT {} FROM {}",
            get_field_names_sql(columns_to_select),
            table
    );
    size_t idx{0};
    for (auto const& column : columns_to_select) {
        idx_map.emplace(column, idx++);
    }

    if (false == where_clause.empty()) {
        bool is_first{true};
        for (auto const& filter : where_clause) {
            fmt::format_to(statement_buffer_ix, " {} {}", is_first ? "WHERE" : "AND", filter);
            is_first = false;
        }
    }

    if (false == ordering_clause.empty()) {
        fmt::format_to(statement_buffer_ix, " ORDER BY");
        bool is_first{true};
        for (auto const& ordering : ordering_clause) {
            if (is_first) {
                fmt::format_to(statement_buffer_ix, " {}", ordering);
                is_first = false;
                continue;
            }
            fmt::format_to(statement_buffer_ix, ", {}", ordering);
        }
    }

    return SQLitePreparedSelectStatement{
            {statement_buffer.data(), statement_buffer.size()},
            db_handle,
            idx_map
    };
}

auto SQLitePreparedSelectStatement::get_selected_column_idx(std::string const& selected_column
) const -> size_t {
    auto const idx_it{m_idx_map.find(selected_column)};
    if (m_idx_map.cend() == idx_it) {
        std::string const msg{
                "clp::SQLitePreparedSelectStatement Failed with unknown selected column: "
        };
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__, msg + selected_column);
    }
    return idx_it->second;
}
}  // namespace clp

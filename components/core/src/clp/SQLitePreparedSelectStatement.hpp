#ifndef CLP_SQLITEPREPAREDSELECTSTATEMENT_HPP
#define CLP_SQLITEPREPAREDSELECTSTATEMENT_HPP

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <sqlite3/sqlite3.h>

#include "SQLitePreparedStatement.hpp"
#include "TraceableException.hpp"

/**
 * This class provides abstractions to a sqlite select statement. It maintains a map to lookup the
 * index of a selected column in the statement from a given name.
 */
namespace clp {
class SQLitePreparedSelectStatement : public SQLitePreparedStatement {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(
                ErrorCode error_code,
                char const* const filename,
                int line_number,
                std::string msg
        )
                : TraceableException{error_code, filename, line_number},
                  m_msg{std::move(msg)} {}

        // Methods
        auto what() const noexcept -> char const* override { return m_msg.c_str(); }

    private:
        std::string m_msg;
    };

    // Factory functions.
    /**
     * Constructs and returns a SQLite select statement with given parameters.
     * @param columns_to_select The name of columns to select.
     * @param table The name of the table to select from.
     * @param where_clause SQLite `WHERE` clause to filter rows in the result set.
     * @param ordering_clause SQLite `ORDER BY` clause to sort the result set.
     * @param db_handle
     * @throw clp::SQLitePreparedStatement::OperationFailed on failure.
     */
    [[nodiscard]] static auto create_sqlite_prepared_select_statement(
            std::vector<std::string> const& columns_to_select,
            std::string_view table,
            std::vector<std::string> where_clause,
            std::vector<std::string> ordering_clause,
            sqlite3* db_handle
    ) -> SQLitePreparedSelectStatement;

    virtual ~SQLitePreparedSelectStatement() = default;

    // Deletes copy constructor and assignment.
    SQLitePreparedSelectStatement(SQLitePreparedSelectStatement const&) = delete;
    auto operator=(SQLitePreparedSelectStatement const&) -> SQLitePreparedSelectStatement& = delete;

    // Defines default move constructor and assignment.
    SQLitePreparedSelectStatement(SQLitePreparedSelectStatement&& rhs) noexcept = default;
    auto operator=(SQLitePreparedSelectStatement&& rhs
    ) noexcept -> SQLitePreparedSelectStatement& = default;

    // Methods implementing `clp::SQLitePreparedStatement`
    virtual auto reset() -> void {
        m_idx_map.clear();
        SQLitePreparedStatement::reset();
    }

    // Methods
    /**
     * @param selected_column
     * @return The int value from the selected column.
     * @throw OperationFailed if the name is not a valid selected column.
     */
    [[nodiscard]] auto column_int(std::string const& selected_column) const -> int {
        return SQLitePreparedStatement::column_int(
                static_cast<int>(get_selected_column_idx(selected_column))
        );
    }

    /**
     * @param selected_column
     * @return The int64 value from the selected column.
     * @throw OperationFailed if the name is not a valid selected column.
     */
    [[nodiscard]] auto column_int64(std::string const& selected_column) const -> int64_t {
        return SQLitePreparedStatement::column_int64(
                static_cast<int>(get_selected_column_idx(selected_column))
        );
    }

    /**
     * @param selected_column
     * @param value Returns the string value from the selected column.
     * @throw OperationFailed if the name is not a valid selected column.
     */
    auto column_string(std::string const& selected_column, std::string& value) const -> void {
        return SQLitePreparedStatement::column_string(
                static_cast<int>(get_selected_column_idx(selected_column)),
                value
        );
    }

    /**
     * Binds an int value to a selected column.
     * @param selected_column
     * @param val
     * @throw OperationFailed if the name is not a valid selected column.
     */
    auto bind_selected_column_int(std::string const& selected_column, int val) -> void {
        return SQLitePreparedStatement::bind_int(
                static_cast<int>(get_selected_column_idx(selected_column)),
                val
        );
    }

    /**
     * Binds an int64 value to a selected column.
     * @param selected_column
     * @param val
     * @throw OperationFailed if the name is not a valid selected column.
     */
    auto bind_selected_column_int64(std::string const& selected_column, int64_t val) -> void {
        return SQLitePreparedStatement::bind_int64(
                static_cast<int>(get_selected_column_idx(selected_column)),
                val
        );
    }

    /**
     * Binds a text (string) value to a selected column.
     * @param selected_column
     * @param val
     * @param copy_parameter
     * @throw OperationFailed if the name is not a valid selected column.
     */
    auto bind_selected_column_text(
            std::string const& selected_column,
            std::string const& val,
            bool copy_parameter
    ) {
        return SQLitePreparedStatement::bind_text(
                static_cast<int>(get_selected_column_idx(selected_column)),
                val,
                copy_parameter
        );
    }

protected:
    /**
     * Constructor. It should not be accessible outside of the class.
     * `create_sqlite_prepared_select_statement` should be used to create an instance of this
     * class.
     */
    explicit SQLitePreparedSelectStatement(
            std::string_view statement,
            sqlite3* db_handle,
            std::unordered_map<std::string, size_t> idx_map
    )
            : SQLitePreparedStatement{statement.data(), statement.size(), db_handle},
              m_idx_map{std::move(idx_map)} {}

    /**
     * @param selected_column
     * @return The idx of the selected column in the statement.
     * @throw OperationFailed if the name doesn't appear in the select statement.
     */
    [[nodiscard]] auto get_selected_column_idx(std::string const& selected_column) const -> size_t {
        auto const idx_it{m_idx_map.find(selected_column)};
        if (m_idx_map.cend() == idx_it) {
            std::string const msg{
                    "clp::SQLitePreparedSelectStatement Failed with unknown selected column: "
            };
            throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__, msg + selected_column);
        }
        return idx_it->second;
    }

    std::unordered_map<std::string, size_t> m_idx_map;
};
}  // namespace clp

#endif

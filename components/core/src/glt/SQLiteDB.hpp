#ifndef GLT_SQLITEDB_HPP
#define GLT_SQLITEDB_HPP

#include <string>

#include "ErrorCode.hpp"
#include "sqlite3/sqlite3.h"
#include "SQLitePreparedStatement.hpp"
#include "TraceableException.hpp"

namespace glt {
class SQLiteDB {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        // Methods
        char const* what() const noexcept override { return "SQLiteDB operation failed"; }
    };

    // Constructors
    SQLiteDB() : m_db_handle(nullptr) {}

    // Methods
    void open(std::string const& path);
    bool close();

    SQLitePreparedStatement prepare_statement(char const* statement, size_t statement_length);

    SQLitePreparedStatement prepare_statement(std::string const& statement) {
        return prepare_statement(statement.c_str(), statement.length());
    }

    char const* get_error_message() { return sqlite3_errmsg(m_db_handle); }

private:
    // Variables
    sqlite3* m_db_handle;
};
}  // namespace glt

#endif  // GLT_SQLITEDB_HPP

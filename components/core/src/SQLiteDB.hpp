#ifndef SQLITEDB_HPP
#define SQLITEDB_HPP

// C++ standard libraries
#include <string>

// sqlite3
#include "../submodules/sqlite3/sqlite3.h"

// Project headers
#include "ErrorCode.hpp"
#include "SQLitePreparedStatement.hpp"
#include "TraceableException.hpp"

class SQLiteDB {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

        // Methods
        const char* what () const noexcept override {
            return "SQLiteDB operation failed";
        }
    };

    // Constructors
    SQLiteDB () : m_db_handle(nullptr) {}

    // Methods
    void open (const std::string& path);
    bool close ();

    SQLitePreparedStatement prepare_statement (const char* statement, size_t statement_length);
    SQLitePreparedStatement prepare_statement (const std::string& statement) { return prepare_statement(statement.c_str(), statement.length()); }

    const char* get_error_message () { return sqlite3_errmsg(m_db_handle); }

private:
    // Variables
    sqlite3* m_db_handle;
};

#endif // SQLITEDB_HPP

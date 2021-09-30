#ifndef SQLITEPREPAREDSTATEMENT_HPP
#define SQLITEPREPAREDSTATEMENT_HPP

// C++ standard libraries
#include <string>

// sqlite3
#include "../submodules/sqlite3/sqlite3.h"

// Project headers
#include "ErrorCode.hpp"
#include "TraceableException.hpp"

class SQLitePreparedStatement {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

        // Methods
        const char* what () const noexcept override {
            return "SQLitePreparedStatement operation failed";
        }
    };

    // Constructors
    SQLitePreparedStatement (const char* statement, size_t statement_length, sqlite3* db_handle);
    ~SQLitePreparedStatement ();

    // Delete copy constructor and assignment
    SQLitePreparedStatement (const SQLitePreparedStatement&) = delete;
    SQLitePreparedStatement& operator= (const SQLitePreparedStatement&) = delete;

    // Move constructor and assignment
    SQLitePreparedStatement (SQLitePreparedStatement&& rhs) noexcept;
    SQLitePreparedStatement& operator=(SQLitePreparedStatement&& rhs) noexcept;

    // Methods
    void bind_int (int parameter_index, int value);
    void bind_int (const std::string& parameter_name, int value);
    void bind_int64 (int parameter_index, int64_t value);
    void bind_int64 (const std::string& parameter_name, int64_t value);
    void bind_text (int parameter_index, const std::string& value, bool copy_parameter);
    void bind_text (const std::string& parameter_name, const std::string& value, bool copy_parameter);
    void reset ();

    bool step ();
    int column_int (int parameter_index) const;
    int column_int (const std::string& parameter_name) const;
    int64_t column_int64 (int parameter_index) const;
    int64_t column_int64 (const std::string& parameter_name) const;
    void column_string (int parameter_index, std::string& value) const;
    void column_string (const std::string& parameter_name, std::string& value) const;

    bool is_row_ready () const { return m_row_ready; }

private:
    // Members
    sqlite3* m_db_handle;
    sqlite3_stmt* m_statement_handle;
    bool m_row_ready;
};

#endif // SQLITEPREPAREDSTATEMENT_HPP

#ifndef MYSQLPREPAREDSTATEMENT_HPP
#define MYSQLPREPAREDSTATEMENT_HPP

// C++ standard libraries
#include <string>
#include <vector>

// MySQL
#include <mariadb/mysql.h>

// Project headers
#include "ErrorCode.hpp"
#include "MySQLParamBindings.hpp"
#include "TraceableException.hpp"

class MySQLPreparedStatement {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

        // Methods
        const char* what () const noexcept override {
            return "MySQLPreparedStatement operation failed";
        }
    };

    // Constructors
    explicit MySQLPreparedStatement (MYSQL* db_handle);

    // Delete copy constructor and assignment
    MySQLPreparedStatement (const MySQLPreparedStatement&) = delete;
    MySQLPreparedStatement& operator= (const MySQLPreparedStatement&) = delete;

    // Move constructor and assignment
    MySQLPreparedStatement (MySQLPreparedStatement&& rhs) noexcept;
    MySQLPreparedStatement& operator= (MySQLPreparedStatement&& rhs) noexcept;

    // Destructor
    ~MySQLPreparedStatement ();

    // Methods
    void set (const char* statement, size_t statement_length);
    bool execute ();

    MySQLParamBindings& get_statement_bindings () { return m_statement_bindings; }

private:
    // Methods
    void close ();

    // Variables
    MYSQL* m_db_handle;

    MYSQL_STMT* m_statement_handle;
    MySQLParamBindings m_statement_bindings;

    bool m_is_set;
};

#endif // MYSQLPREPAREDSTATEMENT_HPP

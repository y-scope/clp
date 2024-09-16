#ifndef CLP_MYSQLPREPAREDSTATEMENT_HPP
#define CLP_MYSQLPREPAREDSTATEMENT_HPP

#include <string>
#include <vector>

#include <mysql.h>

#include "ErrorCode.hpp"
#include "MySQLParamBindings.hpp"
#include "TraceableException.hpp"

namespace clp {
class MySQLPreparedStatement {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        // Methods
        char const* what() const noexcept override {
            return "MySQLPreparedStatement operation failed";
        }
    };

    // Constructors
    explicit MySQLPreparedStatement(MYSQL* db_handle);

    // Delete copy constructor and assignment
    MySQLPreparedStatement(MySQLPreparedStatement const&) = delete;
    MySQLPreparedStatement& operator=(MySQLPreparedStatement const&) = delete;

    // Move constructor and assignment
    MySQLPreparedStatement(MySQLPreparedStatement&& rhs) noexcept;
    MySQLPreparedStatement& operator=(MySQLPreparedStatement&& rhs) noexcept;

    // Destructor
    ~MySQLPreparedStatement();

    // Methods
    void set(char const* statement, size_t statement_length);
    bool execute();

    MySQLParamBindings& get_statement_bindings() { return m_statement_bindings; }

private:
    // Methods
    void close();

    // Variables
    MYSQL* m_db_handle;

    MYSQL_STMT* m_statement_handle;
    MySQLParamBindings m_statement_bindings;

    bool m_is_set;
};
}  // namespace clp

#endif  // CLP_MYSQLPREPAREDSTATEMENT_HPP

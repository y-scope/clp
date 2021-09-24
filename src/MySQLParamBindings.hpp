#ifndef MYSQLPARAMBINDINGS_HPP
#define MYSQLPARAMBINDINGS_HPP

// C++ standard libraries
#include <cstdint>
#include <vector>

// MySQL libraries
#include <mariadb/mysql.h>

// Project headers
#include "ErrorCode.hpp"
#include "TraceableException.hpp"

/**
 * Class representing parameter bindings for a prepared SQL statement
 */
class MySQLParamBindings {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

        // Methods
        const char* what () const noexcept override {
            return "MySQLParamBindings operation failed";
        }
    };

    // Methods
    /**
     * Clears all bindings
     */
    void clear ();
    /**
     * Resizes the bindings array
     * @param num_fields
     */
    void resize (size_t num_fields);

    void bind_int64 (size_t field_index, int64_t& value);
    void bind_uint64 (size_t field_index, uint64_t& value);
    void bind_varchar (size_t field_index, const char* value, size_t value_length);

    MYSQL_BIND* get_internal_mysql_bindings () { return m_statement_bindings.data(); }

private:
    // Variables
    std::vector<MYSQL_BIND> m_statement_bindings;
    std::vector<unsigned long> m_statement_binding_lengths;
};

#endif // MYSQLPARAMBINDINGS_HPP

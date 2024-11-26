#ifndef CLP_MYSQLPARAMBINDINGS_HPP
#define CLP_MYSQLPARAMBINDINGS_HPP

#include <cstdint>
#include <vector>

#include <mysql.h>

#include "ErrorCode.hpp"
#include "TraceableException.hpp"

namespace clp {
/**
 * Class representing parameter bindings for a prepared SQL statement
 */
class MySQLParamBindings {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        // Methods
        char const* what() const noexcept override { return "MySQLParamBindings operation failed"; }
    };

    // Methods
    /**
     * Clears all bindings
     */
    void clear();
    /**
     * Resizes the bindings array
     * @param num_fields
     */
    void resize(size_t num_fields);

    void bind_int64(size_t field_index, int64_t& value);
    void bind_uint64(size_t field_index, uint64_t& value);
    void bind_varchar(size_t field_index, char const* value, size_t value_length);

    MYSQL_BIND* get_internal_mysql_bindings() { return m_statement_bindings.data(); }

private:
    // Variables
    std::vector<MYSQL_BIND> m_statement_bindings;
    std::vector<unsigned long> m_statement_binding_lengths;
};
}  // namespace clp

#endif  // CLP_MYSQLPARAMBINDINGS_HPP

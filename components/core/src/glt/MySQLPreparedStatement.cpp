#include "MySQLPreparedStatement.hpp"

#include "Defs.h"
#include "spdlog_with_specializations.hpp"

using std::string;

namespace glt {
MySQLPreparedStatement::MySQLPreparedStatement(MYSQL* db_handle)
        : m_db_handle(db_handle),
          m_is_set(false) {
    m_statement_handle = mysql_stmt_init(m_db_handle);
    if (nullptr == m_statement_handle) {
        SPDLOG_ERROR(
                "MySQLPreparedStatement: Failed to create statement - {}.",
                mysql_error(m_db_handle)
        );
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }
}

MySQLPreparedStatement::MySQLPreparedStatement(MySQLPreparedStatement&& rhs) noexcept
        : m_db_handle(nullptr),
          m_statement_handle(nullptr),
          m_is_set(false) {
    *this = std::move(rhs);
}

MySQLPreparedStatement& MySQLPreparedStatement::operator=(MySQLPreparedStatement&& rhs) noexcept {
    if (this != &rhs) {
        close();

        m_db_handle = rhs.m_db_handle;
        m_statement_handle = rhs.m_statement_handle;
        m_statement_bindings = std::move(rhs.m_statement_bindings);
        m_is_set = rhs.m_is_set;

        rhs.m_db_handle = nullptr;
        rhs.m_statement_handle = nullptr;
        rhs.m_is_set = false;
    }

    return *this;
}

MySQLPreparedStatement::~MySQLPreparedStatement() {
    close();
    m_db_handle = nullptr;
    m_is_set = false;
}

void MySQLPreparedStatement::set(char const* statement, size_t statement_length) {
    if (m_is_set) {
        throw OperationFailed(ErrorCode_NotReady, __FILENAME__, __LINE__);
    }

    if (0 != mysql_stmt_prepare(m_statement_handle, statement, statement_length)) {
        SPDLOG_ERROR(
                "MySQLPreparedStatement: Failed to prepare statement - {}. '{:.{}}'",
                mysql_stmt_error(m_statement_handle),
                statement,
                statement_length
        );
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }
    m_statement_bindings.resize(mysql_stmt_param_count(m_statement_handle));
    m_is_set = true;
}

bool MySQLPreparedStatement::execute() {
    if (0
        != mysql_stmt_bind_param(
                m_statement_handle,
                m_statement_bindings.get_internal_mysql_bindings()
        ))
    {
        SPDLOG_ERROR(
                "MySQLPreparedStatement: Failed to bind parameters to statement - {}.",
                mysql_stmt_error(m_statement_handle)
        );
        return false;
    }

    if (0 != mysql_stmt_execute(m_statement_handle)) {
        SPDLOG_ERROR(
                "MySQLPreparedStatement: Failed to execute statement - {}.",
                mysql_stmt_error(m_statement_handle)
        );
        return false;
    }

    return true;
}

void MySQLPreparedStatement::close() {
    if (nullptr != m_statement_handle) {
        if (0 != mysql_stmt_close(m_statement_handle)) {
            SPDLOG_ERROR(
                    "MySQLPreparedStatement: Failed to delete statement - {}.",
                    mysql_error(m_db_handle)
            );
        }
        m_statement_handle = nullptr;
        m_statement_bindings.clear();
    }
}
}  // namespace glt

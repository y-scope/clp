#include "MySQLDB.hpp"

#include "spdlog_with_specializations.hpp"

using std::string;

namespace clp {
MySQLDB::Iterator::Iterator(MYSQL* m_db_handle)
        : m_row(nullptr),
          m_field_lengths(nullptr),
          m_num_fields(0) {
    m_query_result = mysql_use_result(m_db_handle);
    if (nullptr == m_query_result) {
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }

    fetch_next_row();
}

MySQLDB::Iterator::Iterator(Iterator&& rhs) noexcept
        : m_query_result(nullptr),
          m_row(nullptr),
          m_field_lengths(nullptr),
          m_num_fields(0) {
    *this = std::move(rhs);
}

MySQLDB::Iterator& MySQLDB::Iterator::operator=(MySQLDB::Iterator&& rhs) noexcept {
    if (this != &rhs) {
        if (nullptr != m_query_result) {
            mysql_free_result(m_query_result);
            m_query_result = nullptr;
        }

        m_query_result = rhs.m_query_result;
        m_row = rhs.m_row;
        m_field_lengths = rhs.m_field_lengths;
        m_num_fields = rhs.m_num_fields;

        rhs.m_query_result = nullptr;
        rhs.m_row = nullptr;
        rhs.m_field_lengths = nullptr;
        rhs.m_num_fields = 0;
    }

    return *this;
}

MySQLDB::Iterator::~Iterator() {
    if (nullptr != m_query_result) {
        m_row = nullptr;
        m_field_lengths = nullptr;
        m_num_fields = 0;
        mysql_free_result(m_query_result);
        m_query_result = nullptr;
    }
}

bool MySQLDB::Iterator::contains_element() const {
    return (nullptr != m_row);
}

void MySQLDB::Iterator::get_next() {
    if (nullptr != m_row) {
        fetch_next_row();
    }
}

void MySQLDB::Iterator::get_field_as_string(size_t field_ix, string& field_value) {
    if (nullptr == m_row) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }
    if (field_ix >= m_num_fields) {
        throw OperationFailed(ErrorCode_OutOfBounds, __FILENAME__, __LINE__);
    }

    field_value.assign(m_row[field_ix], m_field_lengths[field_ix]);
}

void MySQLDB::Iterator::fetch_next_row() {
    m_row = mysql_fetch_row(m_query_result);
    if (nullptr != m_row) {
        m_field_lengths = mysql_fetch_lengths(m_query_result);
        m_num_fields = mysql_num_fields(m_query_result);
    }
}

MySQLDB::~MySQLDB() {
    if (nullptr != m_db_handle) {
        SPDLOG_WARN("MySQLDB not closed before being destroyed.");
        close();
    }
}

void MySQLDB::open(
        string const& host,
        int port,
        string const& username,
        string const& password,
        string const& database
) {
    if (nullptr != m_db_handle) {
        throw OperationFailed(ErrorCode_NotReady, __FILENAME__, __LINE__);
    }

    m_db_handle = mysql_init(nullptr);
    if (nullptr == m_db_handle) {
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }

    auto db_handle = mysql_real_connect(
            m_db_handle,
            host.c_str(),
            username.c_str(),
            password.c_str(),
            database.c_str(),
            port,
            nullptr,
            CLIENT_COMPRESS
    );
    if (nullptr == db_handle) {
        SPDLOG_ERROR("MySQLDB: Failed to connect - {}.", mysql_error(m_db_handle));
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }
}

void MySQLDB::close() {
    if (nullptr == m_db_handle) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    mysql_close(m_db_handle);
    m_db_handle = nullptr;
}

bool MySQLDB::execute_query(string const& sql_query) {
    if (nullptr == m_db_handle) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    if (0 != mysql_real_query(m_db_handle, sql_query.c_str(), sql_query.length())) {
        SPDLOG_ERROR(
                "MySQLDB: Query failed - {}. ({})",
                mysql_error(m_db_handle),
                sql_query.c_str()
        );
        return false;
    }

    return true;
}

MySQLPreparedStatement MySQLDB::prepare_statement(char const* statement, size_t statement_length) {
    if (nullptr == m_db_handle) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    auto prepared_statement = MySQLPreparedStatement(m_db_handle);
    prepared_statement.set(statement, statement_length);
    return prepared_statement;
}
}  // namespace clp

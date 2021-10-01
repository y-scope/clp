#ifndef MYSQLDB_HPP
#define MYSQLDB_HPP

// C++ standard libraries
#include <string>

// MySQL
#include <mariadb/mysql.h>

// Project headers
#include "Defs.h"
#include "ErrorCode.hpp"
#include "MySQLParamBindings.hpp"
#include "MySQLPreparedStatement.hpp"
#include "TraceableException.hpp"

/**
 * Class representing a MySQL-style database
 */
class MySQLDB {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

        // Methods
        const char* what () const noexcept override {
            return "MySQLDB operation failed";
        }
    };

    class Iterator {
    public:
        // Types
        class OperationFailed : public TraceableException {
        public:
            // Constructors
            OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

            // Methods
            const char* what () const noexcept override {
                return "MySQLDB::Iterator operation failed";
            }
        };

        // Constructors
        explicit Iterator (MYSQL* m_db_handle);

        // Delete copy constructor and assignment
        Iterator (const Iterator&) = delete;
        Iterator& operator= (const Iterator&) = delete;

        // Move constructor and assignment
        Iterator (Iterator&& rhs) noexcept;
        Iterator& operator= (Iterator&& rhs) noexcept;

        // Destructors
        ~Iterator ();

        // Methods
        bool contains_element () const;
        void get_next ();
        void get_field_as_string (size_t field_ix, std::string& field_value);

    private:
        // Methods
        /**
         * Fetches the next row from the database server
         */
        void fetch_next_row ();

        // Variables
        MYSQL_RES* m_query_result;
        MYSQL_ROW m_row;
        unsigned int m_num_fields;
        unsigned long* m_field_lengths;
    };

    // Constructors
    MySQLDB () : m_db_handle(nullptr) {}

    // Destructor
    ~MySQLDB ();

    // Methods
    /**
     * Opens a connection to the database server
     * @param host
     * @param port
     * @param username
     * @param password
     * @param database
     */
    void open (const std::string& host, int port, const std::string& username, const std::string& password, const std::string& database);
    /**
     * Closes the connection to the database server
     */
    void close ();

    /**
     * Executes a query on the database server
     * @param sql_query
     * @return
     */
    bool execute_query (const std::string& sql_query);
    /**
     * Prepares a statement on the database server
     * @param statement
     * @param statement_length
     * @return
     */
    MySQLPreparedStatement prepare_statement (const char* statement, size_t statement_length);

    Iterator get_iterator () { return Iterator{m_db_handle}; }

private:
    // Variables
    MYSQL* m_db_handle;
};

#endif // MYSQLDB_HPP

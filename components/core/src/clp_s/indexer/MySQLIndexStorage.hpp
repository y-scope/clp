#ifndef CLP_S_INDEXER_MYSQLINDEXSTORAGE_HPP
#define CLP_S_INDEXER_MYSQLINDEXSTORAGE_HPP

#include "../../clp/MySQLDB.hpp"
#include "../../clp/MySQLPreparedStatement.hpp"
#include "../SchemaTree.hpp"
#include "../TraceableException.hpp"

namespace clp_s::indexer {
/**
 * Class representing a MySQL storage for column metadata (column names and types)
 */
class MySQLIndexStorage {
public:
    static constexpr char cColumnMetadataTableSuffix[] = "column_metadata";

    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    // Constructors
    MySQLIndexStorage(
            std::string const& host,
            int port,
            std::string const& username,
            std::string const& password,
            std::string const& database_name,
            std::string const& table_prefix
    )
            : m_is_open(false),
              m_is_init(false),
              m_host(host),
              m_port(port),
              m_username(username),
              m_password(password),
              m_database_name(database_name),
              m_table_prefix(table_prefix) {}

    // Methods
    /**
     * Opens the database connection
     */
    void open();

    /**
     * Creates the table if it is required and prepares the insert statement
     * @param dataset_name
     * @param should_create_table
     */
    void init(std::string const& dataset_name, bool should_create_table);

    /**
     * Closes the database connection
     */
    void close();

    /**
     * Adds a field (column) to the table
     * @param field_name
     * @param field_type
     */
    void add_field(std::string const& field_name, NodeType field_type);

private:
    // Variables
    bool m_is_open{};
    bool m_is_init{};
    std::string m_host;
    int m_port{};
    std::string m_username;
    std::string m_password;
    std::string m_database_name;
    std::string m_table_prefix;

    clp::MySQLDB m_db;

    std::unique_ptr<clp::MySQLPreparedStatement> m_insert_field_statement;
};
}  // namespace clp_s::indexer

#endif  // CLP_S_INDEXER_MYSQLINDEXSTORAGE_HPP

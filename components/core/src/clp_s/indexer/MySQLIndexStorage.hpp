#ifndef CLP_S_INDEXER_MYSQLINDEXSTORAGE_HPP
#define CLP_S_INDEXER_MYSQLINDEXSTORAGE_HPP

#include <memory>
#include <string>
#include <string_view>

#include <ystdlib/error_handling/TraceableException.hpp>

#include "../../clp/MySQLDB.hpp"
#include "../../clp/MySQLPreparedStatement.hpp"
#include "../SchemaTree.hpp"

namespace clp_s::indexer {
/**
 * Class representing a MySQL storage for column metadata (column names and types)
 */
class MySQLIndexStorage {
public:
    YSTDLIB_ERROR_HANDLING_DEFINE_TRACEABLE_EXCEPTION(OperationFailed);

    // Constructor
    MySQLIndexStorage(
            std::string const& host,
            int port,
            std::string const& username,
            std::string const& password,
            std::string const& database_name,
            std::string const& table_prefix,
            std::string const& dataset_name,
            bool should_create_table
    );

    // Delete copy constructor and assignment operator
    MySQLIndexStorage(MySQLIndexStorage const&) = delete;
    auto operator=(MySQLIndexStorage const&) -> MySQLIndexStorage& = delete;

    // Default move constructor and assignment operator
    MySQLIndexStorage(MySQLIndexStorage&&) noexcept = default;
    auto operator=(MySQLIndexStorage&&) noexcept -> MySQLIndexStorage& = default;

    // Destructor
    ~MySQLIndexStorage();

    // Methods
    /**
     * Adds a field (column) to the table
     * @param field_name
     * @param field_type
     */
    auto add_field(std::string const& field_name, NodeType field_type) -> void;

private:
    // Variables
    clp::MySQLDB m_db;
    std::unique_ptr<clp::MySQLPreparedStatement> m_insert_field_statement;
};
}  // namespace clp_s::indexer

#endif  // CLP_S_INDEXER_MYSQLINDEXSTORAGE_HPP

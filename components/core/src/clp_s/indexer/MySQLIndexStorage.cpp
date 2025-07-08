#include "MySQLIndexStorage.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

#include <fmt/core.h>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include "../../clp/database_utils.hpp"
#include "../../clp/MySQLPreparedStatement.hpp"
#include "../../clp/type_utils.hpp"
#include "../SchemaTree.hpp"

namespace {
// Types
enum class ColumnMetadataTableFieldIndexes : uint8_t {
    Name = 0,
    Type,
    Length,
};

// Constants
constexpr std::string_view cColumnMetadataTableSuffix = "column_metadata";
constexpr std::array<std::string_view, static_cast<size_t>(ColumnMetadataTableFieldIndexes::Length)>
        cColumnMetadataTableFieldNames = {"name", "type"};
}  // namespace

namespace clp_s::indexer {
MySQLIndexStorage::MySQLIndexStorage(
        std::string const& host,
        int port,
        std::string const& username,
        std::string const& password,
        std::string const& database_name,
        std::string const& table_prefix,
        std::string const& dataset_name,
        bool should_create_table
) {
    m_db.open(host, port, username, password, database_name);
    auto const table_name{
            fmt::format("{}{}_{}", table_prefix, dataset_name, cColumnMetadataTableSuffix)
    };
    if (should_create_table) {
        m_db.execute_query(
                fmt::format(
                        "CREATE TABLE IF NOT EXISTS {} ("
                        "name VARCHAR(512) NOT NULL, "
                        "type TINYINT NOT NULL,"
                        "PRIMARY KEY (name, type)"
                        ")",
                        table_name
                )
        );
    }

    fmt::memory_buffer statement_buffer;
    auto statement_buffer_ix{std::back_inserter(statement_buffer)};

    fmt::format_to(
            statement_buffer_ix,
            "INSERT IGNORE INTO {} ({}) VALUES ({})",
            table_name,
            clp::get_field_names_sql(
                    std::vector<std::string>{
                            cColumnMetadataTableFieldNames.begin(),
                            cColumnMetadataTableFieldNames.end()
                    }
            ),
            clp::get_placeholders_sql(cColumnMetadataTableFieldNames.size())
    );
    SPDLOG_DEBUG("{:.{}}", statement_buffer.data(), statement_buffer.size());
    m_insert_field_statement = std::make_unique<clp::MySQLPreparedStatement>(
            m_db.prepare_statement(statement_buffer.data(), statement_buffer.size())
    );
}

MySQLIndexStorage::~MySQLIndexStorage() {
    m_db.close();
}

void MySQLIndexStorage::add_field(std::string const& field_name, NodeType field_type) {
    auto& statement_bindings{m_insert_field_statement->get_statement_bindings()};
    statement_bindings.bind_varchar(
            clp::enum_to_underlying_type(ColumnMetadataTableFieldIndexes::Name),
            field_name.c_str(),
            field_name.length()
    );

    auto field_type_value{static_cast<uint8_t>(field_type)};
    statement_bindings.bind_uint8(
            clp::enum_to_underlying_type(ColumnMetadataTableFieldIndexes::Type),
            field_type_value
    );

    if (false == m_insert_field_statement->execute()) {
        // TODO: determine if we need more SQL error granularity
        throw OperationFailed(
                std::make_error_code(std::errc::io_error),
                "SQL insertion statement failed."
        );
    }
}
}  // namespace clp_s::indexer

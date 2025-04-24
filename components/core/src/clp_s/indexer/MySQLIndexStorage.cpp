#include "MySQLIndexStorage.hpp"

#include <fmt/core.h>
#include <spdlog/spdlog.h>

#include "../../clp/database_utils.hpp"
#include "../../clp/type_utils.hpp"

enum class TableMetadataFieldIndexes : uint16_t {
    Name = 0,
    Type,
    Length,
};

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

    auto const table_name
            = fmt::format("{}{}_{}", table_prefix, dataset_name, cColumnMetadataSuffix);
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

    m_insert_field_statement.reset();

    std::vector<std::string> table_metadata_field_names(
            clp::enum_to_underlying_type(TableMetadataFieldIndexes::Length)
    );
    table_metadata_field_names[clp::enum_to_underlying_type(TableMetadataFieldIndexes::Name)]
            = "name";
    table_metadata_field_names[clp::enum_to_underlying_type(TableMetadataFieldIndexes::Type)]
            = "type";
    fmt::memory_buffer statement_buffer;
    auto statement_buffer_ix = std::back_inserter(statement_buffer);

    fmt::format_to(
            statement_buffer_ix,
            "INSERT IGNORE INTO {} ({}) VALUES ({})",
            table_name,
            clp::get_field_names_sql(table_metadata_field_names),
            clp::get_placeholders_sql(table_metadata_field_names.size())
    );
    SPDLOG_DEBUG("{:.{}}", statement_buffer.data(), statement_buffer.size());
    m_insert_field_statement = std::make_unique<clp::MySQLPreparedStatement>(
            m_db.prepare_statement(statement_buffer.data(), statement_buffer.size())
    );
}

void MySQLIndexStorage::~MySQLIndexStorage() {
    m_insert_field_statement.reset();
    m_db.close();
}

auto MySQLIndexStorage::add_field(std::string const& field_name, NodeType field_type) -> void {
    auto& statement_bindings = m_insert_field_statement->get_statement_bindings();
    statement_bindings.bind_varchar(
            clp::enum_to_underlying_type(TableMetadataFieldIndexes::Name),
            field_name.c_str(),
            field_name.length()
    );

    auto field_type_value = static_cast<uint8_t>(field_type);
    statement_bindings.bind_uint8(
            clp::enum_to_underlying_type(TableMetadataFieldIndexes::Type),
            field_type_value
    );

    if (false == m_insert_field_statement->execute()) {
        throw OperationFailed(ErrorCodeFailure, __FILENAME__, __LINE__);
    }
}
}  // namespace clp_s::indexer

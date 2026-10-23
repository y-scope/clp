#include "MySQLIndexStorage.hpp"

#include <fmt/base.h>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include "../../clp/database_utils.hpp"
#include "../../clp/type_utils.hpp"

enum class InsertFieldStatementPlaceholderIndexes : uint16_t {
    Name = 0,
    Type,
    DatasetName,
    Length,
};

namespace clp_s::indexer {
void MySQLIndexStorage::open() {
    if (m_is_open) {
        throw OperationFailed(ErrorCodeNotReady, __FILENAME__, __LINE__);
    }

    m_db.open(m_host, m_port, m_username, m_password, m_database_name);
    m_is_open = true;
}

void MySQLIndexStorage::init(std::string const& dataset_name) {
    if (false == m_is_open) {
        throw OperationFailed(ErrorCodeNotReady, __FILENAME__, __LINE__);
    }

    auto const column_metadata_table_name{
            fmt::format("{}{}", m_table_prefix, cColumnMetadataTableSuffix)
    };
    auto const datasets_table_name{fmt::format("{}{}", m_table_prefix, cDatasetsTableSuffix)};

    m_insert_field_statement.reset();

    std::vector<std::string> insert_field_names(
            clp::enum_to_underlying_type(InsertFieldStatementPlaceholderIndexes::Length)
    );
    insert_field_names[clp::enum_to_underlying_type(InsertFieldStatementPlaceholderIndexes::Name)]
            = "name";
    insert_field_names[clp::enum_to_underlying_type(InsertFieldStatementPlaceholderIndexes::Type)]
            = "type";
    insert_field_names[clp::enum_to_underlying_type(
            InsertFieldStatementPlaceholderIndexes::DatasetName
    )] = "dataset_id";
    fmt::memory_buffer statement_buffer;
    auto statement_buffer_ix = std::back_inserter(statement_buffer);

    constexpr auto cNumSelectedPlaceholders{
            clp::enum_to_underlying_type(InsertFieldStatementPlaceholderIndexes::Length) - 1
    };
    fmt::format_to(
            statement_buffer_ix,
            "INSERT IGNORE INTO {} ({}) SELECT {},id FROM {} WHERE name = ?",
            column_metadata_table_name,
            clp::get_field_names_sql(insert_field_names),
            clp::get_placeholders_sql(cNumSelectedPlaceholders),
            datasets_table_name
    );
    m_insert_field_statement = std::make_unique<clp::MySQLPreparedStatement>(
            m_db.prepare_statement(statement_buffer.data(), statement_buffer.size())
    );

    m_dataset_name = dataset_name;
    m_insert_field_statement->get_statement_bindings().bind_varchar(
            clp::enum_to_underlying_type(InsertFieldStatementPlaceholderIndexes::DatasetName),
            m_dataset_name.c_str(),
            m_dataset_name.length()
    );

    m_is_init = true;
}

void MySQLIndexStorage::add_field(std::string const& field_name, NodeType field_type) {
    if (false == m_is_init) {
        throw OperationFailed(ErrorCodeNotReady, __FILENAME__, __LINE__);
    }

    auto& statement_bindings = m_insert_field_statement->get_statement_bindings();
    statement_bindings.bind_varchar(
            clp::enum_to_underlying_type(InsertFieldStatementPlaceholderIndexes::Name),
            field_name.c_str(),
            field_name.length()
    );

    auto field_type_value = static_cast<uint8_t>(field_type);
    statement_bindings.bind_uint8(
            clp::enum_to_underlying_type(InsertFieldStatementPlaceholderIndexes::Type),
            field_type_value
    );

    if (false == m_insert_field_statement->execute()) {
        throw OperationFailed(ErrorCodeFailure, __FILENAME__, __LINE__);
    }
}

void MySQLIndexStorage::close() {
    m_insert_field_statement.reset();
    m_db.close();
    m_is_open = false;
    m_is_init = false;
}
}  // namespace clp_s::indexer

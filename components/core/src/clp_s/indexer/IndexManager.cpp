#include "IndexManager.hpp"

#include <filesystem>
#include <stack>

#include "../archive_constants.hpp"

namespace clp_s::indexer {
IndexManager::IndexManager(
        std::optional<clp::GlobalMetadataDBConfig> const& db_config,
        bool should_create_table
) {
    if (db_config.has_value()) {
        m_mysql_index_storage = std::make_unique<MySQLIndexStorage>(
                db_config->get_metadata_db_host(),
                db_config->get_metadata_db_port(),
                db_config->get_metadata_db_username(),
                db_config->get_metadata_db_password(),
                db_config->get_metadata_db_name(),
                db_config->get_metadata_table_prefix()
        );
        m_mysql_index_storage->open();
        m_field_update_callback = [this](std::string& field_name, NodeType field_type) {
            m_mysql_index_storage->add_field(field_name, field_type);
        };
        m_should_create_table = should_create_table;
        m_output_type = OutputType::Database;
    } else {
        throw OperationFailed(ErrorCodeBadParam, __FILENAME__, __LINE__);
    }
}

IndexManager::~IndexManager() {
    if (m_output_type == OutputType::Database) {
        m_mysql_index_storage->close();
    }
}

void IndexManager::update_metadata(std::string const& table_name, Path const& archive_path) {
    m_mysql_index_storage->init(table_name, m_should_create_table);

    ArchiveReader archive_reader;
    archive_reader.open(archive_path, NetworkAuthOption{});

    traverse_schema_tree_and_update_metadata(archive_reader.get_schema_tree());
}

std::string IndexManager::escape_key_name(std::string_view const key_name) {
    std::string escaped_key_name;
    escaped_key_name.reserve(key_name.size());
    for (auto c : key_name) {
        switch (c) {
            case '\"':
                escaped_key_name += "\\\"";
                break;
            case '\\':
                escaped_key_name += "\\\\";
                break;
            case '\n':
                escaped_key_name += "\\n";
                break;
            case '\t':
                escaped_key_name += "\\t";
                break;
            case '\r':
                escaped_key_name += "\\r";
                break;
            case '\b':
                escaped_key_name += "\\b";
                break;
            case '\f':
                escaped_key_name += "\\f";
                break;
            case '.':
                escaped_key_name += "\\.";
                break;
            default:
                if (std::isprint(c)) {
                    escaped_key_name += c;
                } else {
                    char buffer[7];
                    std::snprintf(
                            buffer,
                            sizeof(buffer),
                            "\\u00%02x",
                            static_cast<unsigned char>(c)
                    );
                    escaped_key_name += buffer;
                }
        }
    }
    return escaped_key_name;
}

void IndexManager::traverse_schema_tree_and_update_metadata(
        std::shared_ptr<SchemaTree> const& schema_tree
) {
    if (nullptr == schema_tree) {
        return;
    }

    std::string path_buffer;
    // Stack of pairs of node_id and path_length
    std::stack<std::pair<int32_t, uint64_t>> s;
    s.emplace(schema_tree->get_object_subtree_node_id(), 0);

    while (false == s.empty()) {
        auto [node_id, path_length] = s.top();
        s.pop();

        auto const& node = schema_tree->get_node(node_id);
        auto node_type = node.get_type();
        // TODO: Add support for structured arrays
        if (NodeType::StructuredArray == node_type) {
            continue;
        }
        auto const& children_ids = node.get_children_ids();
        path_buffer.resize(path_length);
        if (false == path_buffer.empty()) {
            path_buffer += ".";
        }
        path_buffer += escape_key_name(node.get_key_name());
        if (children_ids.empty() && NodeType::Object != node_type && NodeType::Unknown != node_type)
        {
            m_field_update_callback(path_buffer, node_type);
        }

        for (auto child_id : children_ids) {
            s.emplace(child_id, path_buffer.size());
        }
    }
}
}  // namespace clp_s::indexer

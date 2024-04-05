#include "SchemaTree.hpp"

#include "archive_constants.hpp"
#include "FileWriter.hpp"
#include "ZstdCompressor.hpp"

namespace clp_s {
int32_t SchemaTree::add_node(int32_t parent_node_id, NodeType type, std::string const& key) {
    std::tuple<int32_t, std::string, NodeType> node_key = {parent_node_id, key, type};
    auto node_it = m_node_map.find(node_key);
    if (node_it != m_node_map.end()) {
        auto node_id = node_it->second;
        m_nodes[node_id]->increase_count();
        return node_id;
    }

    auto node = std::make_shared<SchemaNode>(parent_node_id, m_nodes.size(), key, type, 0);
    node->increase_count();
    m_nodes.push_back(node);
    int32_t node_id = node->get_id();
    if (parent_node_id >= 0) {
        auto parent_node = m_nodes[parent_node_id];
        node->set_depth(parent_node->get_depth() + 1);
        parent_node->add_child(node_id);
    }
    m_node_map[node_key] = node_id;

    return node_id;
}

size_t SchemaTree::store(std::string const& archives_dir, int compression_level) {
    FileWriter schema_tree_writer;
    ZstdCompressor schema_tree_compressor;

    schema_tree_writer.open(
            archives_dir + constants::cArchiveSchemaTreeFile,
            FileWriter::OpenMode::CreateForWriting
    );
    schema_tree_compressor.open(schema_tree_writer, compression_level);

    schema_tree_compressor.write_numeric_value(m_nodes.size());
    for (auto const& node : m_nodes) {
        schema_tree_compressor.write_numeric_value(node->get_parent_id());

        std::string const& key = node->get_key_name();
        schema_tree_compressor.write_numeric_value(key.size());
        schema_tree_compressor.write_string(key);
        schema_tree_compressor.write_numeric_value(node->get_type());
    }

    schema_tree_compressor.close();
    size_t compressed_size = schema_tree_writer.get_pos();
    schema_tree_writer.close();
    return compressed_size;
}
}  // namespace clp_s

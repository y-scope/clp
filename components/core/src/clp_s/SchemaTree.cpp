#include "SchemaTree.hpp"

namespace clp_s {
int32_t SchemaTree::add_node(int32_t parent_node_id, NodeType type, std::string const& key) {
    std::tuple<int32_t, std::string, NodeType> node_key = {parent_node_id, key, type};
    auto node_it = m_node_map.find(node_key);
    if (node_it != m_node_map.end()) {
        auto node_id = node_it->second;
        m_nodes[node_id]->increase_count();
        return node_id;
    }

    auto node = std::make_shared<SchemaNode>(parent_node_id, m_nodes.size(), key, type);
    node->increase_count();
    m_nodes.push_back(node);
    int32_t node_id = node->get_id();
    if (parent_node_id >= 0) {
        auto parent_node = m_nodes[parent_node_id];
        parent_node->add_child(node_id);
    }
    m_node_map[node_key] = node_id;

    return node_id;
}
}  // namespace clp_s

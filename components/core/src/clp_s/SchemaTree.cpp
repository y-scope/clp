#include "SchemaTree.hpp"

#include <stack>

namespace clp_s {
void SchemaNode::mark_node_value(uint64_t value, std::string const& string_value) {
    switch (m_value_state) {
        case NodeValueState::UNINITIALIZED:
            m_value = value;
            m_string_value = string_value;
            m_value_state = NodeValueState::CARDINALITY_ONE;
            break;
        case NodeValueState::CARDINALITY_ONE:
            if (m_value != value) {
                m_value_state = NodeValueState::CARDINALITY_MANY;
            }
            break;
        default:
            break;
    }
}

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

int32_t SchemaTree::add_node_decompression(
        int node_id,
        int parent_node_id,
        NodeType type,
        std::string const& key
) {
    auto node = std::make_shared<SchemaNode>(parent_node_id, node_id, key, type);
    m_nodes.at(node_id) = node;
    if (parent_node_id >= 0) {
        m_nodes.at(parent_node_id)->add_child(node_id);
    }

    return node->get_id();
}

std::vector<std::pair<int32_t, int32_t>> SchemaTree::modify_nodes_based_on_frequency(
        size_t num_records
) {
    int32_t root_node_id = get_root_node_id();
    auto root_node = get_node(root_node_id);

    std::stack<std::vector<int32_t>> vector_stack;
    std::stack<std::vector<int32_t>::const_iterator> it_stack;
    std::stack<std::vector<int32_t>::const_iterator> it_end_stack;

    vector_stack.push(root_node->get_children_ids());
    auto it = vector_stack.top().cbegin();
    auto it_end = vector_stack.top().cend();

    std::vector<std::pair<int32_t, int32_t>> updates;

    size_t lower_bound = static_cast<size_t>(num_records * 0.01);

    int32_t parent_replacement_id = -1;

    for (; it != it_end || it_stack.size() > 0;) {
        if (it == it_end) {
            it = it_stack.top();
            it_stack.pop();
            it_end = it_end_stack.top();
            it_end_stack.pop();
            vector_stack.pop();
            continue;
        }

        auto node = get_node(*it);

        // FIXME: this state machine probably doesn't always work during archive splitting.
        // should rewrite to carefully handle all edge cases
        // however we're pretty unlikely to run into those edge cases for now, so it should be fine
        if (node->get_count() <= lower_bound && node->get_type() != NodeType::TRUNCATEDCHILDREN
            && node->get_type() != NodeType::TRUNCATEDOBJECT
            && node->get_type() != NodeType::VARVALUE)
        {
            node->set_state(NodeValueState::TRUNCATED);
            if (this->get_node(node->get_parent_id())->get_state() == NodeValueState::TRUNCATED
                && node->get_type() != NodeType::OBJECT)
            {
                updates.push_back({*it, parent_replacement_id});
            } else if (this->get_node(node->get_parent_id())->get_state() != NodeValueState::TRUNCATED && node->get_type() != NodeType::OBJECT)
            {
                auto parent_node = this->get_node(node->get_parent_id());
                parent_replacement_id
                        = add_node(parent_node->get_id(), NodeType::TRUNCATEDCHILDREN, "0");
                updates.push_back({*it, parent_replacement_id});
            } else if (this->get_node(node->get_parent_id())->get_state() != NodeValueState::TRUNCATED && node->get_type() == NodeType::OBJECT)
            {
                parent_replacement_id = add_node(
                        node->get_parent_id(),
                        NodeType::TRUNCATEDOBJECT,
                        node->get_key_name()
                );
                updates.push_back({*it, parent_replacement_id});
            } else if (this->get_node(node->get_parent_id())->get_state() == NodeValueState::TRUNCATED && node->get_type() == NodeType::OBJECT)
            {
                updates.push_back({*it, parent_replacement_id});
            }
        } else if (node->get_state() == NodeValueState::CARDINALITY_ONE) {
            int32_t var_node_id = add_node(*it, NodeType::VARVALUE, node->get_string_var_value());
            updates.push_back({*it, var_node_id});
            ++it;
            continue;
        }

        ++it;
        vector_stack.push(node->get_children_ids());
        if (vector_stack.top().size() > 0) {
            it_stack.push(it);
            it_end_stack.push(it_end);
            it = vector_stack.top().cbegin();
            it_end = vector_stack.top().cend();
        } else {
            vector_stack.pop();
        }
    }

    return updates;
}

}  // namespace clp_s

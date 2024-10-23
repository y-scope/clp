#include "SchemaTree.hpp"

#include <cstddef>
#include <optional>
#include <string>

#include "../ErrorCode.hpp"

namespace clp::ffi {
auto SchemaTree::get_node(Node::id_t id) const -> Node const& {
    if (m_tree_nodes.size() <= static_cast<size_t>(id)) {
        throw OperationFailed(
                ErrorCode_OutOfBounds,
                __FILE__,
                __LINE__,
                "Invalid node ID: " + std::to_string(id)
        );
    }
    return m_tree_nodes[id];
}

auto SchemaTree::try_get_node_id(NodeLocator const& locator) const -> std::optional<Node::id_t> {
    auto const parent_id{static_cast<size_t>(locator.get_parent_id())};
    if (m_tree_nodes.size() <= parent_id) {
        return std::nullopt;
    }
    std::optional<Node::id_t> node_id;
    for (auto const child_id : m_tree_nodes[parent_id].get_children_ids()) {
        auto const& node{m_tree_nodes[child_id]};
        if (node.get_key_name() == locator.get_key_name() && node.get_type() == locator.get_type())
        {
            node_id.emplace(child_id);
            break;
        }
    }
    return node_id;
}

auto SchemaTree::insert_node(NodeLocator const& locator) -> Node::id_t {
    if (try_get_node_id(locator).has_value()) {
        throw OperationFailed(ErrorCode_Failure, __FILE__, __LINE__, "Node already exists.");
    }
    auto const node_id{static_cast<Node::id_t>(m_tree_nodes.size())};
    m_tree_nodes.emplace_back(Node::create(node_id, locator));
    auto& parent_node{m_tree_nodes[locator.get_parent_id()]};
    if (Node::Type::Obj != parent_node.get_type()) {
        throw OperationFailed(
                ErrorCode_Failure,
                __FILE__,
                __LINE__,
                "Non-object nodes cannot have children."
        );
    }
    parent_node.append_new_child(node_id);
    return node_id;
}

auto SchemaTree::revert() -> void {
    if (false == m_snapshot_size.has_value()) {
        throw OperationFailed(ErrorCode_Failure, __FILE__, __LINE__, "No snapshot exists.");
    }
    while (m_tree_nodes.size() != m_snapshot_size) {
        auto const& node{m_tree_nodes.back()};
        auto const optional_parent_id{node.get_parent_id()};
        if (optional_parent_id.has_value()) {
            m_tree_nodes[optional_parent_id.value()].remove_last_appended_child();
        }
        m_tree_nodes.pop_back();
    }
    m_snapshot_size.reset();
}
}  // namespace clp::ffi

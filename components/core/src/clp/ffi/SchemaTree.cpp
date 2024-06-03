#include "SchemaTree.hpp"

#include <cstddef>
#include <optional>
#include <string>

#include "../ErrorCode.hpp"
#include "SchemaTreeNode.hpp"

namespace clp::ffi {
auto SchemaTree::get_node(SchemaTreeNode::id_t id) const -> SchemaTreeNode const& {
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

auto SchemaTree::try_get_node_id(NodeLocator const& locator
) const -> std::optional<SchemaTreeNode::id_t> {
    auto const parent_id{static_cast<size_t>(locator.get_parent_id())};
    if (m_tree_nodes.size() <= parent_id) {
        return false;
    }
    std::optional<SchemaTreeNode::id_t> node_id;
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

auto SchemaTree::insert_node(NodeLocator const& locator) -> SchemaTreeNode::id_t {
    if (try_get_node_id(locator).has_value()) {
        throw OperationFailed(ErrorCode_Failure, __FILE__, __LINE__, "Node already exists.");
    }
    auto const node_id{static_cast<SchemaTreeNode::id_t>(m_tree_nodes.size())};
    m_tree_nodes.emplace_back(
            node_id,
            locator.get_parent_id(),
            locator.get_key_name(),
            locator.get_type()
    );
    auto& parent_node{m_tree_nodes[locator.get_parent_id()]};
    if (SchemaTreeNode::Type::Obj != parent_node.get_type()) {
        throw OperationFailed(
                ErrorCode_Failure,
                __FILE__,
                __LINE__,
                "Parent node cannot have child."
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
        m_tree_nodes[node.get_parent_id()].remove_last_appended_child();
        m_tree_nodes.pop_back();
    }
    m_snapshot_size.reset();
}
}  // namespace clp::ffi

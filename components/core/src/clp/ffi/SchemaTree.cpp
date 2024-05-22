#include "SchemaTree.hpp"

#include <cstddef>
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
                "The given tree node id is invalid: " + std::to_string(id)
        );
    }
    return m_tree_nodes[id];
}

auto SchemaTree::try_get_node_id(
        TreeNodeLocator const& locator,
        SchemaTreeNode::id_t& node_id
) const -> bool {
    auto const parent_id{static_cast<size_t>(locator.get_parent_id())};
    if (m_tree_nodes.size() <= parent_id) {
        return false;
    }
    for (auto const child_id : m_tree_nodes[parent_id].get_children_ids()) {
        auto const& node{m_tree_nodes[child_id]};
        if (node.get_key_name() == locator.get_key_name() && node.get_type() == locator.get_type())
        {
            node_id = child_id;
            return true;
        }
    }
    return false;
}

auto SchemaTree::insert_node(TreeNodeLocator const& locator) -> SchemaTreeNode::id_t {
    SchemaTreeNode::id_t node_id{};
    if (try_get_node_id(locator, node_id)) {
        throw OperationFailed(ErrorCode_Failure, __FILE__, __LINE__, "Tree Node already exists.");
    }
    node_id = m_tree_nodes.size();
    m_tree_nodes.emplace_back(
            node_id,
            locator.get_parent_id(),
            locator.get_key_name(),
            locator.get_type()
    );
    m_tree_nodes[locator.get_parent_id()].append_new_child_id(node_id);
    return node_id;
}

auto SchemaTree::revert() -> void {
    if (false == m_snapshot_size.has_value()) {
        throw OperationFailed(
                ErrorCode_Failure,
                __FILE__,
                __LINE__,
                "Snapshot was not taken before calling revert."
        );
    }
    while (m_tree_nodes.size() != m_snapshot_size) {
        auto const& node{m_tree_nodes.back()};
        m_tree_nodes[node.get_parent_id()].remove_last_appended_child_id();
        m_tree_nodes.pop_back();
    }
    m_snapshot_size.reset();
}
}  // namespace clp::ffi

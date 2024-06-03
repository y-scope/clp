#ifndef CLP_FFI_SCHEMATREE_HPP
#define CLP_FFI_SCHEMATREE_HPP

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "../ErrorCode.hpp"
#include "../TraceableException.hpp"
#include "SchemaTreeNode.hpp"

namespace clp::ffi {
/**
 * This class implements a schema tree, providing all necessary methods for upper-layer APIs through
 * FFI. Nodes are stored using a flattened vector.
 */
class SchemaTree {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        OperationFailed(
                ErrorCode error_code,
                char const* const filename,
                int line_number,
                std::string message
        )
                : TraceableException{error_code, filename, line_number},
                  m_message{std::move(message)} {}

        [[nodiscard]] auto what() const noexcept -> char const* override {
            return m_message.c_str();
        }

    private:
        std::string m_message;
    };

    /**
     * When constructing the schema tree, we uniquely identify the location of a node being inserted
     * to the tree by the unique triple of the parent id, the key name, and the node type. The
     * reason why the triple is unique is because the combination of the key name and the node type
     * should have no ambiguity for a parent node. This class stores such a triple and act as a
     * unique identifier for a node in the schema tree.
     */
    class TreeNodeLocator {
    public:
        TreeNodeLocator(
                SchemaTreeNode::id_t parent_id,
                std::string_view key_name,
                SchemaTreeNode::Type type
        )
                : m_parent_id{parent_id},
                  m_key_name{key_name},
                  m_type{type} {}

        [[nodiscard]] auto get_parent_id() const -> SchemaTreeNode::id_t { return m_parent_id; }

        [[nodiscard]] auto get_key_name() const -> std::string_view { return m_key_name; }

        [[nodiscard]] auto get_type() const -> SchemaTreeNode::Type { return m_type; }

    private:
        SchemaTreeNode::id_t m_parent_id;
        std::string_view m_key_name;
        SchemaTreeNode::Type m_type;
    };

    // Constants
    static constexpr SchemaTreeNode::id_t cRootId{0};

    // Constructors
    SchemaTree() { m_tree_nodes.emplace_back(cRootId, cRootId, "", SchemaTreeNode::Type::Obj); }

    // Disable copy constructor/assignment operator
    SchemaTree(SchemaTree const&) = delete;
    auto operator=(SchemaTree const&) -> SchemaTree& = delete;

    // Define default move constructor/assignment operator
    SchemaTree(SchemaTree&&) = default;
    auto operator=(SchemaTree&&) -> SchemaTree& = default;

    // Destructor
    ~SchemaTree() = default;

    // Methods
    [[nodiscard]] auto get_size() const -> size_t { return m_tree_nodes.size(); }

    /**
     * @param id
     * @return The tree node with the given id.
     * @throw OperationFailed if the given id is not valid (i.e., out of bound).
     */
    [[nodiscard]] auto get_node(SchemaTreeNode::id_t id) const -> SchemaTreeNode const&;

    /**
     * Tries to get a node id with the provided locator if the node exists.
     * @param locator Locator of a unique tree node.
     * @param node_id Returns the node id if the node exists.
     * @return Whether the node exists.
     */
    [[nodiscard]] auto
    try_get_node_id(TreeNodeLocator const& locator, SchemaTreeNode::id_t& node_id) const -> bool;

    /**
     * Checks whether there exists a node with the given locator.
     * @param locator
     * @return Whether the node exists.
     */
    [[nodiscard]] auto has_node(TreeNodeLocator const& locator) const -> bool {
        SchemaTreeNode::id_t node_id{};
        return try_get_node_id(locator, node_id);
    }

    /**
     * Inserts a new node to the given locator.
     * @param locator
     * @return The node id of the inserted node.
     * @throw OperationFailed if the node with the given locator already exists.
     */
    [[maybe_unused]] auto insert_node(TreeNodeLocator const& locator) -> SchemaTreeNode::id_t;

    /**
     * Takes a snapshot of the current schema tree for potential recovery on failure.
     */
    auto take_snapshot() -> void { m_snapshot_size.emplace(m_tree_nodes.size()); }

    /**
     * Reverts the tree to the time when the snapshot was taken.
     * @throw OperationFailed if the snapshot was not taken.
     */
    auto revert() -> void;

    /**
     * Resets the schema tree by removing all the tree nodes except root.
     */
    auto reset() -> void {
        m_snapshot_size.reset();
        m_tree_nodes.clear();
        m_tree_nodes.emplace_back(cRootId, cRootId, "", SchemaTreeNode::Type::Obj);
    }

private:
    std::optional<size_t> m_snapshot_size{std::nullopt};
    std::vector<SchemaTreeNode> m_tree_nodes;
};
}  // namespace clp::ffi
#endif

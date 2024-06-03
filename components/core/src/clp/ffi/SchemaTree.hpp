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
 * A tree representing the schema of a set of JSON (or any format with a dynamic structure) records.
 * Each node in a SchemaTree has a key name and a value type, corresponding to the key name and
 * value type of a key-value pair (kv-pair) in a record. A nested kv-pair is represented as a child
 * node of the outer kv-pair. As a result, all leaf nodes represent primitive values while internal
 * nodes represent non-primitive values.
 *
 * NOTE:
 * - The tree is the representation of multiple record's schemas merged together, so technically, it
 *   should be called a MergedSchemaTree; we use SchemaTree for simplicity.
 * - The root node is special with key name "" and type `SchemaTreeNode::Type::Obj`. This means that
 *   this SchemaTree implementation cannot represent records that are not objects.
 * - This SchemaTree implementation does not represent arrays. Instead, arrays in a record can be
 *   serialized as strings and represented using the type `SchemaTreeNode::Type::UnstructuredArray`.
 *
 * For example, consider the two records below.
 *
 * # Record 1
 * {
 *   "a": 0,
 *   "b": {
 *     "c": true
 *   }
 * }
 *
 * # Record 2
 * {
 *   "a": 0.0,
 *   "d": {
 *     "e": "E"
 *   }
 * }
 *
 * The schema trees for each record are below.
 *
 * # Record 1's schema tree
 * <: Obj> --> <a: Int>
 *         --> <b: Obj>   --> <c: Bool>
 *
 * # Record 2's schema tree
 * <: Obj> --> <a: Float>
 *         --> <d: Obj>   --> <e: Str>
 *
 * In the diagrams above, a node is represented by <KeyName: Type> and a parent-child relationship
 * is represented using "-->".
 *
 * When the records' schema trees are merged together, they create the following SchemaTree:
 *
 * <: Obj> --> <a: Int>
 *         --> <a: Float>
 *         --> <b: Obj>   --> <c: Bool>
 *         --> <d: Obj>   --> <e: Str>
 *
 * Notice that nodes with the same key name, type, and parents are merged together. Nodes that
 * differ in just their key name (e.g., "a") remain unique.
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
     * A triple---parent ID, key name, and node type---that uniquely identifies a node.
     * NOTE: We use the term "Locator" to avoid terms like "Key" or "Identifier" that are already in
     * use.
     */
    class NodeLocator {
    public:
        NodeLocator(
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
     * @return The tree node with the given ID.
     * @throw OperationFailed if a node with the given ID doesn't exist in the tree.
     */
    [[nodiscard]] auto get_node(SchemaTreeNode::id_t id) const -> SchemaTreeNode const&;

    /**
     * Tries to get the ID of a node corresponding to the given locator, if the node exists.
     * @param locator
     * @return Tree node ID if the node exists.
     * @return std::nullopt is the node doesn't exist.
     */
    [[nodiscard]] auto try_get_node_id(NodeLocator const& locator
    ) const -> std::optional<SchemaTreeNode::id_t>;

    /**
     * @param locator
     * @return Whether there is a node that corresponds to the given locator.
     */
    [[nodiscard]] auto has_node(NodeLocator const& locator) const -> bool {
        return try_get_node_id(locator).has_value();
    }

    /**
     * Inserts a new node corresponding to the given locator.
     * @param locator
     * @return The ID of the inserted node.
     * @throw OperationFailed if a node that corresponds to the given locator already exists.
     */
    [[maybe_unused]] auto insert_node(NodeLocator const& locator) -> SchemaTreeNode::id_t;

    /**
     * Takes a snapshot of the current schema tree (to allow recovery on failure).
     */
    auto take_snapshot() -> void { m_snapshot_size.emplace(m_tree_nodes.size()); }

    /**
     * Reverts the tree to the last snapshot.
     * @throw OperationFailed if no snapshot exists.
     */
    auto revert() -> void;

    /**
     * Resets the schema tree by removing all nodes except the root.
     */
    auto reset() -> void {
        m_snapshot_size.reset();
        m_tree_nodes.clear();
        m_tree_nodes.emplace_back(cRootId, cRootId, "", SchemaTreeNode::Type::Obj);
    }

private:
    std::optional<size_t> m_snapshot_size;
    std::vector<SchemaTreeNode> m_tree_nodes;
};
}  // namespace clp::ffi
#endif

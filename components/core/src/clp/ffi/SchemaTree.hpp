#ifndef CLP_FFI_SCHEMATREE_HPP
#define CLP_FFI_SCHEMATREE_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "../ErrorCode.hpp"
#include "../TraceableException.hpp"

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

    // Forward declared types
    class Node;
    class NodeLocator;

    /**
     * A node in clp::ffi::SchemaTree. It stores the node's key name, type, parent's ID, and the IDs
     * of all its children.
     */
    class Node {
    public:
        // Types
        using id_t = uint32_t;

        /**
         * Enum defining the possible node types.
         */
        enum class Type : uint8_t {
            Int = 0,
            Float,
            Bool,
            Str,
            UnstructuredArray,
            Obj
        };

        // Disable copy constructor/assignment operator
        Node(Node const&) = delete;
        auto operator=(Node const&) -> Node& = delete;

        // Define default move constructor/assignment operator
        Node(Node&&) = default;
        auto operator=(Node&&) -> Node& = default;

        // Destructor
        ~Node() = default;

        // Methods
        [[nodiscard]] auto operator==(Node const& rhs) const -> bool = default;

        [[nodiscard]] auto get_id() const -> id_t { return m_id; }

        [[nodiscard]] auto is_root() const -> bool { return false == m_parent_id.has_value(); }

        /**
         * @return The ID of the parent node in the schema tree, if the node is not the root.
         * @return std::nullopt if the node is the root.
         */
        [[nodiscard]] auto get_parent_id() const -> std::optional<id_t> { return m_parent_id; }

        /**
         * Gets the parent ID without checking if it's `std::nullopt`.
         * NOTE: This method should only be used if the caller has checked the node is not the root.
         * @return The ID of the parent node in the schema tree.
         */
        [[nodiscard]] auto get_parent_id_unsafe() const -> id_t {
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            return m_parent_id.value();
        }

        [[nodiscard]] auto get_key_name() const -> std::string_view { return m_key_name; }

        [[nodiscard]] auto get_type() const -> Type { return m_type; }

        [[nodiscard]] auto get_children_ids() const -> std::vector<id_t> const& {
            return m_children_ids;
        }

        /**
         * Appends a child using its node ID.
         * NOTE: This method doesn't check if a child with the given ID already exists.
         * @param child_id The child node's ID.
         */
        auto append_new_child(id_t child_id) -> void { m_children_ids.push_back(child_id); }

        /**
         * Removes the last appended child ID (if any).
         */
        auto remove_last_appended_child() -> void {
            if (m_children_ids.empty()) {
                return;
            }
            m_children_ids.pop_back();
        }

    private:
        friend SchemaTree;

        // Factory functions
        /**
         * Creates a non-root tree node.
         * @param id
         * @param locator
         */
        [[nodiscard]] static auto create(id_t id, NodeLocator const& locator) -> Node {
            return {id, locator.get_parent_id(), locator.get_key_name(), locator.get_type()};
        }

        /**
         * Creates a root node.
         */
        [[nodiscard]] static auto create_root() -> Node {
            return {cRootId, std::nullopt, {}, Type::Obj};
        }

        // Constructors
        Node(id_t id, std::optional<id_t> parent_id, std::string_view key_name, Type type)
                : m_id{id},
                  m_parent_id{parent_id},
                  m_key_name{key_name.begin(), key_name.end()},
                  m_type{type} {}

        id_t m_id;
        std::optional<id_t> m_parent_id;
        std::vector<id_t> m_children_ids;
        std::string m_key_name;
        Type m_type;
    };

    /**
     * A triple---parent ID, key name, and node type---that uniquely identifies a node.
     * NOTE: We use the term "Locator" to avoid terms like "Key" or "Identifier" that are already in
     * use.
     */
    class NodeLocator {
    public:
        NodeLocator(Node::id_t parent_id, std::string_view key_name, Node::Type type)
                : m_parent_id{parent_id},
                  m_key_name{key_name},
                  m_type{type} {}

        [[nodiscard]] auto get_parent_id() const -> Node::id_t { return m_parent_id; }

        [[nodiscard]] auto get_key_name() const -> std::string_view { return m_key_name; }

        [[nodiscard]] auto get_type() const -> Node::Type { return m_type; }

    private:
        Node::id_t m_parent_id;
        std::string_view m_key_name;
        Node::Type m_type;
    };

    // Constants
    static constexpr Node::id_t cRootId{0};

    // Constructors
    SchemaTree() { m_tree_nodes.emplace_back(Node::create_root()); }

    // Disable copy constructor/assignment operator
    SchemaTree(SchemaTree const&) = delete;
    auto operator=(SchemaTree const&) -> SchemaTree& = delete;

    // Define default move constructor/assignment operator
    SchemaTree(SchemaTree&&) = default;
    auto operator=(SchemaTree&&) -> SchemaTree& = default;

    // Destructor
    ~SchemaTree() = default;

    // Methods
    [[nodiscard]] auto operator==(SchemaTree const& rhs) const -> bool {
        return m_tree_nodes == rhs.m_tree_nodes;
    }

    [[nodiscard]] auto get_size() const -> size_t { return m_tree_nodes.size(); }

    [[nodiscard]] auto get_root() const -> Node const& { return m_tree_nodes[cRootId]; }

    /**
     * @param id
     * @return The node with the given ID.
     * @throw OperationFailed if a node with the given ID doesn't exist in the tree.
     */
    [[nodiscard]] auto get_node(Node::id_t id) const -> Node const&;

    /**
     * Tries to get the ID of a node corresponding to the given locator, if the node exists.
     * @param locator
     * @return The node's ID if it exists.
     * @return std::nullopt otherwise.
     */
    [[nodiscard]] auto try_get_node_id(NodeLocator const& locator) const
            -> std::optional<Node::id_t>;

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
     * @throw OperationFailed if:
     * - a node that corresponds to the given locator already exists.
     * - the parent node identified by the locator is not an object.
     */
    [[maybe_unused]] auto insert_node(NodeLocator const& locator) -> Node::id_t;

    /**
     * Takes a snapshot of the current schema tree (to allow recovery on failure).
     */
    auto take_snapshot() -> void { m_snapshot_size.emplace(m_tree_nodes.size()); }

    /**
     * Reverts the tree to the last snapshot.
     * @throw OperationFailed if no snapshot exists.
     */
    auto revert() -> void;

private:
    // Variables
    std::optional<size_t> m_snapshot_size;
    std::vector<Node> m_tree_nodes;
};
}  // namespace clp::ffi
#endif

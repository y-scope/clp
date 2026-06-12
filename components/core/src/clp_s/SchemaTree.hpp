#ifndef CLP_S_SCHEMATREE_HPP
#define CLP_S_SCHEMATREE_HPP

#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <absl/container/btree_map.h>
#include <absl/container/flat_hash_map.h>

#include <clp_s/archive_constants.hpp>
#include <clp_s/search/ast/Literal.hpp>

namespace clp_s {
/**
 * This enum defines the valid MPT node types as well as the 8-bit number used to encode them.
 *
 * The number used to represent each node type can not change. That means that elements in this
 * enum can never be reordered and that new node types always need to be added to the end of the
 * enum (but before Unknown).
 *
 * Node types are used to help record the structure of a log record, with the exception of the
 * "Metadata" node type. The "Metadata" type is a special type used by the implementation to
 * demarcate data needed by the implementation that is not part of the log record. In particular,
 * the implementation may create a special subtree of the MPT which contains fields used to record
 * things like original log order.
 *
 * The following NodeTypes are only used in the experimental prototype:
 *
 * `LogMessage`: Stores a structured representation of an unstructured log message. It is made up of
 * a single `LogType` node and all `CompositeVar`s and primitive type nodes that are in the message.
 *
 * `LogType`: Functionally similar to a `ClpString`, but has no variable dictionary component as the
 * variables are stored in their own nodes unlike a `ClpsString`. The logtype dictionary component
 * is identical.
 */
enum class NodeType : uint8_t {
    Integer = 0,
    Float = 1,
    ClpString = 2,
    VarString = 3,
    Boolean = 4,
    Object = 5,
    UnstructuredArray = 6,
    NullValue = 7,
    DeprecatedDateString = 8,
    StructuredArray = 9,
    Metadata = 10,
    DeltaInteger = 11,
    FormattedFloat = 12,
    DictionaryFloat = 13,
    Timestamp = 14,
    LogMessage = 15,
    LogType = 16,
    LogTypeID = 17,
    ParentRule = 18,
    Unknown = std::underlying_type_t<NodeType>(~0ULL)
};

/**
 * This class represents a single node in the SchemaTree.
 *
 * Note: the result of get_key_name is valid even if the original SchemaNode is later
 * move-constructed.
 */
class SchemaNode {
public:
    // Types
    using id_t = int32_t;

    // Static methods
    /**
     * Converts a node type to a literal type.
     * @param type
     * @return A literal type
     */
    static auto node_to_literal_type(NodeType type) -> clp_s::search::ast::LiteralType;

    // Constructors
    SchemaNode() : m_parent_id(-1), m_id(-1), m_type(NodeType::Integer) {}

    SchemaNode(
            id_t parent_id,
            id_t id,
            std::string_view key_name,
            NodeType type,
            uint32_t count,
            int32_t depth
    )
            : m_parent_id(parent_id),
              m_id(id),
              m_key_name(std::make_unique<std::string>(key_name)),
              m_type(type),
              m_count(count),
              m_depth(depth) {}

    // Methods
    [[nodiscard]] auto get_id() const -> id_t { return m_id; }

    [[nodiscard]] auto get_parent_id() const -> id_t { return m_parent_id; }

    [[nodiscard]] auto get_children_ids() const -> std::vector<id_t> const& {
        return m_children_ids;
    }

    [[nodiscard]] auto get_type() const -> NodeType { return m_type; }

    [[nodiscard]] auto get_key_name() const -> std::string_view { return *m_key_name; }

    [[nodiscard]] auto get_count() const -> uint32_t { return m_count; }

    [[nodiscard]] auto get_depth() const -> int32_t { return m_depth; }

    auto increase_count() -> void { m_count++; }

    auto add_child(id_t child_id) -> void { m_children_ids.push_back(child_id); }

    /**
     * Returns whether the node acts as a structural container (i.e., it can have children and be
     * traversed during key resolution).
     * @return true for Object, LogMessage, and ParentRule; false otherwise.
     */
    [[nodiscard]] auto is_structural_container() const -> bool;

private:
    // Data members
    id_t m_parent_id;
    id_t m_id;
    std::vector<id_t> m_children_ids;
    // We use a unique_ptr so that references to this key name are stable after this SchemaNode is
    // move constructed.
    // TODO clpp: see if possible to remove the need to reference key name.
    std::unique_ptr<std::string const> m_key_name;
    NodeType m_type;
    uint32_t m_count{0};
    int32_t m_depth{0};
};

class SchemaTree {
public:
    // Methods
    /**
     * Add a new schema node to the schema tree if the tree doesn't already contain a node with the
     * equivalent key. The schema node is keyed on the parent node ID, node type, and name. The
     * node's count is incremented.
     * @param parent_node_id
     * @param type
     * @param key_name
     * @return The schema node ID.
     * @throw From `add_node(SchemaNode::id_t, NodeType, std::string_view, uint32_t)`.
     */
    auto add_node(SchemaNode::id_t parent_node_id, NodeType type, std::string_view key_name)
            -> SchemaNode::id_t;

    /**
     * Add a new schema node to the schema tree if the tree doesn't already contain a node with the
     * equivalent key. The schema node is keyed on the parent node ID, node type, and name. If the
     * node did not exist, initialize its count.
     * @param parent_node_id
     * @param type
     * @param key_name
     * @param initial_count
     * @return The schema node ID.
     * @throw std::overflow_error When SchemaNode allocation fails.
     */
    auto add_node(
            SchemaNode::id_t parent_node_id,
            NodeType type,
            std::string_view key_name,
            uint32_t initial_count
    ) -> SchemaNode::id_t;

    [[nodiscard]] auto has_node(SchemaNode::id_t id) const -> bool {
        return id < m_nodes.size() && id >= 0;
    }

    [[nodiscard]] auto get_node(SchemaNode::id_t id) const -> SchemaNode const& {
        if (id >= m_nodes.size() || id < 0) {
            throw std::invalid_argument("invalid access of id " + std::to_string(id));
        }

        return m_nodes[id];
    }

    /**
     * Gets the root of the object sub-tree within a namespace.
     * @param subtree_namespace
     * @return the Id of the root of the Object sub-tree for the given namespace.
     * @return -1 if the Object sub-tree does not exist.
     */
    [[nodiscard]] auto get_object_subtree_node_id_for_namespace(
            std::string_view subtree_namespace
    ) const -> SchemaNode::id_t {
        return get_subtree_node_id(subtree_namespace, NodeType::Object);
    }

    /**
     * Gets the field Id for a specified field within the Metadata subtree.
     * @param field_name
     *
     * @return the field Id if the field exists within the Metadata sub-tree, -1 otherwise.
     */
    [[nodiscard]] auto get_metadata_field_id(std::string_view field_name) const -> SchemaNode::id_t;

    /**
     * Gets the Id of the root for a subtree identified by a namespace and type.
     * @param subtree_namespace
     * @param type
     * @return the Id of the subtree identified by the given namespace and type or -1 if the
     * requested subtree does not exist.
     */
    [[nodiscard]] auto get_subtree_node_id(std::string_view subtree_namespace, NodeType type) const
            -> SchemaNode::id_t;

    [[nodiscard]] auto get_subtrees() const
            -> absl::btree_map<std::pair<std::string_view, NodeType>, SchemaNode::id_t> const& {
        return m_namespace_and_type_to_subtree_id;
    }

    /**
     * @return the Id of the root of the Metadata sub-tree.
     * @return -1 if the Metadata sub-tree does not exist.
     */
    [[nodiscard]] auto get_metadata_subtree_node_id() const -> SchemaNode::id_t {
        return get_subtree_node_id(constants::cDefaultNamespace, NodeType::Metadata);
    }

    [[nodiscard]] auto get_nodes() const -> std::vector<SchemaNode> const& { return m_nodes; }

    /**
     * Write the contents of the SchemaTree to the schema tree file
     * @param archives_dir
     * @param compression_level
     * @return the compressed size of the SchemaTree in bytes
     */
    [[nodiscard]] auto store(std::string const& archives_dir, int compression_level) -> size_t;

    /**
     * Clear the schema tree
     */
    auto clear() -> void {
        m_nodes.clear();
        m_node_map.clear();
        m_namespace_and_type_to_subtree_id.clear();
    }

    /**
     * Builds the fully qualified name for a node by walking up to (but not including) the
     * LogMessage ancestor and concatenating key names with ".".
     * @param node_id The node ID to start from.
     * @return The dot-delimited FQN.
     */
    [[nodiscard]] auto build_qualified_name(SchemaNode::id_t node_id) const -> std::string;

    /**
     * Finds an ancestor node within a subtree that matches the given type. When multiple matching
     * nodes exist, returns the one closest to the root node of the subtree.
     * @param subtree_root_node The root node of the subtree
     * @param node The node to start searching from
     * @param subtree_type The type of the ancestor node to find
     * @return The ID of the ancestor node if it exists, otherwise -1
     */
    [[nodiscard]] auto find_matching_subtree_root_in_subtree(
            SchemaNode::id_t subtree_root_node,
            SchemaNode::id_t node,
            NodeType type
    ) const -> SchemaNode::id_t;

private:
    // Data members
    std::vector<SchemaNode> m_nodes;
    absl::flat_hash_map<std::tuple<SchemaNode::id_t, std::string_view, NodeType>, SchemaNode::id_t>
            m_node_map;
    absl::btree_map<std::pair<std::string_view, NodeType>, SchemaNode::id_t>
            m_namespace_and_type_to_subtree_id;
};
}  // namespace clp_s

#endif  // CLP_S_SCHEMATREE_HPP

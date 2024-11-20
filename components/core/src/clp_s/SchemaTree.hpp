#ifndef CLP_S_SCHEMATREE_HPP
#define CLP_S_SCHEMATREE_HPP

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <absl/container/flat_hash_map.h>

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
 */
enum class NodeType : uint8_t {
    Integer,
    Float,
    ClpString,
    VarString,
    Boolean,
    Object,
    UnstructuredArray,
    NullValue,
    DateString,
    StructuredArray,
    Metadata,
    Unknown = std::underlying_type<NodeType>::type(~0ULL)
};

/**
 * This class represents a single node in the SchemaTree.
 *
 * Note: the result of get_key_name is valid even if the original SchemaNode is later
 * move-constructed.
 */
class SchemaNode {
public:
    // Constructor
    SchemaNode() : m_parent_id(-1), m_id(-1), m_type(NodeType::Integer), m_count(0) {}

    SchemaNode(
            int32_t parent_id,
            int32_t id,
            std::string_view const key_name,
            NodeType type,
            int32_t depth
    )
            : m_parent_id(parent_id),
              m_id(id),
              m_key_name_buf(std::make_unique<char[]>(key_name.size())),
              m_key_name(m_key_name_buf.get(), key_name.size()),
              m_type(type),
              m_count(0),
              m_depth(depth) {
        memcpy(m_key_name_buf.get(), key_name.begin(), key_name.size());
    }

    /**
     * Getters
     */
    int32_t get_id() const { return m_id; }

    int32_t get_parent_id() const { return m_parent_id; }

    std::vector<int32_t> const& get_children_ids() const { return m_children_ids; }

    NodeType get_type() const { return m_type; }

    std::string_view const get_key_name() const { return m_key_name; }

    int32_t get_count() const { return m_count; }

    int32_t get_depth() const { return m_depth; }

    void set_depth(int32_t depth) { m_depth = depth; }

    /**
     * Increases the count of this node by 1
     */
    void increase_count() { m_count++; }

    /**
     * Adds a child node to this node
     * @param child_id
     */
    void add_child(int32_t child_id) { m_children_ids.push_back(child_id); }

private:
    int32_t m_id;
    int32_t m_parent_id;
    std::vector<int32_t> m_children_ids;
    // We use a buffer so that references to this key name are stable after this SchemaNode is move
    // constructed
    std::unique_ptr<char[]> m_key_name_buf;
    std::string_view m_key_name;
    NodeType m_type;
    int32_t m_count;
    int32_t m_depth{0};
};

class SchemaTree {
public:
    SchemaTree() = default;

    int32_t add_node(int parent_node_id, NodeType type, std::string_view const key);

    bool has_node(int32_t id) { return id < m_nodes.size() && id >= 0; }

    SchemaNode const& get_node(int32_t id) const {
        if (id >= m_nodes.size() || id < 0) {
            throw std::invalid_argument("invalid access of id " + std::to_string(id));
        }

        return m_nodes[id];
    }

    /**
     * @return the Id of the root of the Object sub-tree that records the structure of JSON data.
     * @return -1 if the Object sub-tree does not exist.
     */
    int32_t get_object_subtree_node_id() const { return m_object_subtree_id; }

    /**
     * Get the field Id for a specified field within the Metadata subtree.
     * @param field_name
     *
     * @return the field Id if the field exists within the Metadata sub-tree, -1 otherwise.
     */
    int32_t get_metadata_field_id(std::string_view const field_name);

    /**
     * @return the Id of the root of the Metadata sub-tree.
     * @return -1 if the Metadata sub-tree does not exist.
     */
    int32_t get_metadata_subtree_node_id() { return m_metadata_subtree_id; }

    std::vector<SchemaNode> const& get_nodes() const { return m_nodes; }

    /**
     * Write the contents of the SchemaTree to the schema tree file
     * @param archives_dir
     * @param compression_level
     * @return the compressed size of the SchemaTree in bytes
     */
    [[nodiscard]] size_t store(std::string const& archives_dir, int compression_level);

    /**
     * Clear the schema tree
     */
    void clear() {
        m_nodes.clear();
        m_node_map.clear();
    }

    /**
     * Finds an ancestor node within a subtree that matches the given type. When multiple matching
     * nodes exist, returns the one closest to the root node of the subtree.
     * @param subtree_root_node The root node of the subtree
     * @param node The node to start searching from
     * @param subtree_type The type of the ancestor node to find
     * @return The ID of the ancestor node if it exists, otherwise -1
     */
    [[nodiscard]] int32_t find_matching_subtree_root_in_subtree(
            int32_t const subtree_root_node,
            int32_t node,
            NodeType type
    ) const;

private:
    std::vector<SchemaNode> m_nodes;
    absl::flat_hash_map<std::tuple<int32_t, std::string_view const, NodeType>, int32_t> m_node_map;
    int32_t m_object_subtree_id{-1};
    int32_t m_metadata_subtree_id{-1};
};
}  // namespace clp_s

#endif  // CLP_S_SCHEMATREE_HPP

#ifndef CLP_S_SCHEMATREE_HPP
#define CLP_S_SCHEMATREE_HPP

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "DictionaryWriter.hpp"

namespace clp_s {
enum class NodeType : uint8_t {
    INTEGER,
    FLOAT,
    CLPSTRING,
    VARSTRING,
    BOOLEAN,
    OBJECT,
    ARRAY,
    NULLVALUE,
    DATESTRING,
    FLOATDATESTRING,
    VARVALUE,
    TRUNCATEDOBJECT,  // these two types are nominally the same for encoding, but have
    TRUNCATEDCHILDREN  // different search semantics, so they must be distinguished
};

enum class NodeValueState {
    UNINITIALIZED,
    CARDINALITY_ONE,
    CARDINALITY_MANY,
    TRUNCATED
};

class SchemaNode {
public:
    // Constructor
    SchemaNode() : m_parent_id(-1), m_id(-1), m_type(NodeType::INTEGER), m_count(0) {}

    SchemaNode(int32_t parent_id, int32_t id, std::string key_name, NodeType type)
            : m_parent_id(parent_id),
              m_id(id),
              m_key_name(std::move(key_name)),
              m_type(type),
              m_count(0),
              m_value(0),
              m_value_state(NodeValueState::UNINITIALIZED) {}

    /**
     * Getters
     */
    int32_t get_id() const { return m_id; }

    int32_t get_parent_id() const { return m_parent_id; }

    std::vector<int32_t> const& get_children_ids() const { return m_children_ids; }

    NodeType get_type() const { return m_type; }

    std::string const& get_key_name() const { return m_key_name; }

    int32_t get_count() const { return m_count; }

    /**
     * Increases the count of this node by 1
     */
    void increase_count() { m_count++; }

    /**
     * Adds a child node to this node
     * @param child_id
     */
    void add_child(int32_t child_id) { m_children_ids.push_back(child_id); }

    void mark_node_value(uint64_t value, std::string const& string_value);

    /**
     * Get the current state of this schema node.
     * @return the current state
     */
    NodeValueState get_state() const { return m_value_state; }

    /**
     * Set the current state of this schema node.
     * @param state
     */
    void set_state(NodeValueState state) { m_value_state = state; }

    /**
     * Get the value stored in this schema node if it is cardinality one.
     * @return the value in this schema node
     */
    uint64_t get_var_value() const { return m_value; }

    /**
     * Get the string value stored in this schema node if it is cardinality one.
     * @return the string value in this schema node
     */
    std::string const& get_string_var_value() const { return m_string_value; }

private:
    int32_t m_id;
    int32_t m_parent_id;
    std::vector<int32_t> m_children_ids;
    std::string m_key_name;
    NodeType m_type;
    int32_t m_count;
    uint64_t m_value;
    std::string m_string_value;
    NodeValueState m_value_state;
};

class SchemaTree {
public:
    SchemaTree() = default;

    int32_t add_node(int parent_node_id, NodeType type, std::string const& key);

    void resize_decompression(size_t new_size) { m_nodes.resize(new_size); }

    int32_t
    add_node_decompression(int node_id, int parent_node_id, NodeType type, std::string const& key);

    bool has_node(int32_t id) { return id < m_nodes.size() && id >= 0; }

    std::shared_ptr<SchemaNode> get_node(int32_t id) {
        if (id >= m_nodes.size() || id < 0) {
            throw std::invalid_argument("invalid access of id " + std::to_string(id));
        }

        return m_nodes[id];
    }

    int32_t get_root_node_id() { return m_nodes[0]->get_id(); }

    std::vector<std::shared_ptr<SchemaNode>> const& get_nodes() const { return m_nodes; }

    /**
     * Scan through all nodes and modify their state based on frequency statistics
     *
     * @return the list of nodes that have been changed, and their new IDs
     */
    std::vector<std::pair<int32_t, int32_t>> modify_nodes_based_on_frequency(size_t num_records);

private:
    std::vector<std::shared_ptr<SchemaNode>> m_nodes;
    absl::flat_hash_map<std::tuple<int32_t, std::string, NodeType>, int32_t> m_node_map;
};
}  // namespace clp_s

#endif  // CLP_S_SCHEMATREE_HPP

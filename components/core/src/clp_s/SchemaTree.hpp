#ifndef CLP_S_SCHEMATREE_HPP
#define CLP_S_SCHEMATREE_HPP

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <absl/container/flat_hash_map.h>

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
    UNKNOWN
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
              m_count(0) {}

    /**
     * Getters
     */
    int32_t get_id() const { return m_id; }

    int32_t get_parent_id() const { return m_parent_id; }

    std::vector<int32_t>& get_children_ids() { return m_children_ids; }

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

private:
    int32_t m_id;
    int32_t m_parent_id;
    std::vector<int32_t> m_children_ids;
    std::string m_key_name;
    NodeType m_type;
    int32_t m_count;
};

class SchemaTree {
public:
    SchemaTree() = default;

    int32_t add_node(int parent_node_id, NodeType type, std::string const& key);

    bool has_node(int32_t id) { return id < m_nodes.size() && id >= 0; }

    std::shared_ptr<SchemaNode> get_node(int32_t id) {
        if (id >= m_nodes.size() || id < 0) {
            throw std::invalid_argument("invalid access of id " + std::to_string(id));
        }

        return m_nodes[id];
    }

    int32_t get_root_node_id() { return m_nodes[0]->get_id(); }

    std::vector<std::shared_ptr<SchemaNode>> get_nodes() { return m_nodes; }

private:
    std::vector<std::shared_ptr<SchemaNode>> m_nodes;
    absl::flat_hash_map<std::tuple<int32_t, std::string, NodeType>, int32_t> m_node_map;
};
}  // namespace clp_s

#endif  // CLP_S_SCHEMATREE_HPP

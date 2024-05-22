#ifndef CLP_FFI_SCHEMATREENODE_HPP
#define CLP_FFI_SCHEMATREENODE_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace clp::ffi {
/**
 * This class implements a node in the schema tree. It stores information of a tree node, including
 * the node type, key name, parent id, and ids of all the child nodes.
 */
class SchemaTreeNode {
public:
    using id_t = size_t;

    /**
     * Enum defining schema tree node types.
     */
    enum class Type : uint8_t {
        Int = 0,
        Float,
        Bool,
        Str,
        UnstructuredArray,
        Obj
    };

    // Constructors
    SchemaTreeNode(id_t id, id_t parent_id, std::string_view key_name, Type type)
            : m_id{id},
              m_parent_id{parent_id},
              m_key_name{key_name.begin(), key_name.end()},
              m_type{type} {}

    // Delete copy constructor and assignment
    SchemaTreeNode(SchemaTreeNode const&) = delete;
    auto operator=(SchemaTreeNode const&) -> SchemaTreeNode& = delete;

    // Define default move constructor and assignment
    SchemaTreeNode(SchemaTreeNode&&) = default;
    auto operator=(SchemaTreeNode&&) -> SchemaTreeNode& = default;

    // Destructor
    ~SchemaTreeNode() = default;

    // Methods
    [[nodiscard]] auto get_id() const -> id_t { return m_id; }

    [[nodiscard]] auto get_parent_id() const -> id_t { return m_parent_id; }

    [[nodiscard]] auto get_key_name() const -> std::string_view { return m_key_name; }

    [[nodiscard]] auto get_type() const -> Type { return m_type; }

    [[nodiscard]] auto get_children_ids() const -> std::vector<id_t> const& {
        return m_children_ids;
    }

    /**
     * Appends a child node id to the end of the children list.
     * Note: the node id is the node index in the schema tree. It is impossible to check if the node
     * added is unique in the children list. Therefore, this function does not check whether the
     * node already exists. It is the caller's responsibility (schema tree) to ensure the node
     * appended is unique.
     * @param child_id The node id of the given child.
     */
    auto append_new_child_id(id_t child_id) -> void { m_children_ids.push_back(child_id); }

    /**
     * Removes the last appended child id from the children list.
     */
    auto remove_last_appended_child_id() -> void {
        if (m_children_ids.empty()) {
            return;
        }
        m_children_ids.pop_back();
    }

private:
    id_t m_id;
    id_t m_parent_id;
    std::vector<id_t> m_children_ids;
    std::string m_key_name;
    Type m_type;
};
}  // namespace clp::ffi

#endif

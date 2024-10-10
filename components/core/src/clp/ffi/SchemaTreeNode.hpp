#ifndef CLP_FFI_SCHEMATREENODE_HPP
#define CLP_FFI_SCHEMATREENODE_HPP

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace clp::ffi {
/**
 * A node in clp::ffi::SchemaTree. It stores the node's key name, type, parent's ID, and the IDs of
 * all its children.
 */
class SchemaTreeNode {
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

    // Factory functions
    /**
     * Creates a tree node.
     * @param id
     * @param parent_id
     * @param key_name
     * @param type
     */
    [[nodiscard]] static auto
    create(id_t id, id_t parent_id, std::string_view key_name, Type type) -> SchemaTreeNode {
        return {id, parent_id, key_name, type};
    }

    /**
     * Creates a root node.
     * @param id
     * @param parent_id
     * @param key_name
     * @param type
     */
    [[nodiscard]] static auto create_root(id_t id) -> SchemaTreeNode {
        return {id, std::nullopt, "", Type::Obj};
    }

    // Disable copy constructor/assignment operator
    SchemaTreeNode(SchemaTreeNode const&) = delete;
    auto operator=(SchemaTreeNode const&) -> SchemaTreeNode& = delete;

    // Define default move constructor/assignment operator
    SchemaTreeNode(SchemaTreeNode&&) = default;
    auto operator=(SchemaTreeNode&&) -> SchemaTreeNode& = default;

    // Destructor
    ~SchemaTreeNode() = default;

    // Methods
    [[nodiscard]] auto get_id() const -> id_t { return m_id; }

    [[nodiscard]] auto is_root() const -> bool { return false == m_parent_id.has_value(); }

    /**
     * @return the ID of the parent node in the schema tree if the node is not root.
     * @return std::nullopt if the node is root.
     */
    [[nodiscard]] auto get_parent_id() const -> std::optional<id_t> { return m_parent_id; }

    /**
     * Gets the parent ID without checking if it is std::nullopt.
     * NOTE: This method should only be used if the caller has checked the node is not the root.
     * `is_root()` must return false to call this method safely.
     * @return the ID of the parent node in the schema tree.
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
    // Constructors
    SchemaTreeNode(id_t id, std::optional<id_t> parent_id, std::string_view key_name, Type type)
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
}  // namespace clp::ffi

#endif

#include "KeyValuePairLogEvent.hpp"

#include <memory>
#include <optional>
#include <stack>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <json/single_include/nlohmann/json.hpp>
#include <outcome/single-header/outcome.hpp>

#include "../ir/EncodedTextAst.hpp"
#include "../time_types.hpp"
#include "SchemaTree.hpp"
#include "SchemaTreeNode.hpp"
#include "Value.hpp"

using clp::ir::EightByteEncodedTextAst;
using clp::ir::FourByteEncodedTextAst;
using std::string;
using std::vector;

namespace clp::ffi {
namespace {
/**
 * Callback to handle JSON exceptions. When we cannot throw a captured JSON exception (i.e., in a
 * destructor), there must be a way to not crash the program but also let the caller be aware of the
 * exception. This concept plays the role to allow users to customize a callback handler when
 * catching an exception.
 */
template <typename Func>
concept JsonExceptionCallbackConcept = std::is_invocable_v<Func, nlohmann::json::exception const&>;

/**
 * Class for iterating the schema tree using DFS. It contains a JSON map object representing itself,
 * which will be constructed by traversing through its children. When all children has been visited,
 * the iterator will be popped out from the DFS stack and emplace the constructed map to its parent
 * as a subtree.
 */
template <JsonExceptionCallbackConcept JsonExceptionCallback>
class SchemaTreeDfsIterator {
public:
    // Constructor
    SchemaTreeDfsIterator(
            SchemaTreeNode const* node,
            vector<SchemaTreeNode::id_t> children,
            nlohmann::json::object_t* parent,
            JsonExceptionCallback json_exception_callback
    )
            : m_schema_tree_node{node},
              m_children{std::move(children)},
              m_curr_child_it{m_children.cbegin()},
              m_parent{parent},
              m_map(nlohmann::json::object()),
              m_json_exception_callback{json_exception_callback} {}

    // Delete copy/move constructor and assignment
    SchemaTreeDfsIterator(SchemaTreeDfsIterator const&) = delete;
    SchemaTreeDfsIterator(SchemaTreeDfsIterator&&) = delete;
    auto operator=(SchemaTreeDfsIterator const&) -> SchemaTreeDfsIterator& = delete;
    auto operator=(SchemaTreeDfsIterator&&) -> SchemaTreeDfsIterator& = delete;

    // Destructor
    ~SchemaTreeDfsIterator() {
        try {
            // On exit, if the current node is the root, then move the entire tree to the `parent`
            // to return. Otherwise, construct a subtree in the parent.
            if (m_schema_tree_node->get_id() == SchemaTree::cRootId) {
                *m_parent = std::move(m_map);
            } else {
                m_parent->emplace(string{m_schema_tree_node->get_key_name()}, std::move(m_map));
            }
        } catch (nlohmann::json::exception const& ex) {
            m_json_exception_callback(ex);
        }
    }

    /**
     * @return whether there are more children to traverse.
     */
    [[nodiscard]] auto has_next_child() const -> bool {
        return m_curr_child_it != m_children.end();
    }

    /**
     * Gets the next child and advances the underlying child idx.
     * @return the next child to traverse.
     */
    [[nodiscard]] auto get_next_child() -> SchemaTreeNode::id_t { return *(m_curr_child_it++); }

    [[nodiscard]] auto get_map() -> nlohmann::json::object_t& { return m_map; }

private:
    SchemaTreeNode const* m_schema_tree_node;
    vector<SchemaTreeNode::id_t> m_children;
    vector<SchemaTreeNode::id_t>::const_iterator m_curr_child_it;
    nlohmann::json::object_t* m_parent;
    nlohmann::json::object_t m_map;
    JsonExceptionCallback m_json_exception_callback;
};

/**
 * @param type
 * @param value
 * @return Whether the given schema tree node type matches the given value's type.
 */
[[nodiscard]] auto
node_type_matches_value_type(SchemaTreeNode::Type type, Value const& value) -> bool;

/**
 * Validates whether the given node-ID value pairs are leaf nodes in the `SchemaTree` forming a
 * sub-tree of their own.
 * @param schema_tree
 * @param node_id_value_pairs
 * @return success if the inputs are valid, or an error code indicating the failure:
 * - std::errc::operation_not_permitted if a node ID doesn't represent a valid node in the
 *   schema tree, or a non-leaf node ID is paired with a value.
 * - std::errc::protocol_error if the schema tree node type doesn't match the value's type.
 * - std::errc::protocol_not_supported if the same key appears more than once under a parent
 *   node.
 */
[[nodiscard]] auto validate_node_id_value_pairs(
        SchemaTree const& schema_tree,
        KeyValuePairLogEvent::NodeIdValuePairs const& node_id_value_pairs
) -> std::errc;

/**
 * @param schema_tree
 * @param node_id
 * @param node_id_value_pairs
 * @return Whether the given node is a leaf node in the sub-tree of the `SchemaTree` defined by
 * `node_id_value_pairs`. A node is considered a leaf if none of its descendants appear in
 * `node_id_value_pairs`.
 */
[[nodiscard]] auto is_leaf_node(
        SchemaTree const& schema_tree,
        SchemaTreeNode::id_t node_id,
        KeyValuePairLogEvent::NodeIdValuePairs const& node_id_value_pairs
) -> bool;

/**
 * @param node_id_value_pairs
 * @param schema_tree
 * @return A result containing the bitmap where the node IDs appeared in the schema of
 * `node_id_value_pairs` are set, or an error code indicating the failure:
 * - std::errc::result_out_of_range if the key ID doesn't exist in the schema tree.
 */
[[nodiscard]] auto get_sub_schema_tree_bitmap(
        KeyValuePairLogEvent::NodeIdValuePairs const& node_id_value_pairs,
        SchemaTree const& schema_tree
) -> OUTCOME_V2_NAMESPACE::std_result<vector<bool>>;

/**
 * @param node
 * @param sub_schema_tree_bitmap
 * @return A vector that contains all the children of the current node that appears in the sub tree.
 */
[[nodiscard]] auto get_children_in_the_sub_schema_tree(
        SchemaTreeNode const& node,
        vector<bool> const& sub_schema_tree_bitmap
) -> vector<SchemaTreeNode::id_t>;

/**
 * Inserts the given key-value pair into the JSON map.
 * @param node The schema tree node of the key to insert.
 * @param val The value to insert.
 * @param json_map Outputs the inserted JSON map.
 * @return Whether the insertion was successful.
 */
[[nodiscard]] auto insert_kv_pair_json_map(
        SchemaTreeNode const& node,
        std::optional<Value> const& val,
        nlohmann::json::object_t& json_map
) -> bool;

/**
 * Decodes a value as an `EncodedTextAst` according to the encoding type.
 * NOTE: this method assumes the upper level caller already checked that `val` is either
 * `FourByteEncodedTextAst` or `EightByteEncodedTextAst`.
 * @param val
 * @return Same as `EncodedTextAst::decode_and_unparse`.
 */
[[nodiscard]] auto decode_as_encoded_text_ast(Value const& val) -> std::optional<string>;

auto node_type_matches_value_type(SchemaTreeNode::Type type, Value const& value) -> bool {
    switch (type) {
        case SchemaTreeNode::Type::Obj:
            return value.is_null();
        case SchemaTreeNode::Type::Int:
            return value.is<value_int_t>();
        case SchemaTreeNode::Type::Float:
            return value.is<value_float_t>();
        case SchemaTreeNode::Type::Bool:
            return value.is<value_bool_t>();
        case SchemaTreeNode::Type::UnstructuredArray:
            return value.is<FourByteEncodedTextAst>() || value.is<EightByteEncodedTextAst>();
        case SchemaTreeNode::Type::Str:
            return value.is<string>() || value.is<FourByteEncodedTextAst>()
                   || value.is<EightByteEncodedTextAst>();
        default:
            return false;
    }
}

auto validate_node_id_value_pairs(
        SchemaTree const& schema_tree,
        KeyValuePairLogEvent::NodeIdValuePairs const& node_id_value_pairs
) -> std::errc {
    try {
        std::unordered_map<SchemaTreeNode::id_t, std::unordered_set<std::string_view>>
                parent_node_id_to_key_names;
        for (auto const& [node_id, value] : node_id_value_pairs) {
            if (SchemaTree::cRootId == node_id) {
                return std::errc::operation_not_permitted;
            }

            auto const& node{schema_tree.get_node(node_id)};
            auto const node_type{node.get_type()};
            if (false == value.has_value()) {
                // Value is an empty object (`{}`, which is not the same as `null`)
                if (SchemaTreeNode::Type::Obj != node_type) {
                    return std::errc::protocol_error;
                }
            } else if (false == node_type_matches_value_type(node_type, value.value())) {
                return std::errc::protocol_error;
            }

            if (SchemaTreeNode::Type::Obj == node_type
                && false == is_leaf_node(schema_tree, node_id, node_id_value_pairs))
            {
                // The node's value is `null` or `{}` but its descendants appear in
                // `node_id_value_pairs`.
                return std::errc::operation_not_permitted;
            }

            auto const parent_node_id{node.get_parent_id()};
            auto const key_name{node.get_key_name()};
            if (parent_node_id_to_key_names.contains(parent_node_id)) {
                auto const [it, new_key_inserted]{
                        parent_node_id_to_key_names.at(parent_node_id).emplace(key_name)
                };
                if (false == new_key_inserted) {
                    // The key is duplicated under the same parent
                    return std::errc::protocol_not_supported;
                }
            } else {
                parent_node_id_to_key_names.emplace(parent_node_id, std::unordered_set{key_name});
            }
        }
    } catch (SchemaTree::OperationFailed const& ex) {
        return std::errc::operation_not_permitted;
    }
    return std::errc{};
}

auto is_leaf_node(
        SchemaTree const& schema_tree,
        SchemaTreeNode::id_t node_id,
        KeyValuePairLogEvent::NodeIdValuePairs const& node_id_value_pairs
) -> bool {
    vector<SchemaTreeNode::id_t> dfs_stack;
    dfs_stack.reserve(schema_tree.get_size());
    dfs_stack.push_back(node_id);
    while (false == dfs_stack.empty()) {
        auto const curr_node_id{dfs_stack.back()};
        dfs_stack.pop_back();
        for (auto const child_node_id : schema_tree.get_node(curr_node_id).get_children_ids()) {
            if (node_id_value_pairs.contains(child_node_id)) {
                return false;
            }
            dfs_stack.push_back(child_node_id);
        }
    }
    return true;
}

auto get_sub_schema_tree_bitmap(
        KeyValuePairLogEvent::NodeIdValuePairs const& node_id_value_pairs,
        SchemaTree const& schema_tree
) -> OUTCOME_V2_NAMESPACE::std_result<vector<bool>> {
    auto sub_schema_tree_bitmap{vector<bool>(schema_tree.get_size(), false)};
    for (auto const& [node_id, val] : node_id_value_pairs) {
        if (node_id >= sub_schema_tree_bitmap.size()) {
            return std::errc::result_out_of_range;
        }
        sub_schema_tree_bitmap[node_id] = true;

        // Iteratively mark the parents as true
        auto parent_id{schema_tree.get_node(node_id).get_parent_id()};
        while (true) {
            if (sub_schema_tree_bitmap[parent_id]) {
                // Parent already set by other child
                break;
            }
            sub_schema_tree_bitmap[parent_id] = true;
            if (SchemaTree::cRootId == parent_id) {
                break;
            }
            parent_id = schema_tree.get_node(parent_id).get_parent_id();
        }
    }
    return sub_schema_tree_bitmap;
}

auto get_children_in_the_sub_schema_tree(
        SchemaTreeNode const& node,
        vector<bool> const& sub_schema_tree_bitmap
) -> vector<SchemaTreeNode::id_t> {
    vector<SchemaTreeNode::id_t> children;
    for (auto const child_id : node.get_children_ids()) {
        if (sub_schema_tree_bitmap[child_id]) {
            children.push_back(child_id);
        }
    }
    return children;
}

auto insert_kv_pair_json_map(
        SchemaTreeNode const& node,
        std::optional<Value> const& val,
        nlohmann::json::object_t& json_map
) -> bool {
    auto const key_name{node.get_key_name()};
    auto const type{node.get_type()};
    if (false == val.has_value()) {
        json_map.emplace(string{key_name}, nlohmann::json::object());
        return true;
    }

    try {
        auto const& raw_val{val.value()};
        switch (type) {
            case SchemaTreeNode::Type::Int:
                json_map.emplace(string{key_name}, raw_val.get_immutable_view<value_int_t>());
                break;
            case SchemaTreeNode::Type::Float:
                json_map.emplace(string{key_name}, raw_val.get_immutable_view<value_float_t>());
                break;
            case SchemaTreeNode::Type::Bool:
                json_map.emplace(string{key_name}, raw_val.get_immutable_view<bool>());
                break;
            case SchemaTreeNode::Type::Str:
                if (raw_val.is<string>()) {
                    json_map.emplace(
                            string{key_name},
                            string{raw_val.get_immutable_view<string>()}
                    );
                } else {
                    auto const decoded_result{decode_as_encoded_text_ast(raw_val)};
                    if (false == decoded_result.has_value()) {
                        return false;
                    }
                    json_map.emplace(string{key_name}, decoded_result.value());
                }
                break;
            case SchemaTreeNode::Type::UnstructuredArray: {
                auto const decoded_result{decode_as_encoded_text_ast(raw_val)};
                if (false == decoded_result.has_value()) {
                    return false;
                }
                json_map.emplace(string{key_name}, nlohmann::json::parse(decoded_result.value()));
                break;
            }
            case SchemaTreeNode::Type::Obj:
                json_map.emplace(string{key_name}, nullptr);
                break;
            default:
                return false;
        }
    } catch (nlohmann::json::exception const& ex) {
        return false;
    } catch (Value::OperationFailed const& ex) {
        return false;
    }

    return true;
}

auto decode_as_encoded_text_ast(Value const& val) -> std::optional<string> {
    return val.is<FourByteEncodedTextAst>()
                   ? val.get_immutable_view<FourByteEncodedTextAst>().decode_and_unparse()
                   : val.get_immutable_view<EightByteEncodedTextAst>().decode_and_unparse();
}
}  // namespace

auto KeyValuePairLogEvent::create(
        std::shared_ptr<SchemaTree const> schema_tree,
        NodeIdValuePairs node_id_value_pairs,
        UtcOffset utc_offset
) -> OUTCOME_V2_NAMESPACE::std_result<KeyValuePairLogEvent> {
    if (auto const ret_val{validate_node_id_value_pairs(*schema_tree, node_id_value_pairs)};
        std::errc{} != ret_val)
    {
        return ret_val;
    }
    return KeyValuePairLogEvent{std::move(schema_tree), std::move(node_id_value_pairs), utc_offset};
}

auto KeyValuePairLogEvent::serialize_to_json(
) const -> OUTCOME_V2_NAMESPACE::std_result<nlohmann::json> {
    if (m_node_id_value_pairs.empty()) {
        return nlohmann::json::object();
    }

    auto const sub_schema_tree_bitmap_ret{
            get_sub_schema_tree_bitmap(m_node_id_value_pairs, *m_schema_tree)
    };
    if (sub_schema_tree_bitmap_ret.has_error()) {
        return sub_schema_tree_bitmap_ret.error();
    }
    auto const& sub_schema_tree_bitmap{sub_schema_tree_bitmap_ret.value()};

    auto json_root = nlohmann::json::object_t();

    bool json_exception_captured{false};
    auto json_exception_handler = [&]([[maybe_unused]] nlohmann::json::exception const& ex
                                  ) -> void { json_exception_captured = true; };
    using DfsIterator = SchemaTreeDfsIterator<decltype(json_exception_handler)>;

    // Note: we explicitly use `std::stack` which has `std::deque` as the underlying container.
    // Otherwise, we have to implement move semantics for `DfsIterator` to enable vector growth.
    std::stack<DfsIterator> dfs_stack;

    auto const& root_node{m_schema_tree->get_node(SchemaTree::cRootId)};
    dfs_stack.emplace(
            &root_node,
            get_children_in_the_sub_schema_tree(root_node, sub_schema_tree_bitmap),
            &json_root,
            json_exception_handler
    );

    while (false == dfs_stack.empty() && false == json_exception_captured) {
        auto& top{dfs_stack.top()};
        if (false == top.has_next_child()) {
            dfs_stack.pop();
            continue;
        }
        auto const child_id{top.get_next_child()};
        auto const& child_node{m_schema_tree->get_node(child_id)};
        if (m_node_id_value_pairs.contains(child_id)) {
            // Dealing with leaf nodes
            if (false
                == insert_kv_pair_json_map(
                        child_node,
                        m_node_id_value_pairs.at(child_id),
                        top.get_map()
                ))
            {
                return std::errc::protocol_error;
            }
        } else {
            dfs_stack.emplace(
                    &child_node,
                    get_children_in_the_sub_schema_tree(child_node, sub_schema_tree_bitmap),
                    &top.get_map(),
                    json_exception_handler
            );
        }
    }

    if (json_exception_captured) {
        return std::errc::protocol_error;
    }

    return json_root;
}
}  // namespace clp::ffi

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
#include "Value.hpp"

using clp::ir::EightByteEncodedTextAst;
using clp::ir::FourByteEncodedTextAst;
using std::string;
using std::vector;

namespace clp::ffi {
namespace {
/**
 * Concept for a function to handle a JSON exception.
 * @tparam Func
 */
template <typename Func>
concept JsonExceptionHandlerConcept = std::is_invocable_v<Func, nlohmann::json::exception const&>;

/**
 * Helper class for `KeyValuePairLogEvent::serialize_to_json`, used to:
 * - iterate over the children of a non-leaf schema tree node, so long as those children are in the
 *   subtree defined by the `KeyValuePairLogEvent`.
 * - group a non-leaf schema tree node with the JSON object that it's being serialized into.
 * - add the node's corresponding JSON object to its parent's corresponding JSON object (or if the
 *   node is the root, replace the parent JSON object) when this class is destructed.
 * @tparam JsonExceptionHandler Type of handler for any `nlohmann::json::exception` that occurs
 * during destruction.
 */
template <JsonExceptionHandlerConcept JsonExceptionHandler>
class JsonSerializationIterator {
public:
    // Constructor
    JsonSerializationIterator(
            SchemaTree::Node const* schema_tree_node,
            vector<bool> const& schema_subtree_bitmap,
            nlohmann::json::object_t* parent_json_obj,
            JsonExceptionHandler json_exception_callback
    )
            : m_schema_tree_node{schema_tree_node},
              m_parent_json_obj{parent_json_obj},
              m_json_exception_callback{json_exception_callback} {
        for (auto const child_id : schema_tree_node->get_children_ids()) {
            if (schema_subtree_bitmap[child_id]) {
                m_child_schema_tree_nodes.push_back(child_id);
            }
        }
        m_child_schema_tree_node_it = m_child_schema_tree_nodes.cbegin();
    }

    // Delete copy/move constructor and assignment
    JsonSerializationIterator(JsonSerializationIterator const&) = delete;
    JsonSerializationIterator(JsonSerializationIterator&&) = delete;
    auto operator=(JsonSerializationIterator const&) -> JsonSerializationIterator& = delete;
    auto operator=(JsonSerializationIterator&&) -> JsonSerializationIterator& = delete;

    // Destructor
    ~JsonSerializationIterator() {
        try {
            // If the current node is the root, then replace the `parent` with this node's JSON
            // object. Otherwise, add this node's JSON object as a child of the parent JSON object.
            if (m_schema_tree_node->is_root()) {
                *m_parent_json_obj = std::move(m_json_obj);
            } else {
                m_parent_json_obj->emplace(
                        string{m_schema_tree_node->get_key_name()},
                        std::move(m_json_obj)
                );
            }
        } catch (nlohmann::json::exception const& ex) {
            m_json_exception_callback(ex);
        }
    }

    /**
     * @return Whether there are more child schema tree nodes to traverse.
     */
    [[nodiscard]] auto has_next_child_schema_tree_node() const -> bool {
        return m_child_schema_tree_node_it != m_child_schema_tree_nodes.end();
    }

    /**
     * Gets the next child schema tree node and advances the iterator.
     * @return The next child schema tree node.
     */
    [[nodiscard]] auto get_next_child_schema_tree_node() -> SchemaTree::Node::id_t {
        return *(m_child_schema_tree_node_it++);
    }

    [[nodiscard]] auto get_json_obj() -> nlohmann::json::object_t& { return m_json_obj; }

private:
    SchemaTree::Node const* m_schema_tree_node;
    vector<SchemaTree::Node::id_t> m_child_schema_tree_nodes;
    vector<SchemaTree::Node::id_t>::const_iterator m_child_schema_tree_node_it;
    nlohmann::json::object_t* m_parent_json_obj;
    nlohmann::json::object_t m_json_obj;
    JsonExceptionHandler m_json_exception_callback;
};

/**
 * @param type
 * @param value
 * @return Whether the given schema tree node type matches the given value's type.
 */
[[nodiscard]] auto node_type_matches_value_type(SchemaTree::Node::Type type, Value const& value)
        -> bool;

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
        SchemaTree::Node::id_t node_id,
        KeyValuePairLogEvent::NodeIdValuePairs const& node_id_value_pairs
) -> bool;

/**
 * @param node_id_value_pairs
 * @param schema_tree
 * @return A result containing a bitmap where every bit corresponds to the ID of a node in the
 * schema tree, and the set bits correspond to the nodes in the subtree defined by all paths from
 * the root node to the nodes in `node_id_value_pairs`; or an error code indicating a failure:
 * - std::errc::result_out_of_range if a node ID in `node_id_value_pairs` doesn't exist in the
 *   schema tree.
 */
[[nodiscard]] auto get_schema_subtree_bitmap(
        KeyValuePairLogEvent::NodeIdValuePairs const& node_id_value_pairs,
        SchemaTree const& schema_tree
) -> OUTCOME_V2_NAMESPACE::std_result<vector<bool>>;

/**
 * Inserts the given key-value pair into the JSON object (map).
 * @param node The schema tree node of the key to insert.
 * @param optional_val The value to insert.
 * @param json_obj The JSON object to insert the kv-pair into.
 * @return Whether the insertion was successful.
 */
[[nodiscard]] auto insert_kv_pair_into_json_obj(
        SchemaTree::Node const& node,
        std::optional<Value> const& optional_val,
        nlohmann::json::object_t& json_obj
) -> bool;

/**
 * Decodes a value as an `EncodedTextAst` according to the encoding type.
 * NOTE: This function assumes that `val` is either a `FourByteEncodedTextAst` or
 * `EightByteEncodedTextAst`.
 * @param val
 * @return Same as `EncodedTextAst::decode_and_unparse`.
 */
[[nodiscard]] auto decode_as_encoded_text_ast(Value const& val) -> std::optional<string>;

/**
 * Serializes the given node-ID-value pairs into a `nlohmann::json` object.
 * @param schema_tree
 * @param node_id_value_pairs
 * @param schema_subtree_bitmap
 * @return A result containing the serialized JSON object or an error code indicating the failure:
 * - std::errc::protocol_error if a value in the log event couldn't be decoded, or it couldn't be
 *   inserted into a JSON object.
 */
[[nodiscard]] auto serialize_node_id_value_pairs_to_json(
        SchemaTree const& schema_tree,
        KeyValuePairLogEvent::NodeIdValuePairs const& node_id_value_pairs,
        vector<bool> const& schema_subtree_bitmap
) -> OUTCOME_V2_NAMESPACE::std_result<nlohmann::json>;

/**
 * @param node A non-root schema tree node.
 * @param parent_node_id_to_key_names
 * @return true if `node`'s key is unique among its sibling nodes with `parent_node_id_to_key_names`
 * updated to keep track of this unique key name.
 * @return false if a sibling of `node` has the same key.
 */
[[nodiscard]] auto check_key_uniqueness_among_sibling_nodes(
        SchemaTree::Node const& node,
        std::unordered_map<SchemaTree::Node::id_t, std::unordered_set<std::string_view>>&
                parent_node_id_to_key_names
) -> bool;

auto node_type_matches_value_type(SchemaTree::Node::Type type, Value const& value) -> bool {
    switch (type) {
        case SchemaTree::Node::Type::Obj:
            return value.is_null();
        case SchemaTree::Node::Type::Int:
            return value.is<value_int_t>();
        case SchemaTree::Node::Type::Float:
            return value.is<value_float_t>();
        case SchemaTree::Node::Type::Bool:
            return value.is<value_bool_t>();
        case SchemaTree::Node::Type::UnstructuredArray:
            return value.is<FourByteEncodedTextAst>() || value.is<EightByteEncodedTextAst>();
        case SchemaTree::Node::Type::Str:
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
        std::unordered_map<SchemaTree::Node::id_t, std::unordered_set<std::string_view>>
                parent_node_id_to_key_names;
        std::vector<bool> key_duplication_checked_node_id_bitmap(schema_tree.get_size(), false);
        for (auto const& [node_id, value] : node_id_value_pairs) {
            auto const& node{schema_tree.get_node(node_id)};
            if (node.is_root()) {
                return std::errc::operation_not_permitted;
            }

            auto const node_type{node.get_type()};
            if (false == value.has_value()) {
                // Value is an empty object (`{}`, which is not the same as `null`)
                if (SchemaTree::Node::Type::Obj != node_type) {
                    return std::errc::protocol_error;
                }
            } else if (false == node_type_matches_value_type(node_type, value.value())) {
                return std::errc::protocol_error;
            }

            if (SchemaTree::Node::Type::Obj == node_type
                && false == is_leaf_node(schema_tree, node_id, node_id_value_pairs))
            {
                // The node's value is `null` or `{}` but its descendants appear in
                // `node_id_value_pairs`.
                return std::errc::operation_not_permitted;
            }

            if (false
                == check_key_uniqueness_among_sibling_nodes(node, parent_node_id_to_key_names))
            {
                return std::errc::protocol_not_supported;
            }

            // Iteratively check if there's any key duplication in the node's ancestors until:
            // 1. The ancestor has already been checked. We only need to check an ancestor node
            //    once since if there are key duplications among its siblings, it would've been
            //    caught when the sibling was first checked (the order in which siblings get checked
            //    doesn't affect the results).
            // 2. We reach the root node.
            auto next_ancestor_node_id_to_check{node.get_parent_id_unsafe()};
            while (false == key_duplication_checked_node_id_bitmap[next_ancestor_node_id_to_check])
            {
                auto const& node_to_check{schema_tree.get_node(next_ancestor_node_id_to_check)};
                if (node_to_check.is_root()) {
                    key_duplication_checked_node_id_bitmap[node_to_check.get_id()] = true;
                    break;
                }

                if (false
                    == check_key_uniqueness_among_sibling_nodes(
                            node_to_check,
                            parent_node_id_to_key_names
                    ))
                {
                    return std::errc::protocol_not_supported;
                }

                key_duplication_checked_node_id_bitmap[next_ancestor_node_id_to_check] = true;
                next_ancestor_node_id_to_check = node_to_check.get_parent_id_unsafe();
            }
        }
    } catch (SchemaTree::OperationFailed const& ex) {
        return std::errc::operation_not_permitted;
    }
    return std::errc{};
}

auto is_leaf_node(
        SchemaTree const& schema_tree,
        SchemaTree::Node::id_t node_id,
        KeyValuePairLogEvent::NodeIdValuePairs const& node_id_value_pairs
) -> bool {
    vector<SchemaTree::Node::id_t> dfs_stack;
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

auto get_schema_subtree_bitmap(
        KeyValuePairLogEvent::NodeIdValuePairs const& node_id_value_pairs,
        SchemaTree const& schema_tree
) -> OUTCOME_V2_NAMESPACE::std_result<vector<bool>> {
    vector<bool> schema_subtree_bitmap(schema_tree.get_size(), false);
    for (auto const& [node_id, val] : node_id_value_pairs) {
        if (node_id >= schema_subtree_bitmap.size()) {
            return std::errc::result_out_of_range;
        }
        schema_subtree_bitmap[node_id] = true;

        // Iteratively mark the parents as true
        auto optional_parent_id{schema_tree.get_node(node_id).get_parent_id()};
        while (true) {
            // Ideally, we'd use this if statement as the loop condition, but clang-tidy will
            // complain about an unchecked `optional` access.
            if (false == optional_parent_id.has_value()) {
                // Reached the root
                break;
            }
            auto const parent_id{optional_parent_id.value()};
            if (schema_subtree_bitmap[parent_id]) {
                // Parent already set by other child
                break;
            }
            schema_subtree_bitmap[parent_id] = true;
            optional_parent_id = schema_tree.get_node(parent_id).get_parent_id();
        }
    }
    return schema_subtree_bitmap;
}

auto insert_kv_pair_into_json_obj(
        SchemaTree::Node const& node,
        std::optional<Value> const& optional_val,
        nlohmann::json::object_t& json_obj
) -> bool {
    string const key_name{node.get_key_name()};
    auto const type{node.get_type()};
    if (false == optional_val.has_value()) {
        json_obj.emplace(key_name, nlohmann::json::object());
        return true;
    }

    try {
        auto const& val{optional_val.value()};
        switch (type) {
            case SchemaTree::Node::Type::Int:
                json_obj.emplace(key_name, val.get_immutable_view<value_int_t>());
                break;
            case SchemaTree::Node::Type::Float:
                json_obj.emplace(key_name, val.get_immutable_view<value_float_t>());
                break;
            case SchemaTree::Node::Type::Bool:
                json_obj.emplace(key_name, val.get_immutable_view<bool>());
                break;
            case SchemaTree::Node::Type::Str:
                if (val.is<string>()) {
                    json_obj.emplace(key_name, string{val.get_immutable_view<string>()});
                } else {
                    auto const decoded_result{decode_as_encoded_text_ast(val)};
                    if (false == decoded_result.has_value()) {
                        return false;
                    }
                    json_obj.emplace(key_name, decoded_result.value());
                }
                break;
            case SchemaTree::Node::Type::UnstructuredArray: {
                auto const decoded_result{decode_as_encoded_text_ast(val)};
                if (false == decoded_result.has_value()) {
                    return false;
                }
                json_obj.emplace(key_name, nlohmann::json::parse(decoded_result.value()));
                break;
            }
            case SchemaTree::Node::Type::Obj:
                json_obj.emplace(key_name, nullptr);
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

auto serialize_node_id_value_pairs_to_json(
        SchemaTree const& schema_tree,
        KeyValuePairLogEvent::NodeIdValuePairs const& node_id_value_pairs,
        vector<bool> const& schema_subtree_bitmap
) -> OUTCOME_V2_NAMESPACE::std_result<nlohmann::json> {
    if (node_id_value_pairs.empty()) {
        return nlohmann::json::object();
    }

    bool json_exception_captured{false};
    auto json_exception_handler = [&]([[maybe_unused]] nlohmann::json::exception const& ex
                                  ) -> void { json_exception_captured = true; };
    using DfsIterator = JsonSerializationIterator<decltype(json_exception_handler)>;

    // NOTE: We use a `std::stack` (which uses `std::deque` as the underlying container) instead of
    // a `std::vector` to avoid implementing move semantics for `DfsIterator` (required when the
    // vector grows).
    std::stack<DfsIterator> dfs_stack;

    // Traverse the schema tree in DFS order, but only traverse the nodes that are set in
    // `schema_subtree_bitmap`.
    //
    // On the way down:
    // - for each non-leaf node, create a `nlohmann::json::object_t`;
    // - for each leaf node, insert the key-value pair into the parent `nlohmann::json::object_t`.
    //
    // On the way up, add the current node's `nlohmann::json::object_t` to the parent's
    // `nlohmann::json::object_t`.
    auto const& root_schema_tree_node{schema_tree.get_root()};
    auto root_json_obj = nlohmann::json::object_t();

    dfs_stack.emplace(
            &root_schema_tree_node,
            schema_subtree_bitmap,
            &root_json_obj,
            json_exception_handler
    );
    while (false == dfs_stack.empty() && false == json_exception_captured) {
        auto& top{dfs_stack.top()};
        if (false == top.has_next_child_schema_tree_node()) {
            dfs_stack.pop();
            continue;
        }
        auto const child_schema_tree_node_id{top.get_next_child_schema_tree_node()};
        auto const& child_schema_tree_node{schema_tree.get_node(child_schema_tree_node_id)};
        if (node_id_value_pairs.contains(child_schema_tree_node_id)) {
            // Handle leaf node
            if (false
                == insert_kv_pair_into_json_obj(
                        child_schema_tree_node,
                        node_id_value_pairs.at(child_schema_tree_node_id),
                        top.get_json_obj()
                ))
            {
                return std::errc::protocol_error;
            }
        } else {
            dfs_stack.emplace(
                    &child_schema_tree_node,
                    schema_subtree_bitmap,
                    &top.get_json_obj(),
                    json_exception_handler
            );
        }
    }

    if (json_exception_captured) {
        return std::errc::protocol_error;
    }

    return root_json_obj;
}

auto check_key_uniqueness_among_sibling_nodes(
        SchemaTree::Node const& node,
        std::unordered_map<SchemaTree::Node::id_t, std::unordered_set<std::string_view>>&
                parent_node_id_to_key_names
) -> bool {
    // The caller checks that the given node is not the root, so we can query the underlying
    // parent ID safely without a check.
    auto const parent_node_id{node.get_parent_id_unsafe()};
    auto const key_name{node.get_key_name()};
    auto const parent_node_id_to_key_names_it{parent_node_id_to_key_names.find(parent_node_id)};
    if (parent_node_id_to_key_names_it != parent_node_id_to_key_names.end()) {
        auto const [it, new_key_inserted]{parent_node_id_to_key_names_it->second.emplace(key_name)};
        if (false == new_key_inserted) {
            // The key is duplicated under the same parent
            return false;
        }
    } else {
        parent_node_id_to_key_names.emplace(parent_node_id, std::unordered_set{key_name});
    }
    return true;
}
}  // namespace

auto KeyValuePairLogEvent::create(
        std::shared_ptr<SchemaTree const> auto_gen_keys_schema_tree,
        std::shared_ptr<SchemaTree const> user_gen_keys_schema_tree,
        NodeIdValuePairs auto_gen_node_id_value_pairs,
        NodeIdValuePairs user_gen_node_id_value_pairs,
        UtcOffset utc_offset
) -> OUTCOME_V2_NAMESPACE::std_result<KeyValuePairLogEvent> {
    if (nullptr == auto_gen_keys_schema_tree || nullptr == user_gen_keys_schema_tree) {
        return std::errc::invalid_argument;
    }

    if (auto const ret_val{validate_node_id_value_pairs(
                *auto_gen_keys_schema_tree,
                auto_gen_node_id_value_pairs
        )};
        std::errc{} != ret_val)
    {
        return ret_val;
    }

    if (auto const ret_val{validate_node_id_value_pairs(
                *user_gen_keys_schema_tree,
                user_gen_node_id_value_pairs
        )};
        std::errc{} != ret_val)
    {
        return ret_val;
    }

    return KeyValuePairLogEvent{
            std::move(auto_gen_keys_schema_tree),
            std::move(user_gen_keys_schema_tree),
            std::move(auto_gen_node_id_value_pairs),
            std::move(user_gen_node_id_value_pairs),
            utc_offset
    };
}

auto KeyValuePairLogEvent::get_auto_gen_keys_schema_subtree_bitmap() const
        -> OUTCOME_V2_NAMESPACE::std_result<std::vector<bool>> {
    return get_schema_subtree_bitmap(m_auto_gen_node_id_value_pairs, *m_auto_gen_keys_schema_tree);
}

auto KeyValuePairLogEvent::get_user_gen_keys_schema_subtree_bitmap() const
        -> outcome_v2::std_result<std::vector<bool>> {
    return get_schema_subtree_bitmap(m_user_gen_node_id_value_pairs, *m_user_gen_keys_schema_tree);
}

auto KeyValuePairLogEvent::serialize_to_json() const
        -> OUTCOME_V2_NAMESPACE::std_result<std::pair<nlohmann::json, nlohmann::json>> {
    auto const auto_gen_keys_schema_subtree_bitmap_result{get_auto_gen_keys_schema_subtree_bitmap()
    };
    if (auto_gen_keys_schema_subtree_bitmap_result.has_error()) {
        return auto_gen_keys_schema_subtree_bitmap_result.error();
    }
    auto serialized_auto_gen_kv_pairs_result{serialize_node_id_value_pairs_to_json(
            *m_auto_gen_keys_schema_tree,
            m_auto_gen_node_id_value_pairs,
            auto_gen_keys_schema_subtree_bitmap_result.value()
    )};
    if (serialized_auto_gen_kv_pairs_result.has_error()) {
        return serialized_auto_gen_kv_pairs_result.error();
    }

    auto const user_gen_keys_schema_subtree_bitmap_result{get_user_gen_keys_schema_subtree_bitmap()
    };
    if (user_gen_keys_schema_subtree_bitmap_result.has_error()) {
        return user_gen_keys_schema_subtree_bitmap_result.error();
    }
    auto serialized_user_gen_kv_pairs_result{serialize_node_id_value_pairs_to_json(
            *m_user_gen_keys_schema_tree,
            m_user_gen_node_id_value_pairs,
            user_gen_keys_schema_subtree_bitmap_result.value()
    )};
    if (serialized_user_gen_kv_pairs_result.has_error()) {
        return serialized_user_gen_kv_pairs_result.error();
    }

    return {std::move(serialized_auto_gen_kv_pairs_result.value()),
            std::move(serialized_user_gen_kv_pairs_result.value())};
}
}  // namespace clp::ffi

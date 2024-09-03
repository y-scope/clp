#include "KeyValuePairLogEvent.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <outcome/single-header/outcome.hpp>

#include "../ir/EncodedTextAst.hpp"
#include "../time_types.hpp"
#include "SchemaTree.hpp"
#include "SchemaTreeNode.hpp"
#include "Value.hpp"

using clp::ir::EightByteEncodedTextAst;
using clp::ir::FourByteEncodedTextAst;
using std::string;

namespace clp::ffi {
namespace {
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
    std::vector<SchemaTreeNode::id_t> dfs_stack;
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
}  // namespace clp::ffi

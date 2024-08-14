#include "KeyValuePairLogEvent.hpp"

#include <memory>
#include <string>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <utility>

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
[[nodiscard]] auto is_valid_value_type(SchemaTreeNode::Type type, Value const& value) -> bool;

auto is_valid_value_type(SchemaTreeNode::Type type, Value const& value) -> bool {
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
}  // namespace

auto KeyValuePairLogEvent::create(
        std::shared_ptr<SchemaTree> schema_tree,
        KeyValuePairs kv_pairs,
        UtcOffset utc_offset
) -> OUTCOME_V2_NAMESPACE::std_result<KeyValuePairLogEvent> {
    std::unordered_map<SchemaTreeNode::id_t, std::unordered_set<string>> key_sets;
    try {
        for (auto const& [key_id, value] : kv_pairs) {
            if (SchemaTree::cRootId == key_id) {
                return std::errc::protocol_error;
            }

            auto const& node{schema_tree->get_node(key_id)};
            auto const type{node.get_type()};
            if (false == value.has_value()) {
                // Value is an empty object (`{}`, which is not the same as `null`)
                if (SchemaTreeNode::Type::Obj != type) {
                    return std::errc::protocol_error;
                }
            } else if (false == is_valid_value_type(type, value.value())) {
                return std::errc::protocol_error;
            }

            auto const parent_id{node.get_parent_id()};
            auto const key_name{node.get_key_name()};
            if (key_sets.contains(parent_id)) {
                if (key_sets.at(parent_id).contains({key_name.begin(), key_name.end()})) {
                    // The key is duplicated
                    return std::errc::protocol_not_supported;
                }
            } else {
                key_sets.emplace(parent_id, std::unordered_set{string{key_name}});
            }
        }
    } catch (SchemaTree::OperationFailed const& ex) {
        return std::errc::operation_not_permitted;
    }
    return KeyValuePairLogEvent{std::move(schema_tree), std::move(kv_pairs), utc_offset};
}
}  // namespace clp::ffi

#include "KeyValuePairLogEvent.hpp"

#include <memory>
#include <string>
#include <system_error>
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

namespace clp::ffi {
namespace {
/**
 * @param type
 * @param value
 * @return Whether the given type and the value matches.
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
            return value.is<std::string>() || value.is<FourByteEncodedTextAst>()
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
    try {
        for (auto const& [key_id, value] : kv_pairs) {
            auto const type{schema_tree->get_node(key_id).get_type()};
            if (false == value.has_value()) {
                // Empty value
                if (SchemaTreeNode::Type::Obj != type) {
                    return std::errc::protocol_error;
                }
            } else if (false == is_valid_value_type(type, value.value())) {
                return std::errc::protocol_error;
            }
        }
    } catch (SchemaTree::OperationFailed const& ex) {
        return std::errc::protocol_error;
    }
    return KeyValuePairLogEvent{std::move(schema_tree), std::move(kv_pairs), utc_offset};
}
}  // namespace clp::ffi

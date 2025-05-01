#ifndef CLP_FFI_IR_STREAM_SEARCH_UTILS_HPP
#define CLP_FFI_IR_STREAM_SEARCH_UTILS_HPP

#include <optional>

#include <outcome/outcome.hpp>

#include "../../../../clp_s/search/ast/FilterExpr.hpp"
#include "../../../../clp_s/search/ast/Literal.hpp"
#include "../../SchemaTree.hpp"
#include "../../Value.hpp"

namespace clp::ffi::ir_stream::search {
/**
 * @param node_type
 * @return A bitmask representing all possible matching literal types of `node_type`.
 */
[[nodiscard]] auto schema_tree_node_type_to_literal_types(SchemaTree::Node::Type node_type)
        -> clp_s::search::ast::LiteralTypeBitmask;

/**
 * @param node_type
 * @param value
 * @return The matching literal type of the given <`node_type`, `value`> pair.
 */
[[nodiscard]] auto schema_tree_node_type_value_pair_to_literal_type(
        SchemaTree::Node::Type node_type,
        std::optional<Value> const& value
) -> clp_s::search::ast::LiteralType;

/**
 * Evaluates a filter expression against the specified <`literal_type`, `value`> pair.
 * @param filter
 * @param literal_type
 * @param value
 * @param case_sensitive_match Whether the string comparison should be case-sensitive.
 * @return A result containing a boolean indicate whether the value satisfies the filter expression
 * on success, or an error code indicating the failure:
 * - ErrorCodeEnum::LiteralTypeUnexpected if `literal_type` is one of the following:
 *   - LiteralType::NullT
 *   - LiteralType::UnknownT
 *   - Any other unrecognized literal type enum
 * - ErrorCodeEnum::LiteralTypeUnsupported if `literal_type` is one of the following:
 *   - LiteralType::EpochDateT since the current IR format doesn't support this type.
 *   - LiteralType::ArrayT since array search hasn't been implemented.
 */
[[nodiscard]] auto evaluate_filter_against_literal_type_value_pair(
        clp_s::search::ast::FilterExpr const* filter,
        clp_s::search::ast::LiteralType literal_type,
        std::optional<Value> const& value,
        bool case_sensitive_match
) -> outcome_v2::std_result<bool>;
}  // namespace clp::ffi::ir_stream::search

#endif  // CLP_FFI_IR_STREAM_SEARCH_UTILS_HPP

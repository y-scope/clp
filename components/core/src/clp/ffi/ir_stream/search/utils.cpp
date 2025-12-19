#include "utils.hpp"

#include <concepts>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include <string_utils/string_utils.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include "../../../../clp_s/search/ast/FilterExpr.hpp"
#include "../../../../clp_s/search/ast/FilterOperation.hpp"
#include "../../../../clp_s/search/ast/Literal.hpp"
#include "../../../ffi/EncodedTextAst.hpp"
#include "../../SchemaTree.hpp"
#include "../../Value.hpp"
#include "ErrorCode.hpp"

namespace clp::ffi::ir_stream::search {
namespace {
using clp_s::search::ast::FilterOperation;
using clp_s::search::ast::Literal;
using clp_s::search::ast::literal_type_bitmask_t;
using clp_s::search::ast::LiteralType;

/**
 * Evaluates a numerical filter operation by applying the specified `FilterOperation` to two
 * operands.
 * @tparam OperandType The type of the operands, must be either `value_int_t` or `value_float_t`.
 * @param op
 * @param filter_operand The operand associated with the filter.
 * @param value_operand The value operand to evaluate.
 * @return Whether the filter condition is satisfied.
 */
template <typename OperandType>
requires std::same_as<OperandType, value_int_t> || std::same_as<OperandType, value_float_t>
[[nodiscard]] auto evaluate_numerical_filter_op(
        FilterOperation op,
        OperandType filter_operand,
        OperandType value_operand
) -> bool;

/**
 * Evaluates a string filter operation by applying the specified `FilterOperation` to two string
 * operands.
 * @param op
 * @param filter_operand The operand associated with the filter.
 * @param value_operand The value operand to evaluate.
 * @return Whether the filter condition is satisfied.
 */
[[nodiscard]] auto evaluate_string_filter_op(
        FilterOperation op,
        std::string_view filter_operand,
        std::string_view value_operand,
        bool case_sensitive_match
) -> bool;

/**
 * Evaluates an integer filter operation by applying the specified `FilterOperation` to the given
 * filter operand and value.
 * @param op
 * @param operand
 * @param value
 * @return Whether the filter condition is satisfied.
 */
[[nodiscard]] auto evaluate_int_filter_op(
        FilterOperation op,
        std::shared_ptr<Literal> const& operand,
        Value const& value
) -> bool;

/**
 * Evaluates a float filter operation by applying the specified `FilterOperation` to the given
 * filter operand and value.
 * @param op
 * @param operand
 * @param value
 * @return Whether the filter condition is satisfied.
 */
[[nodiscard]] auto evaluate_float_filter_op(
        FilterOperation op,
        std::shared_ptr<Literal> const& operand,
        Value const& value
) -> bool;

/**
 * Evaluates a boolean filter operation by applying the specified `FilterOperation` to the given
 * operand and value.
 * @param op
 * @param operand
 * @param value
 * @return Whether the filter condition is satisfied.
 */
[[nodiscard]] auto evaluate_bool_filter_op(
        FilterOperation op,
        std::shared_ptr<Literal> const& operand,
        Value const& value
) -> bool;

/**
 * Evaluates a `VarString` filter operation by applying the specified `FilterOperation` to the given
 * operand and value.
 * @param op
 * @param operand
 * @param value
 * @return Whether the filter condition is satisfied.
 */
[[nodiscard]] auto evaluate_var_string_filter_op(
        FilterOperation op,
        std::shared_ptr<Literal> const& operand,
        Value const& value,
        bool case_sensitive_match
) -> bool;

/**
 * Evaluates a `ClpString` filter operation by applying the specified `FilterOperation` to the given
 * operand and value.
 * @param op
 * @param operand
 * @param value
 * @return A result containing a boolean indicating whether the filter condition is satisfied on
 * success, or an error code indicating the failure:
 * - Forwards `EncodedTextAst::to_string`'s return values.
 */
[[nodiscard]] auto evaluate_clp_string_filter_op(
        FilterOperation op,
        std::shared_ptr<Literal> const& operand,
        Value const& value,
        bool case_sensitive_match
) -> ystdlib::error_handling::Result<bool>;

template <typename OperandType>
requires std::same_as<OperandType, value_int_t> || std::same_as<OperandType, value_float_t>
auto evaluate_numerical_filter_op(
        FilterOperation op,
        OperandType filter_operand,
        OperandType value_operand
) -> bool {
    switch (op) {
        case FilterOperation::EQ:
            return value_operand == filter_operand;
        case FilterOperation::NEQ:
            return value_operand != filter_operand;
        case FilterOperation::LT:
            return value_operand < filter_operand;
        case FilterOperation::GT:
            return value_operand > filter_operand;
        case FilterOperation::LTE:
            return value_operand <= filter_operand;
        case FilterOperation::GTE:
            return value_operand >= filter_operand;
        default:
            return false;
    }
}

auto evaluate_string_filter_op(
        FilterOperation op,
        std::string_view filter_operand,
        std::string_view value_operand,
        bool case_sensitive_match
) -> bool {
    switch (op) {
        case FilterOperation::EQ:
            return clp::string_utils::wildcard_match_unsafe(
                    value_operand,
                    filter_operand,
                    case_sensitive_match
            );
        case FilterOperation::NEQ:
            return false
                   == clp::string_utils::wildcard_match_unsafe(
                           value_operand,
                           filter_operand,
                           case_sensitive_match
                   );
        default:
            return false;
    }
}

auto evaluate_int_filter_op(
        FilterOperation op,
        std::shared_ptr<Literal> const& operand,
        Value const& value
) -> bool {
    value_int_t filter_operand{};
    if (false == operand->as_int(filter_operand, op)) {
        return false;
    }
    return evaluate_numerical_filter_op(
            op,
            filter_operand,
            value.get_immutable_view<value_int_t>()
    );
}

auto evaluate_float_filter_op(
        FilterOperation op,
        std::shared_ptr<Literal> const& operand,
        Value const& value
) -> bool {
    value_float_t filter_operand{};
    if (false == operand->as_float(filter_operand, op)) {
        return false;
    }
    return evaluate_numerical_filter_op(
            op,
            filter_operand,
            value.get_immutable_view<value_float_t>()
    );
}

auto evaluate_bool_filter_op(
        FilterOperation op,
        std::shared_ptr<Literal> const& operand,
        Value const& value
) -> bool {
    bool filter_operand{};
    if (false == operand->as_bool(filter_operand, op)) {
        return false;
    }
    auto const value_operand{value.get_immutable_view<value_bool_t>()};

    switch (op) {
        case FilterOperation::EQ:
            return value_operand == filter_operand;
        case FilterOperation::NEQ:
            return value_operand != filter_operand;
        default:
            return false;
    }
}

auto evaluate_var_string_filter_op(
        FilterOperation op,
        std::shared_ptr<Literal> const& operand,
        Value const& value,
        bool case_sensitive_match
) -> bool {
    std::string filter_operand;
    if (false == operand->as_var_string(filter_operand, op)) {
        return false;
    }
    auto const value_operand{value.get_immutable_view<std::string>()};

    return evaluate_string_filter_op(op, filter_operand, value_operand, case_sensitive_match);
}

auto evaluate_clp_string_filter_op(
        FilterOperation op,
        std::shared_ptr<Literal> const& operand,
        Value const& value,
        bool case_sensitive_match
) -> ystdlib::error_handling::Result<bool> {
    std::string filter_operand;
    if (false == operand->as_clp_string(filter_operand, op)) {
        return false;
    }

    auto const value_operand{YSTDLIB_ERROR_HANDLING_TRYX(
            value.is<clp::ffi::EightByteEncodedTextAst>()
                    ? value.get_immutable_view<clp::ffi::EightByteEncodedTextAst>().to_string()
                    : value.get_immutable_view<clp::ffi::FourByteEncodedTextAst>().to_string()
    )};

    return evaluate_string_filter_op(op, filter_operand, value_operand, case_sensitive_match);
}
}  // namespace

auto schema_tree_node_type_to_literal_types(SchemaTree::Node::Type node_type)
        -> clp_s::search::ast::literal_type_bitmask_t {
    switch (node_type) {
        case SchemaTree::Node::Type::Int:
            return LiteralType::IntegerT;
        case SchemaTree::Node::Type::Float:
            return LiteralType::FloatT;
        case SchemaTree::Node::Type::Bool:
            return LiteralType::BooleanT;
        case SchemaTree::Node::Type::Str:
            return LiteralType::ClpStringT | LiteralType::VarStringT | LiteralType::TimestampT;
        case SchemaTree::Node::Type::UnstructuredArray:
            return LiteralType::ArrayT;
        case SchemaTree::Node::Type::Obj:
            // TODO: Add `LiteralType::ObjectT` once supported.
            return LiteralType::NullT;
        default:
            return LiteralType::UnknownT;
    }
}

auto schema_tree_node_type_value_pair_to_literal_type(
        SchemaTree::Node::Type node_type,
        std::optional<Value> const& value
) -> clp_s::search::ast::LiteralType {
    if (false == value.has_value()) {
        // TODO: Consider `LiteralType::ObjectT` once supported.
        return LiteralType::UnknownT;
    }

    switch (node_type) {
        case SchemaTree::Node::Type::Int:
            return LiteralType::IntegerT;
        case SchemaTree::Node::Type::Float:
            return LiteralType::FloatT;
        case SchemaTree::Node::Type::Bool:
            return LiteralType::BooleanT;
        case SchemaTree::Node::Type::UnstructuredArray:
            return LiteralType::ArrayT;
        case SchemaTree::Node::Type::Str:
            if (value.value().is<std::string>()) {
                return LiteralType::VarStringT;
            }
            return LiteralType::ClpStringT;
        case SchemaTree::Node::Type::Obj:
            if (value.value().is_null()) {
                return LiteralType::NullT;
            }
            // TODO: Return `LiteralType::ObjectT` once supported.
            return LiteralType::UnknownT;
        default:
            return LiteralType::UnknownT;
    }
}

auto evaluate_filter_against_literal_type_value_pair(
        clp_s::search::ast::FilterExpr const* filter,
        LiteralType literal_type,
        std::optional<Value> const& value,
        bool case_sensitive_match
) -> ystdlib::error_handling::Result<bool> {
    auto const op{filter->get_operation()};
    if (FilterOperation::EXISTS == op) {
        return true;
    }
    if (FilterOperation::NEXISTS == op) {
        return false;
    }

    switch (literal_type) {
        case LiteralType::IntegerT:
            return value.has_value() && evaluate_int_filter_op(op, filter->get_operand(), *value);
        case LiteralType::FloatT:
            return value.has_value() && evaluate_float_filter_op(op, filter->get_operand(), *value);
        case LiteralType::BooleanT:
            return value.has_value() && evaluate_bool_filter_op(op, filter->get_operand(), *value);
        case LiteralType::VarStringT:
            return value.has_value()
                   && evaluate_var_string_filter_op(
                           op,
                           filter->get_operand(),
                           *value,
                           case_sensitive_match
                   );
        case LiteralType::ClpStringT:
            if (false == value.has_value()) {
                return false;
            }
            return evaluate_clp_string_filter_op(
                    op,
                    filter->get_operand(),
                    *value,
                    case_sensitive_match
            );
        case LiteralType::TimestampT:
        case LiteralType::ArrayT:
            return ErrorCode{ErrorCodeEnum::LiteralTypeUnsupported};
        case LiteralType::NullT:
        case LiteralType::UnknownT:
        default:
            return ErrorCode{ErrorCodeEnum::LiteralTypeUnexpected};
    }
}
}  // namespace clp::ffi::ir_stream::search

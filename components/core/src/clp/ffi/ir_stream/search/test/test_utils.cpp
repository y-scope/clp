#include <algorithm>
#include <concepts>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>

#include "../../../../../clp_s/search/ast/BooleanLiteral.hpp"
#include "../../../../../clp_s/search/ast/ColumnDescriptor.hpp"
#include "../../../../../clp_s/search/ast/Expression.hpp"
#include "../../../../../clp_s/search/ast/FilterExpr.hpp"
#include "../../../../../clp_s/search/ast/FilterOperation.hpp"
#include "../../../../../clp_s/search/ast/Integral.hpp"
#include "../../../../../clp_s/search/ast/Literal.hpp"
#include "../../../../../clp_s/search/ast/StringLiteral.hpp"
#include "../../../../../clp_s/search/ast/TimestampLiteral.hpp"
#include "../../../../ir/types.hpp"
#include "../../../EncodedTextAst.hpp"
#include "../../../Value.hpp"
#include "../utils.hpp"

namespace clp::ffi::ir_stream::search::test {
namespace {
using clp::ffi::Value;
using clp::ffi::value_bool_t;
using clp::ffi::value_float_t;
using clp::ffi::value_int_t;
using clp::ir::eight_byte_encoded_variable_t;
using clp::ir::four_byte_encoded_variable_t;
using clp_s::search::ast::BooleanLiteral;
using clp_s::search::ast::ColumnDescriptor;
using clp_s::search::ast::Expression;
using clp_s::search::ast::FilterExpr;
using clp_s::search::ast::FilterOperation;
using clp_s::search::ast::Integral;
using clp_s::search::ast::Literal;
using clp_s::search::ast::LiteralType;
using clp_s::search::ast::StringLiteral;
using clp_s::search::ast::TimestampLiteral;

using ValueToMatchedFilterOpsPair
        = std::pair<std::optional<Value>, std::unordered_set<FilterOperation>>;

constexpr std::string_view cRefTestString{"test"};

/**
 * Asserts the filter evaluation results match the expectation on the given values.
 * @param filter_to_test
 * @param filter_operand_literal_type
 * @param value_to_matched_filter_ops_pairs A vector of pairs, where each pair consists of:
 *   - A value to test against the filter.
 *   - All of the filter operations that are expected to evaluate to `true` for this value.
 * @return Whether all evaluations match their expected results.
 */
[[nodiscard]] auto assert_filter_evaluation_results_on_values(
        FilterExpr const* filter_to_test,
        LiteralType filter_operand_literal_type,
        std::vector<ValueToMatchedFilterOpsPair> const& value_to_matched_filter_ops_pairs
) -> bool;

/**
 * Checks filter evaluation against integral values.
 * @tparam IntegralValueType
 * @param filter_to_test
 * @param filter_operand_literal_type
 * @return Whether all evaluations match their expected results.
 */
template <typename IntegralValueType>
requires std::same_as<IntegralValueType, value_int_t>
         || std::same_as<IntegralValueType, value_float_t>
[[nodiscard]] auto check_filter_evaluation_against_integral_values(
        FilterExpr const* filter_to_test,
        LiteralType filter_operand_literal_type
) -> bool;

/**
 * Checks filter evaluation against bool values.
 * @param filter_to_test
 * @param filter_operand_literal_type
 * @return Whether all evaluations match their expected results.
 */
[[nodiscard]] auto check_filter_evaluation_against_bool_values(
        FilterExpr const* filter_to_test,
        LiteralType filter_operand_literal_type
) -> bool;

/**
 * Checks filter evaluation against and variable strings.
 * @param filter_to_test
 * @param filter_operand_literal_type
 * @return Whether all evaluations match their expected results.
 */
[[nodiscard]] auto check_filter_evaluation_against_var_strings(
        FilterExpr const* filter_to_test,
        LiteralType filter_operand_literal_type
) -> bool;

/**
 * Checks filter evaluation against and encoded text ASTs (Clp strings).
 * @tparam encoded_variable_t
 * @param filter_to_test
 * @param filter_operand_literal_type
 * @return Whether the filter evaluation results match expectation.
 */
template <typename encoded_variable_t>
requires std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>
         || std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>
[[nodiscard]] auto check_filter_evaluation_against_encoded_text_asts(
        FilterExpr const* filter_to_test,
        LiteralType filter_operand_literal_type
) -> bool;

/**
 * Checks filter evaluation against all supported values.
 * @param filter_to_test
 * @param filter_operand_literal_type
 * @return Whether all evaluations match their expected results.
 */
[[nodiscard]] auto
check_filter_evaluation(FilterExpr const* filter_to_test, LiteralType filter_operand_literal_type)
        -> bool;

auto assert_filter_evaluation_results_on_values(
        FilterExpr const* filter_to_test,
        LiteralType filter_operand_literal_type,
        std::vector<ValueToMatchedFilterOpsPair> const& value_to_matched_filter_ops_pairs
) -> bool {
    return std::ranges::all_of(
            value_to_matched_filter_ops_pairs,
            [&](auto const& value_and_matched_filter_ops_pair) {
                auto const& [value_to_test, matched_filter_ops]{value_and_matched_filter_ops_pair};
                bool const expected_match_result{
                        matched_filter_ops.contains(filter_to_test->get_operation())
                };
                auto const actual_match_result{evaluate_filter_against_literal_type_value_pair(
                        filter_to_test,
                        filter_operand_literal_type,
                        value_to_test,
                        false
                )};
                REQUIRE_FALSE(actual_match_result.has_error());
                return expected_match_result == actual_match_result.value();
            }
    );
}

template <typename IntegralValueType>
requires std::same_as<IntegralValueType, value_int_t>
         || std::same_as<IntegralValueType, value_float_t>
auto check_filter_evaluation_against_integral_values(
        FilterExpr const* filter_to_test,
        LiteralType filter_operand_literal_type
) -> bool {
    auto const op{filter_to_test->get_operation()};
    constexpr auto cValueLiteralType{
            std::same_as<IntegralValueType, value_int_t> ? LiteralType::IntegerT
                                                         : LiteralType::FloatT
    };
    if (cValueLiteralType != filter_operand_literal_type) {
        std::optional<Value> const empty_value_to_evaluate;
        auto const actual_match_result{evaluate_filter_against_literal_type_value_pair(
                filter_to_test,
                filter_operand_literal_type,
                empty_value_to_evaluate,
                false
        )};
        REQUIRE_FALSE(actual_match_result.has_error());
        return false == actual_match_result.value();
    }

    IntegralValueType filter_operand{};
    if constexpr (std::same_as<IntegralValueType, value_int_t>) {
        REQUIRE(filter_to_test->get_operand()->as_int(filter_operand, op));
    } else {
        REQUIRE(filter_to_test->get_operand()->as_float(filter_operand, op));
    }

    std::vector<ValueToMatchedFilterOpsPair> const value_to_matched_filter_ops_pairs{
            {Value{filter_operand},
             {FilterOperation::EQ, FilterOperation::GTE, FilterOperation::LTE}},
            {Value{filter_operand + static_cast<IntegralValueType>(1)},
             {FilterOperation::NEQ, FilterOperation::GT, FilterOperation::GTE}},
            {Value{filter_operand - static_cast<IntegralValueType>(1)},
             {FilterOperation::NEQ, FilterOperation::LT, FilterOperation::LTE}},
    };

    return assert_filter_evaluation_results_on_values(
            filter_to_test,
            filter_operand_literal_type,
            value_to_matched_filter_ops_pairs
    );
}

auto check_filter_evaluation_against_bool_values(
        FilterExpr const* filter_to_test,
        LiteralType filter_operand_literal_type
) -> bool {
    if (LiteralType::BooleanT != filter_operand_literal_type) {
        std::optional<Value> const empty_value_to_evaluate;
        auto const actual_match_result{evaluate_filter_against_literal_type_value_pair(
                filter_to_test,
                filter_operand_literal_type,
                empty_value_to_evaluate,
                false
        )};
        REQUIRE_FALSE(actual_match_result.has_error());
        return false == actual_match_result.value();
    }

    auto const op{filter_to_test->get_operation()};
    bool filter_operand{};
    REQUIRE(filter_to_test->get_operand()->as_bool(filter_operand, op));

    std::vector<ValueToMatchedFilterOpsPair> const value_to_matched_filter_ops_pairs{
            {Value{filter_operand}, {FilterOperation::EQ}},
            {Value{false == filter_operand}, {FilterOperation::NEQ}}
    };

    return assert_filter_evaluation_results_on_values(
            filter_to_test,
            filter_operand_literal_type,
            value_to_matched_filter_ops_pairs
    );
}

auto check_filter_evaluation_against_var_strings(
        FilterExpr const* filter_to_test,
        LiteralType filter_operand_literal_type
) -> bool {
    if (LiteralType::VarStringT != filter_operand_literal_type) {
        std::optional<Value> const empty_value_to_evaluate;
        auto const actual_match_result{evaluate_filter_against_literal_type_value_pair(
                filter_to_test,
                filter_operand_literal_type,
                empty_value_to_evaluate,
                false
        )};
        REQUIRE_FALSE(actual_match_result.has_error());
        return false == actual_match_result.value();
    }

    std::vector<ValueToMatchedFilterOpsPair> const value_to_matched_filter_ops_pairs{
            {Value{std::string{cRefTestString}}, {FilterOperation::EQ}},
            {Value{std::string{cRefTestString} + std::string{cRefTestString}},
             {FilterOperation::EQ}},
            {Value{std::string{cRefTestString.data(), cRefTestString.size() / 2}},
             {FilterOperation::NEQ}}
    };

    return assert_filter_evaluation_results_on_values(
            filter_to_test,
            filter_operand_literal_type,
            value_to_matched_filter_ops_pairs
    );
}

template <typename encoded_variable_t>
requires std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>
         || std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>
auto check_filter_evaluation_against_encoded_text_asts(
        FilterExpr const* filter_to_test,
        LiteralType filter_operand_literal_type
) -> bool {
    if (LiteralType::ClpStringT != filter_operand_literal_type) {
        std::optional<Value> const empty_value_to_evaluate;
        auto const actual_match_result{evaluate_filter_against_literal_type_value_pair(
                filter_to_test,
                filter_operand_literal_type,
                empty_value_to_evaluate,
                false
        )};
        REQUIRE_FALSE(actual_match_result.has_error());
        return false == actual_match_result.value();
    }

    std::string const matched{"The test ID=" + std::string{cRefTestString}};
    std::string const unmatched{"This is an unmatched string."};
    std::vector<ValueToMatchedFilterOpsPair> const value_to_matched_filter_ops_pairs{
            {Value{EncodedTextAst<encoded_variable_t>::parse_and_encode_from(matched)},
             {FilterOperation::EQ}},
            {Value{EncodedTextAst<encoded_variable_t>::parse_and_encode_from(unmatched)},
             {FilterOperation::NEQ}},
    };

    return assert_filter_evaluation_results_on_values(
            filter_to_test,
            filter_operand_literal_type,
            value_to_matched_filter_ops_pairs
    );
}

auto
check_filter_evaluation(FilterExpr const* filter_to_test, LiteralType filter_operand_literal_type)
        -> bool {
    auto const op{filter_to_test->get_operation()};

    CAPTURE(FilterExpr::op_type_str(op));
    CAPTURE(Literal::type_to_string(filter_operand_literal_type));

    std::optional<Value> const empty_value_to_evaluate;
    if (FilterOperation::EXISTS == op) {
        auto const actual_match_result{evaluate_filter_against_literal_type_value_pair(
                filter_to_test,
                filter_operand_literal_type,
                empty_value_to_evaluate,
                false
        )};
        REQUIRE_FALSE(actual_match_result.has_error());
        return actual_match_result.value();
    }
    if (FilterOperation::NEXISTS == op) {
        auto const actual_match_result{evaluate_filter_against_literal_type_value_pair(
                filter_to_test,
                filter_operand_literal_type,
                empty_value_to_evaluate,
                false
        )};
        REQUIRE_FALSE(actual_match_result.has_error());
        return false == actual_match_result.value();
    }

    if (false
        == check_filter_evaluation_against_integral_values<value_int_t>(
                filter_to_test,
                filter_operand_literal_type
        ))
    {
        WARN("Failed to evaluate filter against integers.");
        return false;
    }

    if (false
        == check_filter_evaluation_against_integral_values<value_float_t>(
                filter_to_test,
                filter_operand_literal_type
        ))
    {
        WARN("Failed to evaluate filter against floats.");
        return false;
    }

    if (false
        == check_filter_evaluation_against_bool_values(filter_to_test, filter_operand_literal_type))
    {
        WARN("Failed to evaluate filter against booleans.");
        return false;
    }

    if (false
        == check_filter_evaluation_against_var_strings(filter_to_test, filter_operand_literal_type))
    {
        WARN("Failed to evaluate filter against var strings.");
        return false;
    }

    if (false
        == check_filter_evaluation_against_encoded_text_asts<four_byte_encoded_variable_t>(
                filter_to_test,
                filter_operand_literal_type
        ))
    {
        WARN("Failed to evaluate filter against encoded text ASTs (four-byte-encoded).");
        return false;
    }

    if (false
        == check_filter_evaluation_against_encoded_text_asts<eight_byte_encoded_variable_t>(
                filter_to_test,
                filter_operand_literal_type
        ))
    {
        WARN("Failed to evaluate filter against encoded text ASTs (eight-byte-encoded).");
        return false;
    }

    return true;
}
}  // namespace

TEST_CASE("ffi_ir_stream_search_filter_evaluation", "[ffi][ir_stream][search]") {
    constexpr value_int_t cRefInt{0};
    constexpr value_float_t cRefFloat{0};
    constexpr value_bool_t cRefBool{false};

    auto ref_str_literal{StringLiteral::create("*" + std::string{cRefTestString} + "*")};
    auto ref_bool_literal{BooleanLiteral::create_from_bool(cRefBool)};
    auto ref_int_literal{Integral::create_from_int(cRefInt)};
    auto ref_float_literal{Integral::create_from_float(cRefFloat)};
    auto ref_timestamp_literal{TimestampLiteral::create(cRefInt)};

    REQUIRE_FALSE((nullptr == ref_str_literal));
    REQUIRE_FALSE((nullptr == ref_bool_literal));
    REQUIRE_FALSE((nullptr == ref_int_literal));
    REQUIRE_FALSE((nullptr == ref_float_literal));
    REQUIRE_FALSE((nullptr == ref_timestamp_literal));

    constexpr std::string_view cColumnName{"test_column"};
    auto column_descriptor{
            ColumnDescriptor::create_from_escaped_tokens({std::string{cColumnName}}, "")
    };
    REQUIRE_FALSE((nullptr == column_descriptor));

    std::vector<std::pair<std::shared_ptr<Expression>, LiteralType>> const filter_and_type_pairs{
            // Exists/Not-exists
            {FilterExpr::create(column_descriptor, FilterOperation::EXISTS), LiteralType::UnknownT},
            {FilterExpr::create(column_descriptor, FilterOperation::NEXISTS),
             LiteralType::UnknownT},

            // Int
            {FilterExpr::create(column_descriptor, FilterOperation::EQ, ref_int_literal),
             LiteralType::IntegerT},
            {FilterExpr::create(column_descriptor, FilterOperation::NEQ, ref_int_literal),
             LiteralType::IntegerT},
            {FilterExpr::create(column_descriptor, FilterOperation::LT, ref_int_literal),
             LiteralType::IntegerT},
            {FilterExpr::create(column_descriptor, FilterOperation::LTE, ref_int_literal),
             LiteralType::IntegerT},
            {FilterExpr::create(column_descriptor, FilterOperation::GT, ref_int_literal),
             LiteralType::IntegerT},
            {FilterExpr::create(column_descriptor, FilterOperation::GTE, ref_int_literal),
             LiteralType::IntegerT},

            // Float
            {FilterExpr::create(column_descriptor, FilterOperation::EQ, ref_float_literal),
             LiteralType::FloatT},
            {FilterExpr::create(column_descriptor, FilterOperation::NEQ, ref_float_literal),
             LiteralType::FloatT},
            {FilterExpr::create(column_descriptor, FilterOperation::LT, ref_float_literal),
             LiteralType::FloatT},
            {FilterExpr::create(column_descriptor, FilterOperation::LTE, ref_float_literal),
             LiteralType::FloatT},
            {FilterExpr::create(column_descriptor, FilterOperation::GT, ref_float_literal),
             LiteralType::FloatT},
            {FilterExpr::create(column_descriptor, FilterOperation::GTE, ref_float_literal),
             LiteralType::FloatT},

            // Timestamp
            {FilterExpr::create(column_descriptor, FilterOperation::EQ, ref_timestamp_literal),
             LiteralType::IntegerT},
            {FilterExpr::create(column_descriptor, FilterOperation::NEQ, ref_timestamp_literal),
             LiteralType::IntegerT},
            {FilterExpr::create(column_descriptor, FilterOperation::LT, ref_timestamp_literal),
             LiteralType::IntegerT},
            {FilterExpr::create(column_descriptor, FilterOperation::LTE, ref_timestamp_literal),
             LiteralType::IntegerT},
            {FilterExpr::create(column_descriptor, FilterOperation::GT, ref_timestamp_literal),
             LiteralType::IntegerT},
            {FilterExpr::create(column_descriptor, FilterOperation::GTE, ref_timestamp_literal),
             LiteralType::IntegerT},
            {FilterExpr::create(column_descriptor, FilterOperation::EQ, ref_timestamp_literal),
             LiteralType::FloatT},
            {FilterExpr::create(column_descriptor, FilterOperation::NEQ, ref_timestamp_literal),
             LiteralType::FloatT},
            {FilterExpr::create(column_descriptor, FilterOperation::LT, ref_timestamp_literal),
             LiteralType::FloatT},
            {FilterExpr::create(column_descriptor, FilterOperation::LTE, ref_timestamp_literal),
             LiteralType::FloatT},
            {FilterExpr::create(column_descriptor, FilterOperation::GT, ref_timestamp_literal),
             LiteralType::FloatT},
            {FilterExpr::create(column_descriptor, FilterOperation::GTE, ref_timestamp_literal),
             LiteralType::FloatT},

            // Bool
            {FilterExpr::create(column_descriptor, FilterOperation::EQ, ref_bool_literal),
             LiteralType::BooleanT},
            {FilterExpr::create(column_descriptor, FilterOperation::NEQ, ref_bool_literal),
             LiteralType::BooleanT},

            // VarString
            {FilterExpr::create(column_descriptor, FilterOperation::EQ, ref_str_literal),
             LiteralType::VarStringT},
            {FilterExpr::create(column_descriptor, FilterOperation::NEQ, ref_str_literal),
             LiteralType::VarStringT},

            // ClpString
            {FilterExpr::create(column_descriptor, FilterOperation::EQ, ref_str_literal),
             LiteralType::ClpStringT},
            {FilterExpr::create(column_descriptor, FilterOperation::NEQ, ref_str_literal),
             LiteralType::ClpStringT},
    };

    auto const [filter_expr, filter_operand_literal_type]
            = GENERATE_COPY(from_range(filter_and_type_pairs));
    auto const filter_to_test{std::dynamic_pointer_cast<FilterExpr>(filter_expr)};
    REQUIRE_FALSE((nullptr == filter_to_test));
    REQUIRE(check_filter_evaluation(filter_to_test.get(), filter_operand_literal_type));
}
}  // namespace clp::ffi::ir_stream::search::test

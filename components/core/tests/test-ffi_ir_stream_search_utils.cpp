#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include <Catch2/single_include/catch2/catch.hpp>

#include "../src/clp/ffi/encoding_methods.hpp"
#include "../src/clp/ffi/ir_stream/search/utils.hpp"
#include "../src/clp/ffi/Value.hpp"
#include "../src/clp/ir/EncodedTextAst.hpp"
#include "../src/clp/ir/types.hpp"
#include "../src/clp_s/search/ast/BooleanLiteral.hpp"
#include "../src/clp_s/search/ast/ColumnDescriptor.hpp"
#include "../src/clp_s/search/ast/Expression.hpp"
#include "../src/clp_s/search/ast/FilterExpr.hpp"
#include "../src/clp_s/search/ast/FilterOperation.hpp"
#include "../src/clp_s/search/ast/Integral.hpp"
#include "../src/clp_s/search/ast/Literal.hpp"
#include "../src/clp_s/search/ast/StringLiteral.hpp"

namespace {
using clp::ffi::ir_stream::search::evaluate_filter_against_literal_type_value_pair;
using clp::ffi::Value;
using clp::ffi::value_bool_t;
using clp::ffi::value_float_t;
using clp::ffi::value_int_t;
using clp::ir::eight_byte_encoded_variable_t;
using clp::ir::EightByteEncodedTextAst;
using clp::ir::four_byte_encoded_variable_t;
using clp::ir::FourByteEncodedTextAst;
using clp_s::search::ast::BooleanLiteral;
using clp_s::search::ast::ColumnDescriptor;
using clp_s::search::ast::Expression;
using clp_s::search::ast::FilterExpr;
using clp_s::search::ast::FilterOperation;
using clp_s::search::ast::Integral;
using clp_s::search::ast::Literal;
using clp_s::search::ast::LiteralType;
using clp_s::search::ast::StringLiteral;

constexpr std::string_view cRefTestString{"test"};

/**
 * Parses and encodes the given string as an instance of `EncodedTextAst`.
 * @tparam encoded_variable_t
 * @param text
 * @return The encoded result.
 */
template <typename encoded_variable_t>
requires std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>
         || std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>
[[nodiscard]] auto get_encoded_text_ast(std::string_view text)
        -> clp::ir::EncodedTextAst<encoded_variable_t>;

/**
 * Checks filter evaluation against integral values.
 * @tparam IntegralValueType
 * @param filter_to_test
 * @param filter_operand_literal_type
 * @return Whether the filter evaluation results match expectation.
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
 * @return Whether the filter evaluation results match expectation.
 */
[[nodiscard]] auto check_filter_evaluation_against_bool_values(
        FilterExpr const* filter_to_test,
        LiteralType filter_operand_literal_type
) -> bool;

/**
 * Checks filter evaluation against and variable strings.
 * @param filter_to_test
 * @param filter_operand_literal_type
 * @return Whether the filter evaluation results match expectation.
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
 * Checks filter evaluation against different types of values.
 * @param filter_to_test
 * @param filter_operand_literal_type
 * @return Whether the filter evaluation results match expectation.
 */
[[nodiscard]] auto
check_filter_evaluation(FilterExpr const* filter_to_test, LiteralType filter_operand_literal_type)
        -> bool;

template <typename encoded_variable_t>
requires std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>
         || std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>
auto get_encoded_text_ast(std::string_view text) -> clp::ir::EncodedTextAst<encoded_variable_t> {
    std::string logtype;
    std::vector<encoded_variable_t> encoded_vars;
    std::vector<int32_t> dict_var_bounds;
    REQUIRE(clp::ffi::encode_message(text, logtype, encoded_vars, dict_var_bounds));
    REQUIRE(((dict_var_bounds.size() % 2) == 0));

    std::vector<std::string> dict_vars;
    for (size_t i{0}; i < dict_var_bounds.size(); i += 2) {
        auto const begin_pos{static_cast<size_t>(dict_var_bounds[i])};
        auto const end_pos{static_cast<size_t>(dict_var_bounds[i + 1])};
        dict_vars.emplace_back(text.cbegin() + begin_pos, text.cbegin() + end_pos);
    }

    return clp::ir::EncodedTextAst<encoded_variable_t>{logtype, dict_vars, encoded_vars};
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
        return false
               == evaluate_filter_against_literal_type_value_pair(
                       filter_to_test,
                       filter_operand_literal_type,
                       empty_value_to_evaluate,
                       false
               );
    }

    IntegralValueType filter_operand{};
    if constexpr (std::same_as<IntegralValueType, value_int_t>) {
        REQUIRE(filter_to_test->get_operand()->as_int(filter_operand, op));
    } else {
        REQUIRE(filter_to_test->get_operand()->as_float(filter_operand, op));
    }

    std::vector<std::pair<std::optional<Value>, std::unordered_set<FilterOperation>>> const
            value_and_matched_filter_ops_pairs{
                    {Value{filter_operand},
                     {FilterOperation::EQ, FilterOperation::GTE, FilterOperation::LTE}},
                    {Value{filter_operand + static_cast<IntegralValueType>(1)},
                     {FilterOperation::NEQ, FilterOperation::GT, FilterOperation::GTE}},
                    {Value{filter_operand - static_cast<IntegralValueType>(1)},
                     {FilterOperation::NEQ, FilterOperation::LT, FilterOperation::LTE}},
            };

    return std::ranges::all_of(
            value_and_matched_filter_ops_pairs,
            [&](auto const& value_and_matched_filter_ops_pair) {
                auto const& [value_to_test, matched_filter_ops]{value_and_matched_filter_ops_pair};
                bool const expected_match_result{matched_filter_ops.contains(op)};
                return expected_match_result
                       == evaluate_filter_against_literal_type_value_pair(
                               filter_to_test,
                               cValueLiteralType,
                               value_to_test,
                               false
                       );
            }
    );
}

auto check_filter_evaluation_against_bool_values(
        FilterExpr const* filter_to_test,
        LiteralType filter_operand_literal_type
) -> bool {
    if (LiteralType::BooleanT != filter_operand_literal_type) {
        std::optional<Value> const empty_value_to_evaluate;
        return false
               == evaluate_filter_against_literal_type_value_pair(
                       filter_to_test,
                       filter_operand_literal_type,
                       empty_value_to_evaluate,
                       false
               );
    }

    auto const op{filter_to_test->get_operation()};
    bool filter_operand{};
    REQUIRE(filter_to_test->get_operand()->as_bool(filter_operand, op));

    std::vector<std::pair<std::optional<Value>, FilterOperation>> const
            value_and_matched_filter_op_pairs{
                    {Value{filter_operand}, {FilterOperation::EQ}},
                    {Value{false == filter_operand}, {FilterOperation::NEQ}}
            };

    return std::ranges::all_of(
            value_and_matched_filter_op_pairs,
            [&](auto const& value_and_matched_filter_op_pair) {
                auto const& [value_to_test, matched_filter_op]{value_and_matched_filter_op_pair};
                bool const expected_match_result{op == matched_filter_op ? true : false};
                return expected_match_result
                       == evaluate_filter_against_literal_type_value_pair(
                               filter_to_test,
                               LiteralType::BooleanT,
                               value_to_test,
                               false
                       );
            }
    );
}

auto check_filter_evaluation_against_var_strings(
        FilterExpr const* filter_to_test,
        LiteralType filter_operand_literal_type
) -> bool {
    if (LiteralType::VarStringT != filter_operand_literal_type) {
        std::optional<Value> const empty_value_to_evaluate;
        return false
               == evaluate_filter_against_literal_type_value_pair(
                       filter_to_test,
                       filter_operand_literal_type,
                       empty_value_to_evaluate,
                       false
               );
    }

    std::vector<std::pair<std::optional<Value>, FilterOperation>> const
            value_and_matched_filter_op_pairs{
                    {Value{std::string{cRefTestString}}, {FilterOperation::EQ}},
                    {Value{std::string{cRefTestString} + std::string{cRefTestString}},
                     {FilterOperation::EQ}},
                    {Value{std::string{cRefTestString.data(), cRefTestString.size() / 2}},
                     {FilterOperation::NEQ}}
            };

    return std::ranges::all_of(
            value_and_matched_filter_op_pairs,
            [&](auto const& value_and_matched_filter_op_pair) {
                auto const& [value_to_test, matched_filter_op]{value_and_matched_filter_op_pair};
                bool const expected_match_result{
                        filter_to_test->get_operation() == matched_filter_op ? true : false
                };
                return expected_match_result
                       == evaluate_filter_against_literal_type_value_pair(
                               filter_to_test,
                               LiteralType::VarStringT,
                               value_to_test,
                               false
                       );
            }
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
        return false
               == evaluate_filter_against_literal_type_value_pair(
                       filter_to_test,
                       filter_operand_literal_type,
                       empty_value_to_evaluate,
                       false
               );
    }

    std::string const matched{"The test ID=" + std::string{cRefTestString}};
    std::string const unmatched{"This is an unmatched string."};
    std::vector<std::pair<std::optional<Value>, FilterOperation>> const
            value_and_matched_filter_op_pairs{
                    {Value{get_encoded_text_ast<encoded_variable_t>(matched)},
                     {FilterOperation::EQ}},
                    {Value{get_encoded_text_ast<encoded_variable_t>(unmatched)},
                     {FilterOperation::NEQ}},
            };

    return std::ranges::all_of(
            value_and_matched_filter_op_pairs,
            [&](auto const& value_and_matched_filter_op_pair) {
                auto const& [value_to_test, matched_filter_op]{value_and_matched_filter_op_pair};
                bool const expected_match_result{
                        filter_to_test->get_operation() == matched_filter_op ? true : false
                };
                return expected_match_result
                       == evaluate_filter_against_literal_type_value_pair(
                               filter_to_test,
                               LiteralType::ClpStringT,
                               value_to_test,
                               false
                       );
            }
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
        return evaluate_filter_against_literal_type_value_pair(
                filter_to_test,
                filter_operand_literal_type,
                empty_value_to_evaluate,
                false
        );
    }
    if (FilterOperation::NEXISTS == op) {
        return false
               == evaluate_filter_against_literal_type_value_pair(
                       filter_to_test,
                       filter_operand_literal_type,
                       empty_value_to_evaluate,
                       false
               );
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

    REQUIRE_FALSE((nullptr == ref_str_literal));
    REQUIRE_FALSE((nullptr == ref_bool_literal));
    REQUIRE_FALSE((nullptr == ref_int_literal));
    REQUIRE_FALSE((nullptr == ref_float_literal));

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

    auto const [filter_expr, filer_operand_literal_type]{
            GENERATE_COPY(from_range(filter_and_type_pairs))
    };
    auto const filter_to_test{std::dynamic_pointer_cast<FilterExpr>(filter_expr)};
    REQUIRE_FALSE((nullptr == filter_to_test));
    REQUIRE(check_filter_evaluation(filter_to_test.get(), filer_operand_literal_type));
}

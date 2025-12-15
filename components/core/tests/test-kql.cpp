#include <sstream>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <fmt/base.h>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include "../src/clp_s/search/ast/AndExpr.hpp"
#include "../src/clp_s/search/ast/ColumnDescriptor.hpp"
#include "../src/clp_s/search/ast/FilterExpr.hpp"
#include "../src/clp_s/search/ast/OrExpr.hpp"
#include "../src/clp_s/search/ast/TimestampLiteral.hpp"
#include "../src/clp_s/search/kql/kql.hpp"
#include "LogSuppressor.hpp"

using clp_s::search::ast::AndExpr;
using clp_s::search::ast::DescriptorToken;
using clp_s::search::ast::FilterExpr;
using clp_s::search::ast::FilterOperation;
using clp_s::search::ast::OrExpr;
using clp_s::search::ast::TimestampLiteral;
using clp_s::search::kql::parse_kql_expression;
using std::string;
using std::stringstream;
using std::vector;

TEST_CASE("Test parsing KQL", "[KQL]") {
    // Suppress logging
    LogSuppressor suppressor{};

    SECTION("Pure wildcard key queries") {
        auto query = GENERATE(
                "value",
                "  value ",
                "\"value\"",
                " \"value\" ",
                "*:value",
                " * : value ",
                "*:\"value\"",
                " *:   \"value\" ",
                "\"*\":value",
                " \"*\" :value",
                "\"*\":\"value\"",
                "   \"*\":  \"value\" "
        );
        stringstream wildcard_filter{query};
        auto filter = std::dynamic_pointer_cast<FilterExpr>(parse_kql_expression(wildcard_filter));
        REQUIRE(nullptr != filter);
        REQUIRE(nullptr != filter->get_operand());
        REQUIRE(nullptr != filter->get_column());
        REQUIRE(false == filter->has_only_expression_operands());
        REQUIRE(false == filter->is_inverted());
        REQUIRE(filter->get_column()->is_pure_wildcard());
        REQUIRE(filter->get_column()->get_namespace().empty());
        REQUIRE(false == filter->get_column()->get_subtree_type().has_value());
        REQUIRE(FilterOperation::EQ == filter->get_operation());
        std::string extracted_value;
        REQUIRE(filter->get_operand()->as_var_string(extracted_value, filter->get_operation()));
        REQUIRE("value" == extracted_value);
    }

    SECTION("Basic filter") {
        auto query = GENERATE("key:value", " key   : value ");
        stringstream basic_filter{query};
        auto filter = std::dynamic_pointer_cast<FilterExpr>(parse_kql_expression(basic_filter));
        REQUIRE(nullptr != filter);
        REQUIRE(nullptr != filter->get_operand());
        REQUIRE(nullptr != filter->get_column());
        REQUIRE(false == filter->has_only_expression_operands());
        REQUIRE(false == filter->is_inverted());
        REQUIRE(FilterOperation::EQ == filter->get_operation());
        REQUIRE(1 == filter->get_column()->get_descriptor_list().size());
        REQUIRE(filter->get_column()->get_namespace().empty());
        REQUIRE(false == filter->get_column()->get_subtree_type().has_value());
        auto const key_token = DescriptorToken::create_descriptor_from_escaped_token("key");
        REQUIRE(key_token == *filter->get_column()->descriptor_begin());
        std::string extracted_value;
        REQUIRE(filter->get_operand()->as_var_string(extracted_value, filter->get_operation()));
        REQUIRE("value" == extracted_value);
    }

    SECTION("Basic NOT filter") {
        auto query = GENERATE("not key:value", " not key  : value ");
        stringstream basic_not{query};
        auto not_filter = std::dynamic_pointer_cast<FilterExpr>(parse_kql_expression(basic_not));
        REQUIRE(nullptr != not_filter);
        REQUIRE(nullptr != not_filter->get_operand());
        REQUIRE(nullptr != not_filter->get_column());
        REQUIRE(false == not_filter->has_only_expression_operands());
        REQUIRE(true == not_filter->is_inverted());
        REQUIRE(FilterOperation::EQ == not_filter->get_operation());
        REQUIRE(1 == not_filter->get_column()->get_descriptor_list().size());
        REQUIRE(not_filter->get_column()->get_namespace().empty());
        REQUIRE(false == not_filter->get_column()->get_subtree_type().has_value());
        auto const key_token = DescriptorToken::create_descriptor_from_escaped_token("key");
        REQUIRE(key_token == *not_filter->get_column()->descriptor_begin());
        std::string extracted_value;
        REQUIRE(not_filter->get_operand()
                        ->as_var_string(extracted_value, not_filter->get_operation()));
        REQUIRE("value" == extracted_value);
    }

    SECTION("Incorrect NOT filter") {
        auto query = GENERATE("NOT :", "NOT key: ", "NOT  : value");
        stringstream incorrect_query{query};
        auto failure = parse_kql_expression(incorrect_query);
        REQUIRE(nullptr == failure);
    }

    SECTION("Basic AND expression") {
        auto query = GENERATE(
                "key1:value1 and key2:value2",
                "key1  : value1 and  key2  : value2",
                "key1:value1 AND key2:value2",
                "key1  : value1 AND  key2  : value2",
                "key1:value1 aND key2:value2",
                "key1  : value1 aND  key2  : value2"
        );
        stringstream basic_and{query};
        auto and_expr = std::dynamic_pointer_cast<AndExpr>(parse_kql_expression(basic_and));
        REQUIRE(nullptr != and_expr);
        REQUIRE(false == and_expr->is_inverted());
        REQUIRE(true == and_expr->has_only_expression_operands());
        REQUIRE(2 == and_expr->get_num_operands());
        int suffix_number = 1;
        for (auto const& operand : and_expr->get_op_list()) {
            auto filter = std::dynamic_pointer_cast<FilterExpr>(operand);
            REQUIRE(nullptr != filter);
            REQUIRE(nullptr != filter->get_operand());
            REQUIRE(nullptr != filter->get_column());
            REQUIRE(false == filter->has_only_expression_operands());
            REQUIRE(false == filter->is_inverted());
            REQUIRE(FilterOperation::EQ == filter->get_operation());
            std::string extracted_value;
            REQUIRE(true
                    == filter->get_operand()
                               ->as_var_string(extracted_value, filter->get_operation()));
            std::string key = "key" + std::to_string(suffix_number);
            std::string value = "value" + std::to_string(suffix_number);
            REQUIRE(value == extracted_value);
            auto const key_token = DescriptorToken::create_descriptor_from_escaped_token(key);
            REQUIRE(key_token == *filter->get_column()->descriptor_begin());
            REQUIRE(filter->get_column()->get_namespace().empty());
            REQUIRE(false == filter->get_column()->get_subtree_type().has_value());
            ++suffix_number;
        }
    }

    SECTION("Incorrect AND filter") {
        auto query = GENERATE(
                "a : a AND b :",
                " : a AND b :",
                ": a AND b :b",
                " AND b :b",
                "a: a AND"
        );
        stringstream incorrect_query{query};
        auto failure = parse_kql_expression(incorrect_query);
        REQUIRE(nullptr == failure);
    }

    SECTION("Basic OR expression") {
        auto query = GENERATE(
                "key1:value1 or key2:value2",
                "key1  : value1 or  key2  : value2",
                "key1:value1 OR key2:value2",
                "key1  : value1 OR  key2  : value2",
                "key1:value1 oR key2:value2",
                "key1  : value1 oR  key2  : value2"
        );
        stringstream basic_or{query};
        auto or_expr = std::dynamic_pointer_cast<OrExpr>(parse_kql_expression(basic_or));
        REQUIRE(nullptr != or_expr);
        REQUIRE(false == or_expr->is_inverted());
        REQUIRE(true == or_expr->has_only_expression_operands());
        REQUIRE(2 == or_expr->get_num_operands());
        int suffix_number = 1;
        for (auto const& operand : or_expr->get_op_list()) {
            auto filter = std::dynamic_pointer_cast<FilterExpr>(operand);
            REQUIRE(nullptr != filter);
            REQUIRE(nullptr != filter->get_operand());
            REQUIRE(nullptr != filter->get_column());
            REQUIRE(false == filter->has_only_expression_operands());
            REQUIRE(false == filter->is_inverted());
            REQUIRE(FilterOperation::EQ == filter->get_operation());
            std::string extracted_value;
            REQUIRE(true
                    == filter->get_operand()
                               ->as_var_string(extracted_value, filter->get_operation()));
            std::string key = "key" + std::to_string(suffix_number);
            std::string value = "value" + std::to_string(suffix_number);
            REQUIRE(value == extracted_value);
            auto const key_token = DescriptorToken::create_descriptor_from_escaped_token(key);
            REQUIRE(key_token == *filter->get_column()->descriptor_begin());
            REQUIRE(filter->get_column()->get_namespace().empty());
            REQUIRE(false == filter->get_column()->get_subtree_type().has_value());
            ++suffix_number;
        }
    }

    SECTION("Incorrect OR filter") {
        auto query = GENERATE("a : a OR b :", " : a OR b :", ": a OR b :b", " OR b :b", "a: a OR");
        stringstream incorrect_query{query};
        auto failure = parse_kql_expression(incorrect_query);
        REQUIRE(nullptr == failure);
    }

    SECTION("Escape sequences in column name") {
        auto query = GENERATE(
                "a\\.b.c: *",
                "\"a\\.b.c\": *",
                "a\\.b: {c: *}",
                "\"a\\.b\": {\"c\": *}"
        );
        stringstream escaped_column_query{query};
        auto filter
                = std::dynamic_pointer_cast<FilterExpr>(parse_kql_expression(escaped_column_query));
        REQUIRE(nullptr != filter);
        REQUIRE(nullptr != filter->get_operand());
        REQUIRE(nullptr != filter->get_column());
        REQUIRE(false == filter->has_only_expression_operands());
        REQUIRE(false == filter->is_inverted());
        REQUIRE(FilterOperation::EQ == filter->get_operation());
        REQUIRE(2 == filter->get_column()->get_descriptor_list().size());
        REQUIRE(filter->get_column()->get_namespace().empty());
        REQUIRE(false == filter->get_column()->get_subtree_type().has_value());
        auto it = filter->get_column()->descriptor_begin();
        auto const a_b_token = DescriptorToken::create_descriptor_from_escaped_token("a.b");
        REQUIRE(a_b_token == *it++);
        auto const c_token = DescriptorToken::create_descriptor_from_escaped_token("c");
        REQUIRE(c_token == *it++);
    }

    SECTION("Illegal escape sequences in column name") {
        auto query = GENERATE(
                //"a\\:*", this case is technically legal since ':' gets escaped
                "\"a\\\":*",
                "a\\ :*",
                "\"a\\\" :*",
                "a.:*",
                "\"a.\":*",
                "a. :*",
                "\"a.\" :*"
        );
        stringstream illegal_escape{query};
        auto filter = parse_kql_expression(illegal_escape);
        REQUIRE(nullptr == filter);
    }

    SECTION("Empty token in column name") {
        auto query = GENERATE(".a:*", "a.:*", "a..c:*", "a.b.:*");
        stringstream empty_token_column{query};
        auto filter = parse_kql_expression(empty_token_column);
        REQUIRE(nullptr == filter);
    }

    SECTION("Escape sequences in value") {
        auto translated_pair = GENERATE(
                std::pair{"\\\\", "\\\\"},
                std::pair{"\\??", "\\??"},
                std::pair{"\\**", "\\**"},
                std::pair{"\\u9999", "香"},
                std::pair{"\\r\\n\\t\\b\\f", "\r\n\t\b\f"},
                std::pair{"\\\"", "\""},
                std::pair{"\\{\\}\\(\\)\\<\\>", "{}()<>"},
                std::pair{"\\u003F", "\\?"},
                std::pair{"\\u002A", "\\*"},
                std::pair{"\\u005C", "\\\\"}
        );

        auto formatted_query = fmt::format("*: \"{}\"", translated_pair.first);
        stringstream query{formatted_query};
        auto filter = std::dynamic_pointer_cast<FilterExpr>(parse_kql_expression(query));
        REQUIRE(nullptr != filter);
        REQUIRE(nullptr != filter->get_operand());
        REQUIRE(nullptr != filter->get_column());
        REQUIRE(false == filter->has_only_expression_operands());
        REQUIRE(false == filter->is_inverted());
        REQUIRE(FilterOperation::EQ == filter->get_operation());
        REQUIRE(true == filter->get_column()->is_pure_wildcard());
        REQUIRE(filter->get_column()->get_namespace().empty());
        REQUIRE(false == filter->get_column()->get_subtree_type().has_value());
        std::string literal;
        REQUIRE(true == filter->get_operand()->as_var_string(literal, FilterOperation::EQ));
        REQUIRE(literal == translated_pair.second);
    }

    SECTION("Namespaced column") {
        auto namespace_name = GENERATE("@", "$", "!", "#");
        auto namespaced_query_fmt_string = GENERATE("{}column : *", "\"{}column\": *");
        auto namespaced_query
                = fmt::vformat(namespaced_query_fmt_string, fmt::make_format_args(namespace_name));
        stringstream query{namespaced_query};
        auto filter = std::dynamic_pointer_cast<FilterExpr>(parse_kql_expression(query));
        REQUIRE(nullptr != filter);
        REQUIRE(nullptr != filter->get_operand());
        REQUIRE(nullptr != filter->get_column());
        REQUIRE(false == filter->has_only_expression_operands());
        REQUIRE(false == filter->is_inverted());
        REQUIRE(FilterOperation::EQ == filter->get_operation());
        REQUIRE(namespace_name == filter->get_column()->get_namespace());
        REQUIRE(false == filter->get_column()->get_subtree_type().has_value());
        REQUIRE(1 == filter->get_column()->get_descriptor_list().size());
        auto it = filter->get_column()->descriptor_begin();
        auto const column_token = DescriptorToken::create_descriptor_from_escaped_token("column");
        REQUIRE(column_token == *it);
    }

    SECTION("Escaped namespace in column") {
        auto namespace_name = GENERATE("@", "$", "!", "#");
        auto escaped_namespace_query_fmt_string = GENERATE("\\{}column : *", "\"\\{}column\": *");
        auto column = fmt::vformat("{}column", fmt::make_format_args(namespace_name));
        auto escaped_namespace_query = fmt::vformat(
                escaped_namespace_query_fmt_string,
                fmt::make_format_args(namespace_name)
        );
        stringstream query{escaped_namespace_query};
        auto filter = std::dynamic_pointer_cast<FilterExpr>(parse_kql_expression(query));
        REQUIRE(nullptr != filter);
        REQUIRE(nullptr != filter->get_operand());
        REQUIRE(nullptr != filter->get_column());
        REQUIRE(false == filter->has_only_expression_operands());
        REQUIRE(false == filter->is_inverted());
        REQUIRE(FilterOperation::EQ == filter->get_operation());
        REQUIRE(filter->get_column()->get_namespace().empty());
        REQUIRE(false == filter->get_column()->get_subtree_type().has_value());
        REQUIRE(1 == filter->get_column()->get_descriptor_list().size());
        auto it = filter->get_column()->descriptor_begin();
        auto const column_token = DescriptorToken::create_descriptor_from_escaped_token(column);
        REQUIRE(column_token == *it);
    }

    SECTION("Timestamp expressions are parsed correctly.") {
        auto const [query, expected_operation] = GENERATE(
                std::make_pair(
                        R"(* : timestamp("1970-01-01 00:00:00.000000001"))",
                        FilterOperation::EQ
                ),
                std::make_pair(R"(* : timestamp("1", "\N"))", FilterOperation::EQ),
                std::make_pair(
                        R"(* < timestamp("1970-01-01 00:00:00.000000001"))",
                        FilterOperation::LT
                ),
                std::make_pair(R"(* < timestamp("1", "\N"))", FilterOperation::LT)
        );

        stringstream query_stream{query};
        auto filter{std::dynamic_pointer_cast<FilterExpr>(parse_kql_expression(query_stream))};
        REQUIRE(nullptr != filter);
        REQUIRE(nullptr != filter->get_operand());
        REQUIRE(nullptr != filter->get_column());
        REQUIRE(expected_operation == filter->get_operation());
        REQUIRE_FALSE(filter->has_only_expression_operands());
        REQUIRE_FALSE(filter->is_inverted());
        REQUIRE(filter->get_column()->is_pure_wildcard());
        REQUIRE(filter->get_column()->get_namespace().empty());
        REQUIRE_FALSE(filter->get_column()->get_subtree_type().has_value());
        auto timestamp_literal{std::dynamic_pointer_cast<TimestampLiteral>(filter->get_operand())};
        REQUIRE(nullptr != timestamp_literal);
        REQUIRE(1 == timestamp_literal->as_precision(TimestampLiteral::Precision::Nanoseconds));
    }

    SECTION("Invalid timestamp expressions are rejected.") {
        auto invalid_query = GENERATE(
                R"(a: timestamp()",
                R"(a: timestamp())",
                R"(a: timestamp(,))",
                R"(a: timestamp("1",)",
                R"(a: timestamp("1)",
                R"(a: timestamp(1))",
                R"(a: timestamp("a"))",
                R"(a: timestamp("12345", "\Y-\m-\d"))",
                R"(a: timestamp("12345", "\N))"
        );
        stringstream invalid_query_stream{invalid_query};
        REQUIRE(nullptr == parse_kql_expression(invalid_query_stream));
    }
}

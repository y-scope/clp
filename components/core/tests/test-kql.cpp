#include <sstream>
#include <string>
#include <vector>

#include <Catch2/single_include/catch2/catch.hpp>
#include <spdlog/spdlog.h>

#include "../src/clp_s/search/AndExpr.hpp"
#include "../src/clp_s/search/FilterExpr.hpp"
#include "../src/clp_s/search/kql/kql.hpp"
#include "../src/clp_s/search/OrExpr.hpp"

using clp_s::search::AndExpr;
using clp_s::search::DescriptorToken;
using clp_s::search::FilterExpr;
using clp_s::search::FilterOperation;
using clp_s::search::kql::parse_kql_expression;
using clp_s::search::OrExpr;
using std::string;
using std::stringstream;
using std::vector;

TEST_CASE("Test parsing KQL", "[KQL]") {
    // Suppress logging
    auto previous_logging_level = spdlog::default_logger()->level();
    spdlog::default_logger()->set_level(spdlog::level::off);

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
        REQUIRE(DescriptorToken{"key"} == *filter->get_column()->descriptor_begin());
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
        REQUIRE(DescriptorToken{"key"} == *not_filter->get_column()->descriptor_begin());
    }

    SECTION("Incorrect NOT filter") {
        auto query = GENERATE("NOT :", "NOT key: ", "NOT  : value");
        stringstream incorrect_query{query};
        auto failure = parse_kql_expression(incorrect_query);
        REQUIRE(nullptr == failure);
    }

    SECTION("Basic AND expression") {
        auto query = GENERATE(
                "a:a and b:b",
                "a  : a and  b  : b",
                "a:a AND b:b",
                "a  : a AND  b  : b",
                "a:a aND b:b",
                "a  : a aND  b  : b"
        );
        stringstream basic_and{query};
        auto and_expr = std::dynamic_pointer_cast<AndExpr>(parse_kql_expression(basic_and));
        REQUIRE(nullptr != and_expr);
        REQUIRE(false == and_expr->is_inverted());
        REQUIRE(true == and_expr->has_only_expression_operands());
        REQUIRE(2 == and_expr->get_num_operands());
        char c = 'a';
        for (auto const& operand : and_expr->get_op_list()) {
            std::string const str(1, c);
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
            REQUIRE(str == extracted_value);
            REQUIRE(DescriptorToken{str} == *filter->get_column()->descriptor_begin());
            ++c;
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
                "a:a or b:b",
                "a  : a or  b  : b",
                "a:a OR b:b",
                "a  : a OR  b  : b",
                "a:a oR b:b",
                "a  : a oR  b  : b"
        );
        stringstream basic_or{query};
        auto or_expr = std::dynamic_pointer_cast<OrExpr>(parse_kql_expression(basic_or));
        REQUIRE(nullptr != or_expr);
        REQUIRE(false == or_expr->is_inverted());
        REQUIRE(true == or_expr->has_only_expression_operands());
        REQUIRE(2 == or_expr->get_num_operands());
        char c = 'a';
        for (auto const& operand : or_expr->get_op_list()) {
            std::string const str(1, c);
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
            REQUIRE(str == extracted_value);
            REQUIRE(DescriptorToken{str} == *filter->get_column()->descriptor_begin());
            ++c;
        }
    }

    SECTION("Incorrect OR filter") {
        auto query = GENERATE("a : a OR b :", " : a OR b :", ": a OR b :b", " OR b :b", "a: a OR");
        stringstream incorrect_query{query};
        auto failure = parse_kql_expression(incorrect_query);
        REQUIRE(nullptr == failure);
    }

    // Re-enable logging
    spdlog::default_logger()->set_level(previous_logging_level);
}

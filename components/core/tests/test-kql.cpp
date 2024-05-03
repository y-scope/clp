#include <sstream>
#include <string>
#include <vector>

#include <Catch2/single_include/catch2/catch.hpp>

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
    // Initialize data for testing
    vector<string> const basic_equivalent_wildcard_queries{
            "value",
            "\"value\"",
            "*:value",
            "*:\"value\"",
            "\"*\":value"
    };

    SECTION("Parsing basic wildcard column queries") {
        for (auto const& query : basic_equivalent_wildcard_queries) {
            stringstream wildcard_filter{query};
            std::string extracted_value;
            auto filter
                    = std::dynamic_pointer_cast<FilterExpr>(parse_kql_expression(wildcard_filter));
            REQUIRE(nullptr != filter);
            REQUIRE(false == filter->is_inverted());
            REQUIRE(filter->get_column()->is_pure_wildcard());
            REQUIRE(FilterOperation::EQ == filter->get_operation());
            REQUIRE(filter->get_operand()->as_var_string(extracted_value, filter->get_operation()));
            REQUIRE("value" == extracted_value);
        }
    }

    SECTION("Parsing basic filter") {
        stringstream basic_filter{"key:value"};
        auto filter = std::dynamic_pointer_cast<FilterExpr>(parse_kql_expression(basic_filter));
        REQUIRE(nullptr != filter);
        REQUIRE(false == filter->is_inverted());
        REQUIRE(FilterOperation::EQ == filter->get_operation());
        REQUIRE(1 == filter->get_column()->get_descriptor_list().size());
        REQUIRE(DescriptorToken{"key"} == *filter->get_column()->descriptor_begin());
    }

    SECTION("Parsing basic NOT filter") {
        stringstream basic_not{"not key : value"};
        auto not_filter = std::dynamic_pointer_cast<FilterExpr>(parse_kql_expression(basic_not));
        REQUIRE(nullptr != not_filter);
        REQUIRE(true == not_filter->is_inverted());
        REQUIRE(FilterOperation::EQ == not_filter->get_operation());
        REQUIRE(1 == not_filter->get_column()->get_descriptor_list().size());
        REQUIRE(DescriptorToken{"key"} == *not_filter->get_column()->descriptor_begin());
    }

    SECTION("Parsing basic AND expression") {
        stringstream basic_and{"a:a and b:b"};
        auto and_expr = std::dynamic_pointer_cast<AndExpr>(parse_kql_expression(basic_and));
        REQUIRE(nullptr != and_expr);
        REQUIRE(false == and_expr->is_inverted());
        REQUIRE(true == and_expr->has_only_expression_operands());
        REQUIRE(2 == and_expr->get_num_operands());
        char c = 'a';
        for (auto operand : and_expr->get_op_list()) {
            std::string const str(1, c);
            auto filter = std::dynamic_pointer_cast<FilterExpr>(operand);
            std::string extracted_value;
            REQUIRE(nullptr != filter);
            REQUIRE(false == filter->is_inverted());
            REQUIRE(FilterOperation::EQ == filter->get_operation());
            REQUIRE(true
                    == filter->get_operand()
                               ->as_var_string(extracted_value, filter->get_operation()));
            REQUIRE(str == extracted_value);
            REQUIRE(DescriptorToken{str} == *filter->get_column()->descriptor_begin());
            ++c;
        }
    }

    SECTION("Parsing basic OR expression") {
        stringstream basic_or{"a:a or b:b"};
        auto or_expr = std::dynamic_pointer_cast<OrExpr>(parse_kql_expression(basic_or));
        REQUIRE(nullptr != or_expr);
        REQUIRE(false == or_expr->is_inverted());
        REQUIRE(true == or_expr->has_only_expression_operands());
        REQUIRE(2 == or_expr->get_num_operands());
        char c = 'a';
        for (auto operand : or_expr->get_op_list()) {
            std::string const str(1, c);
            auto filter = std::dynamic_pointer_cast<FilterExpr>(operand);
            std::string extracted_value;
            REQUIRE(nullptr != filter);
            REQUIRE(false == filter->is_inverted());
            REQUIRE(FilterOperation::EQ == filter->get_operation());
            REQUIRE(true
                    == filter->get_operand()
                               ->as_var_string(extracted_value, filter->get_operation()));
            REQUIRE(str == extracted_value);
            REQUIRE(DescriptorToken{str} == *filter->get_column()->descriptor_begin());
            ++c;
        }
    }
}

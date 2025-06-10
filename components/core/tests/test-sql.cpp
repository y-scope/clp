#include <memory>
#include <sstream>

#include <catch2/catch_test_macros.hpp>

#include "../src/clp_s/search/ast/EmptyExpr.hpp"
#include "../src/clp_s/search/sql/sql.hpp"
#include "LogSuppressor.hpp"

using clp_s::search::ast::EmptyExpr;
using clp_s::search::sql::parse_sql_expression;
using std::stringstream;

TEST_CASE("Test parsing SQL", "[SQL]") {
    // Suppress logging
    LogSuppressor const suppressor;

    SECTION("Stub accepts empty string") {
        stringstream empty_string{""};
        auto filter = std::dynamic_pointer_cast<EmptyExpr>(parse_sql_expression(empty_string));
        REQUIRE((nullptr != filter));
    }
}

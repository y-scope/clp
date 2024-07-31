#include <sstream>
#include <string>
#include <vector>

#include <Catch2/single_include/catch2/catch.hpp>
#include <spdlog/spdlog.h>

#include "../src/clp_s/search/EmptyExpr.hpp"
#include "../src/clp_s/search/sql/sql.hpp"
#include "LogSuppressor.hpp"

using clp_s::search::EmptyExpr;
using clp_s::search::sql::parse_sql_expression;
using std::string;
using std::stringstream;
using std::vector;

TEST_CASE("Test parsing SQL", "[SQL]") {
    // Suppress logging
    LogSuppressor suppressor{};

    SECTION("Stub accepts empty string") {
        stringstream empty_string{""};
        auto filter = std::dynamic_pointer_cast<EmptyExpr>(parse_sql_expression(empty_string));
        REQUIRE(nullptr != filter);
    }
}

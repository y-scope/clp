#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

#include "../TimestampParser.hpp"

namespace clp_s::timestamp_parser::test {
TEST_CASE("timestamp_parser_parse_timestamp", "[clp-s][timestamp_parser]") {
    SECTION("Format specifiers accept legal content.") {
        std::string generated_format_buffer;
        std::vector<std::string> months{
                "January",
                "February",
                "March",
                "April",
                "May",
                "June",
                "July",
                "August",
                "September",
                "October",
                "November",
                "December"
        };
        for (auto const& month : months) {
            auto const timestamp{fmt::format("{}a", month)};
            auto const result{parse_timestamp("\\Ba", timestamp, generated_format_buffer)};
            REQUIRE(false == result.has_error());
        }
    }
}
}  // namespace clp_s::timestamp_parser::test

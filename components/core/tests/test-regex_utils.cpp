#include <regex_utils/ErrorCode.hpp>
#include <regex_utils/regex_translation_utils.hpp>

#include <Catch2/single_include/catch2/catch.hpp>

using clp::regex_utils::regex_to_wildcard;

TEST_CASE("regex_to_wildcard", "[regex_utils][regex_to_wildcard]") {
    // Test empty string
    REQUIRE(regex_to_wildcard("").value().empty());

    // Test simple wildcard translations
    REQUIRE((regex_to_wildcard("xyz").value() == "xyz"));
    REQUIRE((regex_to_wildcard(". xyz .* zyx .").value() == "? xyz * zyx ?"));
    REQUIRE((regex_to_wildcard(". xyz .+ zyx .*").value() == "? xyz ?* zyx *"));

    // Test unescaped meta characters
    REQUIRE((regex_to_wildcard(".? xyz .* zyx .").error() == clp::regex_utils::ErrorCode::Question)
    );
    REQUIRE((regex_to_wildcard(". xyz .** zyx .").error() == clp::regex_utils::ErrorCode::Star));
    REQUIRE((regex_to_wildcard(". xyz .*+ zyx .").error() == clp::regex_utils::ErrorCode::Plus));
    REQUIRE((regex_to_wildcard(". xyz |.* zyx .").error() == clp::regex_utils::ErrorCode::Pipe));
    REQUIRE((regex_to_wildcard(". xyz ^.* zyx .").error() == clp::regex_utils::ErrorCode::Caret));
}

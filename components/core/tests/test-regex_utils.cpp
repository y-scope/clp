#include <regex_utils/ErrorCode.hpp>
#include <regex_utils/regex_translation_utils.hpp>
#include <regex_utils/RegexToWildcardTranslatorConfig.hpp>

#include <Catch2/single_include/catch2/catch.hpp>

using clp::regex_utils::ErrorCode;
using clp::regex_utils::regex_to_wildcard;
using clp::regex_utils::RegexToWildcardTranslatorConfig;

TEST_CASE("regex_to_wildcard", "[regex_utils][regex_to_wildcard]") {
    // Test empty string
    REQUIRE(regex_to_wildcard("").value().empty());

    // Test simple wildcard translations
    REQUIRE((regex_to_wildcard("xyz").value() == "xyz"));
    REQUIRE((regex_to_wildcard(". xyz .* zyx .").value() == "? xyz * zyx ?"));
    REQUIRE((regex_to_wildcard(". xyz .+ zyx .*").value() == "? xyz ?* zyx *"));

    // Test unescaped meta characters
    REQUIRE((regex_to_wildcard(".? xyz .* zyx .").error() == ErrorCode::UnsupportedQuestionMark));
    REQUIRE((regex_to_wildcard(". xyz .** zyx .").error() == ErrorCode::UntranslatableStar));
    REQUIRE((regex_to_wildcard(". xyz .*+ zyx .").error() == ErrorCode::UntranslatablePlus));
    REQUIRE((regex_to_wildcard(". xyz |.* zyx .").error() == ErrorCode::UnsupportedPipe));
    REQUIRE((regex_to_wildcard(". xyz ^.* zyx .").error() == ErrorCode::IllegalCaret));
}

TEST_CASE("regex_to_wildcard_anchor_config", "[regex_utils][regex_to_wildcard][anchor_config]") {
    // Test anchors and prefix/suffix wildcards
    RegexToWildcardTranslatorConfig const config{false, true};
    REQUIRE(((regex_to_wildcard("^", config).value() == "*")));
    REQUIRE((regex_to_wildcard("$", config).value() == "*"));
    REQUIRE((regex_to_wildcard("^xyz$", config).value() == "xyz"));
    REQUIRE((regex_to_wildcard("xyz", config).value() == "*xyz*"));
    REQUIRE((regex_to_wildcard("xyz$$", config).value() == "*xyz"));

    REQUIRE((regex_to_wildcard("xyz$zyx$", config).error() == ErrorCode::IllegalDollarSign));
}
